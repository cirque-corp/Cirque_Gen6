// Copyright (c) 2025 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "I2cHidApi.h"
#include "HostBusLayer.h"
#include "DataUtils.h"

I2cHidApi::I2cHidApi(uint8_t i2cAddress, uint16_t maxBufferLength)
{
    m_host_bus = HostBusLayer::host_bus;
    m_i2cAddress = i2cAddress;
    m_maxBufferLength = maxBufferLength;
    m_commandBuffer = new uint8_t[m_maxBufferLength]();
}
    
I2cHidApi::~I2cHidApi()
{
    delete[] m_commandBuffer;
}

reportIds_t I2cHidApi::getReport(HidReport & hidReport)
{
    reportIds_t result = id_unknown;
    uint16_t reportSize = sizeof(hidReportBuffer); // buffer is maximum size of hid report
    uint16_t readCount = m_host_bus->read(m_i2cAddress, reportSize); // reads maximum size for the report
    if (readCount >= reportSize) // read all bytes
    {
        for (int x = 0; x < reportSize; x++)
        {
            hidReportBuffer[x] = m_host_bus->fetch();
        }
        if (hidReport.decodeReport(hidReportBuffer))  // report id and hid length okay
        {
            result = hidReport.reportId;
        }
    }

    return result;
}

void I2cHidApi::readRegister(uint16_t hidRegister, uint8_t * readBuffer, uint16_t readLength)
{
    uint8_t writeData[2];
    writeData[0] = (uint8_t)hidRegister;
    writeData[1] = (uint8_t)(hidRegister >> 8);
    uint16_t actualReadCount = m_host_bus->writeRestartRead(m_i2cAddress, 2, writeData, readLength);
    retrieveReadData(readBuffer, actualReadCount, readLength);
}

void I2cHidApi::getFeatureReport(uint8_t reportID, uint16_t dataRegister, uint8_t *inputBuffer, uint16_t inputLength)
{
    uint8_t cmd[7];
    uint8_t cmdLength = setupHidCommandBytes(cmd, sizeof(cmd), I2cHidApi::OC_GET_REPORT, reportID, I2cHidApi::RT_FEATURE);
    // fill in extra data needed for this command - data register
    cmd[cmdLength++] = (uint8_t)dataRegister;
    cmd[cmdLength++] = (uint8_t)(dataRegister >> 8);
    uint16_t actualReadLength = m_host_bus->writeRestartRead(m_i2cAddress, cmdLength, cmd, inputLength);
    retrieveReadData(inputBuffer, actualReadLength, inputLength);
}

void I2cHidApi::setFeatureReport(uint8_t reportID, uint16_t dataRegister, uint16_t data)
{
    uint8_t cmd[11];
    uint8_t cmdLength = setupHidCommandBytes(cmd, sizeof(cmd), I2cHidApi::OC_SET_REPORT, reportID, I2cHidApi::RT_FEATURE);
    // fill in extra data needed for this command - data register
    cmd[cmdLength++] = (uint8_t)dataRegister;
    cmd[cmdLength++] = (uint8_t)(dataRegister >> 8);
    cmd[cmdLength++] = 0x04; // This function only allows 1 data word (2 bytes) so the length is fixed
    cmd[cmdLength++] = 0x00;
    cmd[cmdLength++] = (uint8_t)data;
    cmd[cmdLength++] = (uint8_t)(data >> 8);
    m_host_bus->write(m_i2cAddress, cmdLength, cmd);
}

I2cHidApi::commandErrors I2cHidApi::readExtendedMemory(uint32_t cirqueAddress, uint8_t * readData, uint16_t readDataLength, 
    addressMaps addressMap)
{
    uint8_t commandBuffer[8];
    uint16_t hidRegister = (addressMap == addressMaps::raw) ? CIRQUE_EXT_READ_RAW_REGISTER : CIRQUE_EXT_READ_REGISTER;
    setupExtendedAccessCommandBytes(commandBuffer, sizeof(commandBuffer), hidRegister, cirqueAddress, readDataLength);

    uint16_t bufferLength = 2 + readDataLength + 1;
    uint16_t actualReadCount = m_host_bus->writeRestartRead(m_i2cAddress, sizeof(commandBuffer), commandBuffer, bufferLength);

    uint8_t lengthLSB = m_host_bus->fetch();
    uint8_t lengthMSB = m_host_bus->fetch();
    retrieveReadData(readData, actualReadCount - 3, readDataLength);
    uint8_t expectedChecksum = m_host_bus->fetch();
    uint8_t actualChecksum = calculateChecksum(readData, readDataLength) + lengthLSB + lengthMSB;

    uint16_t hidLength = lengthLSB + (lengthMSB << 8);

    // exit
    if (hidLength != bufferLength) return cmd_lengthWrong;
    if (expectedChecksum != actualChecksum) return cmd_checksumBad;
    return cmd_okay;
}

