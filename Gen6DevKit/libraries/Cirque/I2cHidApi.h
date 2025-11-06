#ifndef I2C_HID_API_H
#define I2C_HID_API_H

// Copyright (c) 2025 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "HidReport.h"
#include "HostBusLayer.h"
#include "HidDescriptor.h"

// I2C
#define CIRQUE_PRIMARY_ADDRESS 0x2c
#define CIRQUE_LEGACY_ADDRESS 0x2a

// HID
// !!! this is not generic - this is cirque specific
#define CIRQUE_HID_DESCRIPTOR_ADDRESS 0x0020
#define CIRQUE_HID_REPORT_DESCRIPTOR_REGISTER 0x0002
#define CIRQUE_INPUT_REGISTER 0x0003
#define CIRQUE_OUTPUT_REGISTER 0x0004
#define CIRQUE_HID_COMMAND_REGISTER 0x0005
#define CIRQUE_DATA_REGISTER 0x0006
#define CIRQUE_EXT_WRITE_REGISTER 0x0900
#define CIRQUE_EXT_READ_REGISTER 0x0901
#define CIRQUE_EXT_WRITE_RAW_REGISTER 0x0902
#define CIRQUE_EXT_READ_RAW_REGISTER 0x0903

class I2cHidApi
{
public:
    I2cHidApi(uint8_t i2cAddress, uint16_t maxBufferLength);
    virtual ~I2cHidApi() = 0;

    reportIds_t getReport(HidReport & report);

    void readRegister(uint16_t hidRegister, uint8_t * readBuffer, uint16_t readLength);
    void getFeatureReport(uint8_t reportID, uint16_t dataRegister, uint8_t *inputBuffer, uint16_t inputLength);
    void setFeatureReport(uint8_t reportID, uint16_t dataRegister, uint16_t data);
    void getHidDescriptor(HidDescriptor & descriptor);
    void setPower(bool powerOn);
    void reset(void);

    // extended access
    enum commandErrors : uint8_t
    {
        cmd_okay = 0,
        cmd_checksumBad,
        cmd_lengthWrong,
		cmd_parameterBad
    };
    enum addressMaps : uint8_t
    {
        raw = 0,
        standardVirtual = 1
    };
    commandErrors readExtendedMemory(uint32_t cirqueAddress, 
        uint8_t * readData, uint16_t readDataLength, 
        addressMaps addressMap = addressMaps::standardVirtual);
    void writeExtendedMemory(uint32_t cirqueAddress, 
        uint8_t * writeData, uint16_t writeDataLength, 
        addressMaps addressMap = addressMaps::standardVirtual);
     
    // leaving out the Alps Register Access format (ARA)

protected:
    HostBusLayer * m_host_bus;

    uint8_t m_i2cAddress;
    uint16_t m_maxBufferLength;
    uint8_t * m_commandBuffer;
    
    uint16_t m_descriptorAddress = CIRQUE_HID_DESCRIPTOR_ADDRESS;
    uint16_t m_commandRegister = CIRQUE_HID_COMMAND_REGISTER;
    uint8_t hidReportBuffer[sizeof(AnyHIDReport_t)];

    enum hidReportTypes : uint8_t
    {
        RT_RESERVED = 0,
        RT_INPUT,
        RT_OUTPUT,
        RT_FEATURE
    };

    enum hidOpCodes : uint8_t
    {
        OC_RESERVED0 = 0,
        OC_RESET,          // reset the device at any time
        OC_GET_REPORT,     // request from host to device to retrieve a report (input/feature)
        OC_SET_REPORT,     // request from host to device to set a report (output/feature)
        OC_GET_IDLE,       // legacy
        OC_SET_IDLE,       // legacy
        OC_GET_PROTOCOL,   // legacy
        OC_SET_PROTOCOL,   // legacy
        OC_SET_POWER,      // Request from host to device to indicate power setting
        OC_RESERVED1,
        OC_RESERVED2,
        OC_RESERVED3,
        OC_RESERVED4,
        OC_RESERVED5,
        OC_VENDOR_RESERVED, // vendor specific use
        OC_RESERVED6
    };

    void retrieveReadData(uint8_t * data, uint16_t actualLength, uint16_t requestedLength);
    uint8_t setupHidCommandBytes(uint8_t *cmdBytes, uint16_t bufferLength, 
        hidOpCodes opcode, uint8_t reportID, hidReportTypes reportType);  // returns buffer length used (4 or 5)
    // void appendByteToCommandArray(uint8_t * data, uint16_t maxLength, uint8_t theByte);
    void setupExtendedAccessCommandBytes(uint8_t * commandBuffer, uint16_t commandBufferLength, 
        uint16_t hidRegister, uint32_t extendedAddress, uint16_t dataLength);

};

#endif // I2C_HID_API_H

