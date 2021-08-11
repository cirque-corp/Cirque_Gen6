// Copyright (c) 2018 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license


#include "API_HostBus.h"

#define HID_CMD_RESET 1
#define HID_CMD_GET_REPORT 2
#define HID_CMD_SET_REPORT 3
#define HID_CMD_GET_IDLE 4
#define HID_CMD_SET_IDLE 5
#define HID_CMD_GET_PROTOCOL 6
#define HID_CMD_SET_PROTOCOL 7
#define HID_CMD_SET_POWER 8

#define HID_REPORT_TYPE_INPUT 1
#define HID_REPORT_TYPE_OUTPUT 2
#define HID_REPORT_TYPE_FEATURE 3

uint8_t _deviceAddress;

/************************************************************/
/************************************************************/
/********************  PUBLIC FUNCTIONS *********************/

/** The I2C bus commonly runs at a clock frequency of 400kHz.
	The operation of touch system is also tested at 100kHz. */
void HB_init(int I2CFrequency, uint8_t I2CAddress)
{
  I2C_init(I2CFrequency);
  _deviceAddress = I2CAddress;
  HostDR_init();
}

/** The Host DR Line (Host_DR) is a an output from the touch system that signals when
	there is data to be read. The line two states: Asserted (data is available), 
	and de-asserted (no data is available). The details of the operation of 
	that line are shown in HB_DR_Asserted(). */
bool HB_DR_Asserted(void)
{
  return (HostDR_pinState() == 0);
}

/** The most common I2C action is a "report read" operation to transfer touch 
	information from the touch system to the host. The details of the read 
	operation are shown in HB_readReport(). */
void HB_readReport(uint8_t * reportData, uint16_t readLength)
{
  uint8_t i = 0;
 
  I2C_request((uint16_t)_deviceAddress, readLength, (uint16_t)true);

  while(I2C_available())
  {
    reportData[i++] = I2C_read();
  }
}

/** The touch system functionality is controlled using the Entended Memory 
	Access operations. The details of the memory access process are shown 
	in HB_readExtendedMemory() and HB_writeExtendedMemory(); */
uint8_t HB_readExtendedMemory(uint32_t registerAddress, uint8_t * data, uint16_t count)
{
  uint8_t checksum = 0, result = SUCCESS;
  uint16_t i = 0, bytesRead = 0;
  uint8_t lengthBytes[2];
  uint8_t preamble[8] = 
  {
    0x01,
    0x09,
    (uint8_t)(registerAddress & (uint32_t)0x000000FF),
    (uint8_t)((registerAddress & 0x0000FF00)>>8),
    (uint8_t)((registerAddress & 0x00FF0000)>>16),
    (uint8_t)((registerAddress & 0xFF000000)>>24),
    (uint8_t)(count & 0x00FF),
    (uint8_t)((count & 0xFF00) >> 8)
  };
  
  // Send extended memory access command to Gen4
  I2C_beginTransmission(_deviceAddress);
  for(; i < 8; i++)
  {
    I2C_write(preamble[i]);
  }    
  I2C_endTransmission(false);
  
  /* Read requested data from Gen4, plus overhead 
	(3 extra bytes for lengthLow, lengthHigh, & checksum)
  */
  I2C_request(_deviceAddress, count + 3, true);

  // Read first 2 bytes (lower and upper length-bytes)
  for(i = 0; i < 2; i++)
  {
    checksum += lengthBytes[i] = I2C_read();
    bytesRead++;
  }

  // Read data bytes requested by caller
  for(i = 0; i < count; i++)
  {
    checksum += data[i] = I2C_read();
    bytesRead++;
  }

  // Read the and check the last byte (the checksum byte)
  if(checksum != I2C_read())
  {
    result |= BAD_CHECKSUM;
  }

  if(++bytesRead != (lengthBytes[0] | (lengthBytes[1] << 8)))
  {
    result |= LENGTH_MISMATCH;
  }

  return result;
}