void I2cHidApi::writeExtendedMemory(uint32_t cirqueAddress, uint8_t * writeData, uint16_t writeDataLength, 
    addressMaps addressMap)
{
    uint16_t commandBufferLength = 8 + writeDataLength + 1;
    // commandBufferLength = limit length based on m_maxBufferLength
    uint16_t hidRegister = (addressMap == addressMaps::raw) ? CIRQUE_EXT_WRITE_RAW_REGISTER : CIRQUE_EXT_WRITE_REGISTER;
    setupExtendedAccessCommandBytes(m_commandBuffer, commandBufferLength, hidRegister, cirqueAddress, writeDataLength);
    uint16_t i = 8;
    for (int x = 0; x < writeDataLength; x++)
    {
        m_commandBuffer[i++] = writeData[x];
    }
    uint8_t checksum = calculateChecksum(m_commandBuffer, commandBufferLength - 1);
    m_commandBuffer[commandBufferLength - 1] = checksum;
    m_host_bus->write(m_i2cAddress, commandBufferLength, m_commandBuffer);
}

void I2cHidApi::getHidDescriptor(HidDescriptor & descriptor)
{
    uint8_t buffer[descriptor.packetLength];
    readRegister(m_descriptorAddress, buffer, descriptor.packetLength);
    descriptor.decodeFrom(buffer);  
}

// Control the "power state" of the device.
// setPower(false) = "go to low, low power state"
// setPower(true) = "go to full on power state"
void I2cHidApi::setPower(bool powerOn)
{
    uint8_t cmd[5];
    uint8_t powerState = powerOn ? 0 : 1; // on = 0, off = 1
    
    uint8_t cmdLength = setupHidCommandBytes(cmd, sizeof(cmd), I2cHidApi::OC_SET_POWER, powerState, I2cHidApi::RT_RESERVED);
	m_host_bus->write(m_i2cAddress, cmdLength, cmd);
}

void I2cHidApi::reset(void)
{
    uint8_t cmd[5];
    uint8_t cmdLength = setupHidCommandBytes(cmd, sizeof(cmd), I2cHidApi::OC_RESET, 0, I2cHidApi::RT_RESERVED);
    m_host_bus->write(m_i2cAddress, cmdLength, cmd);
}

// *** protected ***

void I2cHidApi::retrieveReadData(uint8_t * data, uint16_t actualLength, uint16_t requestedLength)
{
    actualLength = (actualLength <= requestedLength) ? actualLength : requestedLength; // limit readLength to inputLength
    for(int i = 0; i < actualLength; i++)
    {
        data[i] = m_host_bus->fetch();
    }
}

// Populate the first byes of an array with a HID command. Returns the number of bytes that were populated
// so you know where to append additional bytes or how many to send (this allows reportID > 14 which uses an extra byte)
uint8_t I2cHidApi::setupHidCommandBytes(uint8_t *cmdBytes, uint16_t bufferLength, hidOpCodes opcode, uint8_t reportID, hidReportTypes reportType)
{
    // HID Command
    // byte stream: start, addr | w, ack, 
    //              command register lsb, ack, command register msb, ack, 
    //              command lsb, ack, command msb, ack, [optional reportID > 15 byte, ack],
    //              ( )
    // command data lsb[7..0] : Reserved (2bits), Report Type (2bits), Report ID(4 bits)
    // if Requested Report ID > 14 then set Report ID (4 bits) to 1111, and add an extra byte with the Requested Report ID
    // command data msb[7..0] : Reserved (4bits), Op Code (4 bits)
    // The report ID corresponds to the Top Level Collection that the command acts upon. 
    // This field should only be used on complex devices with multiple input, output or feature report collections. 
    // If the device is not a complex HID device, this field must be 0.

    uint8_t result = 0;

    if ((cmdBytes == 0) || (bufferLength < 5)) return 0;

    cmdBytes[0] = (uint8_t)m_commandRegister;
    cmdBytes[1] = (uint8_t)(m_commandRegister >> 8);
    // For reportID > 14 you have to add an extra byte to the command data
    // this means anything that does a HID command might be n or n + 1 bytes long
    if (reportID > 14)
    {
        cmdBytes[2] = ((reportType << 4) & 0x30) | (0x0F); // set 0xf sentinel
        result = 5; // extra byte used
    }
    else
    {
        cmdBytes[2] = ((reportType << 4) & 0x30) | (reportID & 0x0F);
        result = 4;  // extra byte not used
    }
    cmdBytes[3] = opcode & 0x0F;
    cmdBytes[4] = reportID; // always fill this extra byte, might get used, might not
    return result;
}

void I2cHidApi::setupExtendedAccessCommandBytes(uint8_t * commandBuffer, uint16_t commandBufferLength, 
    uint16_t hidRegister, uint32_t extendedAddress, uint16_t dataLength)
{
    if ((commandBuffer == 0) || (commandBufferLength < 8)) return;

    uint16_t i = 0; 
    commandBuffer[i++] = (uint8_t) hidRegister;
    commandBuffer[i++] = (uint8_t)(hidRegister >> 8);
    commandBuffer[i++] = (uint8_t) extendedAddress;
    commandBuffer[i++] = (uint8_t)(extendedAddress >> 8);
    commandBuffer[i++] = (uint8_t)(extendedAddress >> 16);
    commandBuffer[i++] = (uint8_t)(extendedAddress >> 24);
    commandBuffer[i++] = (uint8_t) dataLength;
    commandBuffer[i++] = (uint8_t)(dataLength >> 8);
}