void HB_writeExtendedMemory(uint32_t registerAddress, uint8_t * data, uint8_t count)
{
  uint8_t checksum = 0, i = 0;
  uint8_t preamble[8] = 
  {
    0x00,
    0x09,
    (uint8_t)(registerAddress & 0x000000FF),
    (uint8_t)((registerAddress & 0x0000FF00)>>8),
    (uint8_t)((registerAddress & 0x00FF0000)>>16),
    (uint8_t)((registerAddress & 0xFF000000)>>24),
    (uint8_t)(count & 0x00FF),
    (uint8_t)((count & 0xFF00) >> 8)
  };

  I2C_beginTransmission(_deviceAddress);
  for(; i < 8; i++)
  {
    I2C_write(preamble[i]);
    checksum += preamble[i];
  }    

  for(i = 0; i < count; i++)
  {
    I2C_write(data[i]);
    checksum += data[i];
  }
  I2C_write(checksum);
  I2C_endTransmission(true);
}

// populate an array of bytes with the start of a HID command
// returns the number of bytes required for this command
// (this allows reportID > 14 which uses an extra byte
static uint8_t SetupHidCommandBytes(uint8_t *cmdBytes, size_t bufferLength, uint8_t opcode, uint8_t reportID, uint8_t reportType)
{
    uint8_t result = 0;

    if((cmdBytes == NULL) || (bufferLength < 5)) return 0;
    
    cmdBytes[0] = CIRQUE_HID_COMMAND_REGISTER;
    cmdBytes[1] = CIRQUE_HID_COMMAND_REGISTER >> 8;
	// unfortunately HID is built so that if you need reportID > 14 you have to add an extra byte to the packet
	// this means anything that does a HID command might be n or n + 1 bytes long
	if (reportID > 14)
	{
      cmdBytes[2] = ((reportType << 4) & 0x30) | (0x0F); // set 0xf sentinel
	  cmdBytes[4] = reportID;
	  result = 5;
	}
	else
	{
      cmdBytes[2] = ((reportType << 4) & 0x30) | (reportID & 0x0F);
      result = 4;
	}
    cmdBytes[3] = opcode & 0x0F;
	return result;
}

static void sendHidCommand(uint8_t *cmdBytes, size_t cmdLength, bool stop)
{
    I2C_beginTransmission(_deviceAddress);
    for(int i = 0; i < cmdLength; i++)
    {
        I2C_write(cmdBytes[i]);
    }
    I2C_endTransmission(stop);
}

void HB_HID_SetPower(bool powerOn)
{
    uint8_t cmd[5];
    uint8_t powerState = powerOn ? 0 : 1;
    
    uint8_t cmdLength = SetupHidCommandBytes(cmd, sizeof(cmd), HID_CMD_SET_POWER, powerState, 0);
	sendHidCommand(cmd, cmdLength, true);
}

void HB_HID_Reset(void)
{
	uint8_t cmd[5];
	
	uint8_t cmdLength = SetupHidCommandBytes(cmd, sizeof(cmd), HID_CMD_RESET, 0, 0);
	sendHidCommand(cmd, cmdLength, true);
}

void HB_HID_GetHidDescriptor(uint16_t HidDescAddr, uint8_t hidDesc[30])
{ 
  HB_HID_readRegister(HidDescAddr, hidDesc, 30);
}

void HB_HID_readRegister(uint16_t hidRegister, uint8_t * buffer, uint16_t readLength)
{
  uint16_t i;
   
  I2C_beginTransmission(_deviceAddress);
  I2C_write((uint8_t)hidRegister);
  I2C_write((uint8_t)(hidRegister >> 8));
  I2C_endTransmission(false);

  I2C_request(_deviceAddress, readLength, true);
  for(i = 0; i < readLength; i++)
  {
    buffer[i] = I2C_read();
  }
}

bool HB_HID_readReset(void)
{
	I2C_request(_deviceAddress, 2, true);
	uint8_t lowByte = I2C_read();
	uint8_t highByte = I2C_read();
	
	return ((lowByte == 0) && (highByte == 0)) ? 1 : 0;
}

void HB_HID_getFeatureReport(uint8_t reportID, uint16_t dataRegister, uint8_t *inputBuffer, uint16_t inputLength)
{
  uint16_t i;
  uint8_t cmd[7];
  
  uint8_t cmdLength = SetupHidCommandBytes(cmd, sizeof(cmd), HID_CMD_GET_REPORT, reportID, HID_REPORT_TYPE_FEATURE);
  cmd[cmdLength++] = (uint8_t)dataRegister;
  cmd[cmdLength++] = (uint8_t)(dataRegister >> 8);
  sendHidCommand(cmd, cmdLength, false);
  I2C_request(_deviceAddress, inputLength, true);
  for(i = 0; i < inputLength; i++)
  {
    inputBuffer[i] = I2C_read();
  }
  
}

void HB_HID_setFeatureReport(uint8_t reportID, uint16_t dataRegister, uint16_t data)
{
  uint8_t cmd[11];
  uint8_t cmdLength = SetupHidCommandBytes(cmd, sizeof(cmd), HID_CMD_SET_REPORT, reportID, HID_REPORT_TYPE_FEATURE);
  cmd[cmdLength++] = (uint8_t)dataRegister;
  cmd[cmdLength++] = (uint8_t)(dataRegister >> 8);
  cmd[cmdLength++] = 0x04; // This function only allows 1 data word (2 bytes) so the length is fixed
  cmd[cmdLength++] = 0x00;
  cmd[cmdLength++] = (uint8_t)data;
  cmd[cmdLength++] = (uint8_t)(data >> 8);
  sendHidCommand(cmd, cmdLength, true);
}


// AlpsRegisterAccess
// returns the number of bytes in the packet
static uint8_t setupAlpsRegisterCommandBytes(uint8_t * cmdBytes, size_t bufferLength, uint8_t reportID, uint8_t dataRegister, uint8_t subCmd, uint32_t address, uint8_t data)
{
  uint8_t i;
  uint8_t sum; 

  if((cmdBytes == NULL) || (bufferLength < 16)) return 0;  // Error pathway

  // this sets bytes 0..3 with command register, command
  uint8_t cmdLength = SetupHidCommandBytes(cmdBytes, bufferLength, HID_CMD_SET_REPORT, reportID, HID_REPORT_TYPE_FEATURE);

  // now add data register, length, report id, sub-command, address, data, checksum
  cmdBytes[cmdLength++] = (uint8_t)dataRegister; // data reg, normally you get this from the hid descriptor, it's hardcoded here
  cmdBytes[cmdLength++] = (uint8_t)(dataRegister >> 8);
  uint8_t sumStartIndex = cmdLength;
  cmdBytes[cmdLength++] = 0x0a; // length
  cmdBytes[cmdLength++] = 0x00;
  cmdBytes[cmdLength++] = reportID;
  cmdBytes[cmdLength++] = subCmd;
  cmdBytes[cmdLength++] = (uint8_t)address;
  cmdBytes[cmdLength++] = (uint8_t)(address >> 8);
  cmdBytes[cmdLength++] = (uint8_t)(address >> 16);
  cmdBytes[cmdLength++] = (uint8_t)(address >> 24);
  uint8_t sumEndIndex = cmdLength;
  cmdBytes[cmdLength++] = data;
  
  sum = 0;
  for (i = sumStartIndex; i < sumEndIndex; i++)
  {
	sum += cmdBytes[i];
  }
  cmdBytes[cmdLength++] = sum;
  
  return cmdLength;
}


void HB_ARA_readMemory(uint32_t address, uint8_t * result)
{
  uint8_t cmd[17];
  uint8_t inputBuffer[10];

  // a lot of hard coded stuff here. reportID and data register should probably be passed in
  // Cirque reportID = 0x0e, cirque data register = 0x0006, subCmd = 0x01 (read)
  // normally read the data register location from the report descriptor. It's hard coded here
  uint8_t cmdLength = setupAlpsRegisterCommandBytes(cmd, sizeof(cmd), 0x0e, 0x0006, 0x01, address, 0x00);  // is subCmd 0x01 or 0xD1?
  
  sendHidCommand(cmd, cmdLength, true);

  HB_HID_getFeatureReport(0x0e, 0x0006, inputBuffer, 10); 
  // For data integrity should probably check all of this:
  // inputBuffer[0..1] should be 0x000a
  // inputBuffer[2] should be 0x0e
  // inputBuffer[9] is the checksum
  
  *result = inputBuffer[8]; 
}

void HB_ARA_writeMemory(uint32_t address, uint8_t data)
{
  uint8_t cmd[17];
  uint8_t inputBuffer[10];
  
  // Cirque reportID = 0x0e, cirque data register = 0x0006
  // normally read the data register location from the report descriptor. It's hard coded here
  uint8_t cmdLength = setupAlpsRegisterCommandBytes(cmd, sizeof(cmd), 0x0e, 0x0006, 0x02, address, data);  // 0x02 or 0xD2?
  
  sendHidCommand(cmd, cmdLength, true);

  HB_HID_getFeatureReport(0x0e, 0x0006, inputBuffer, 10);

  // for data integrity should check inputBuffer and see if the data matches what was expected

}
