// Copyright (c) 2019 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "Borax_BL.h"
#include "I2C.h"
#include "Utils.h"
#include <Arduino.h>

#define BL_REPORT_CMD_REG  (0x0005)
#define BL_REPORT_SET_FEAT (0x0337)
#define BL_REPORT_GET_FEAT (0x0237)
#define BL_REPORT_DATA_REG (0x0006)
#define BL_REPORT_LEN (0x0215)  // Always 533
#define BL_REPORT_ID  (0x07)

#define CMD_WRITE     (0x00)
#define CMD_FLUSH     (0x01)
#define CMD_VALIDATE  (0x02)
#define CMD_RESET     (0x03)
#define CMD_FORMAT_IMG  (0x04)
#define CMD_FORMAT_REG  (0x05)
#define CMD_INVOKE_BL   (0x06)
#define CMD_WRITE_MEM   (0x07)
#define CMD_READ_MEM    (0x08)

#define DUMMY_BYTE  (0xDB)

static uint8_t _i2cSlaveAddr;
static uint8_t _dataBuffer[533];

/**** Private Functions  ****/
static void I2C_write_16(uint16_t data)
{
  I2C_write((uint8_t)(data >> 0));
  I2C_write((uint8_t)(data >> 8));
}

static void I2C_write_32(uint32_t data)
{
  I2C_write((uint8_t)(data >> 0));
  I2C_write((uint8_t)(data >> 8));
  I2C_write((uint8_t)(data >> 16));
  I2C_write((uint8_t)(data >> 24));
}

static void I2C_write_SetFeatureReport(uint8_t cmd)
{
  I2C_write_16(BL_REPORT_CMD_REG);
  I2C_write_16(BL_REPORT_SET_FEAT);
  I2C_write_16(BL_REPORT_DATA_REG);
  I2C_write_16(BL_REPORT_LEN);
  I2C_write(BL_REPORT_ID);
  I2C_write(cmd);
}

static void I2C_write_GetFeatureReport()
{
  I2C_write_16(BL_REPORT_CMD_REG);
  I2C_write_16(BL_REPORT_GET_FEAT);
  I2C_write_16(BL_REPORT_DATA_REG);
}

static void I2C_write_DummyBytes(uint16_t count)
{
  uint16_t i = 0;

  for(; i < count; i++)
  {
    I2C_write(DUMMY_BYTE);
  }
}

/**** Public Functions  ****/
void BL_init(uint8_t i2cSlaveAddr)
{
  _i2cSlaveAddr = i2cSlaveAddr;
}

void BL_cmd_write(uint32_t offset, uint32_t numBytes, const uint8_t * dataPtr)
{  
  I2C_beginTransmission(_i2cSlaveAddr);
  
  I2C_write_SetFeatureReport(CMD_WRITE);

  I2C_write_32(offset);

  I2C_write_32(numBytes);

  uint32_t i = 0;

  for(; i < numBytes; i++)
  {
    I2C_write(dataPtr[i]);
  }

  I2C_write_DummyBytes(521 - numBytes);

  I2C_endTransmission(true);
}

void BL_cmd_flush()
{
  I2C_beginTransmission(_i2cSlaveAddr);
  
  I2C_write_SetFeatureReport(CMD_FLUSH);

  I2C_write_DummyBytes(530 - 1);

  I2C_endTransmission(true);
}

void BL_cmd_validate(uint8_t validationType)
{
  I2C_beginTransmission(_i2cSlaveAddr);
  
  I2C_write_SetFeatureReport(CMD_VALIDATE);

  I2C_write(validationType);

  I2C_write_DummyBytes(530 - 2);

  I2C_endTransmission(true);
}

void BL_cmd_reset()
{
  I2C_beginTransmission(_i2cSlaveAddr);
  
  I2C_write_SetFeatureReport(CMD_RESET);

  I2C_write_DummyBytes(530 - 1);

  I2C_endTransmission(true);
}

void BL_cmd_format_image(uint8_t imageType, uint8_t numRegions, uint32_t entryPoint, uint16_t hidDescriptorAddress, uint8_t i2cAddress, uint8_t reportID)
{
  I2C_beginTransmission(_i2cSlaveAddr);
  
  I2C_write_SetFeatureReport(CMD_FORMAT_IMG);

  I2C_write(imageType);

  I2C_write(numRegions);

  I2C_write_32(entryPoint);

  I2C_write_16(hidDescriptorAddress);

  I2C_write(i2cAddress);
  
  I2C_write(reportID);

  I2C_write_DummyBytes(530 - 11);

  I2C_endTransmission(true);
}

void BL_cmd_format_region(uint8_t regionNumber, uint32_t regionOffset, uint32_t regionSize, uint32_t regionChecksum)
{
  I2C_beginTransmission(_i2cSlaveAddr);
  
  I2C_write_SetFeatureReport(CMD_FORMAT_REG);

  I2C_write(regionNumber);

  I2C_write_32(regionOffset);

  I2C_write_32(regionSize);

  I2C_write_32(regionChecksum);

  I2C_write_DummyBytes(530 - 14);

  I2C_endTransmission(true);
}

void BL_cmd_invoke_bootloader()
{
  I2C_beginTransmission(_i2cSlaveAddr);
  
  I2C_write_SetFeatureReport(CMD_INVOKE_BL);

  I2C_write_DummyBytes(530 - 1);

  I2C_endTransmission(true);
}

void BL_cmd_write_memory(uint32_t offset, uint16_t numBytes, uint8_t * dataPtr)
{  
  I2C_beginTransmission(_i2cSlaveAddr);
  
  I2C_write_SetFeatureReport(CMD_WRITE_MEM);

  I2C_write_32(offset);

  I2C_write_16(numBytes);

  uint32_t i = 0;

  for(; i < numBytes; i++)
  {
    I2C_write(dataPtr[i]);
  }

  uint8_t preamble[7] = 
  { 
    CMD_WRITE_MEM, 
    (uint8_t)(offset >> 0), 
    (uint8_t)(offset >> 8),
    (uint8_t)(offset >> 16),
    (uint8_t)(offset >> 14),
    (uint8_t)(numBytes >> 0), 
    (uint8_t)(numBytes >> 8)
  };

  uint16_t checksum = Fletcher16(preamble, 7);
  checksum = Fletcher16_Continue(dataPtr, numBytes, checksum);

  I2C_write_16(checksum);

  I2C_write_DummyBytes(521 - numBytes);

  I2C_endTransmission(true);
}

void BL_cmd_read_memory(uint32_t offset, uint16_t numBytes, uint8_t * dataPtr)
{  
  I2C_beginTransmission(_i2cSlaveAddr);
  
  I2C_write_SetFeatureReport(CMD_READ_MEM);

  I2C_write_32(offset);

  I2C_write_16(numBytes);

  I2C_write_DummyBytes(523);

  I2C_endTransmission(true);

  I2C_beginTransmission(_i2cSlaveAddr);

  I2C_write_GetFeatureReport();

  I2C_endTransmission(false);

  I2C_request(_i2cSlaveAddr, 533, true);

  uint16_t i = 0;
  
  while(I2C_available())
  {
    _dataBuffer[i++] = I2C_read();
  }

  uint16_t start = (_dataBuffer[5] >= 8) ? 17 : 13;
  for(i = 0; i < numBytes; i++)
  {
    dataPtr[i] = _dataBuffer[start + i];
  }
}

bool BL_get_status(bl_status_t * statusPtr)
{
  I2C_beginTransmission(_i2cSlaveAddr);
  
  I2C_write_GetFeatureReport();

  I2C_endTransmission(false);

  I2C_request(_i2cSlaveAddr, 533, true);

  uint16_t i = 0;
  
  while(I2C_available())
  {
    _dataBuffer[i++] = I2C_read();
  }

  // Length_LSB = _dataBuffer[0]
  // Length_MSB = _dataBuffer[1]
  // ReportID = _dataBuffer[2]

  statusPtr->Sentinel = _dataBuffer[3];
  statusPtr->Sentinel |= _dataBuffer[4] << 8;

  statusPtr->Version = _dataBuffer[5];
  statusPtr->LastError = _dataBuffer[6];
  statusPtr->Flags = _dataBuffer[7];

  if(statusPtr->Version >= 8)
  {
    statusPtr->AtomicWriteSize = _dataBuffer[8];
    statusPtr->WriteDelay = _dataBuffer[9];
    statusPtr->FormatDelay = _dataBuffer[10];

    statusPtr->MemAddress = _dataBuffer[11];
    statusPtr->MemAddress |= _dataBuffer[12] << 8;
    statusPtr->MemAddress |= _dataBuffer[13] << 16;
    statusPtr->MemAddress |= _dataBuffer[14] << 24;
    
    statusPtr->NumBytes = _dataBuffer[15];
    statusPtr->NumBytes |= _dataBuffer[16] << 8;

    for(i = 0; i < (statusPtr->NumBytes <= 514 ? statusPtr->NumBytes : 514); i++)
    {
      statusPtr->Data[i] = _dataBuffer[17 + i];
    }
  
    statusPtr->Checksum = _dataBuffer[17 + (statusPtr->NumBytes <= 514 ? statusPtr->NumBytes : 514)];
    statusPtr->Checksum |= _dataBuffer[17 + (statusPtr->NumBytes <= 514 ? statusPtr->NumBytes : 514) + 1] << 8;
  }
  else
  {
    statusPtr->MemAddress = _dataBuffer[8];
    statusPtr->MemAddress |= _dataBuffer[9] << 8;
    statusPtr->MemAddress |= _dataBuffer[10] << 16;
    statusPtr->MemAddress |= _dataBuffer[11] << 24;
    
    statusPtr->NumBytes = _dataBuffer[12];
    statusPtr->NumBytes |= _dataBuffer[13] << 8;

    for(i = 0; i < (statusPtr->NumBytes <= 517 ? statusPtr->NumBytes : 517); i++)
    {
      statusPtr->Data[i] = _dataBuffer[14 + i];
    }
  
    statusPtr->Checksum = _dataBuffer[14 + (statusPtr->NumBytes <= 517 ? statusPtr->NumBytes : 517)];
    statusPtr->Checksum |= _dataBuffer[14 + (statusPtr->NumBytes <= 517 ? statusPtr->NumBytes : 517) + 1] << 8;
  }

  switch( statusPtr->Sentinel )
  {
    case 0x5AC3:
    case 0xC35A:
    case 0x6C42: //'lB'
    case 0x6D49: //'mI'
    case 0x426C: //'Bl' for old firmware
      return true;
    default:
      return false;
  }
}

bool BL_program(const uint8_t * buf, uint32_t numBytes, uint32_t address)
{
	bl_status_t status;

	if( !BL_get_status( &status ) ) return false;

	if( status.LastError != NO_ERROR )
	{
		// Clear the error and retry.
		BL_cmd_reset();
		delay(100);

		// Check status.
		if( !BL_get_status( &status ) || status.LastError != NO_ERROR ) return false;
	}

	// Invoke bootloader.
	if( status.Sentinel == 0x5AC3 || status.Sentinel == 0x6D49 || status.Sentinel == 0x426C )
	{
		BL_cmd_invoke_bootloader();
		delay(100);

		if( !BL_get_status( &status ) ) return false;
	}

	// Get timing values.
	uint32_t FormatImageDelay = 100;
	uint32_t FormatRegionsPageDelay = 50;
	uint32_t PageWriteDelay = 100;

	if( status.Version >= 0x08)
	{
		FormatRegionsPageDelay = status.FormatDelay;
		PageWriteDelay = status.WriteDelay * 10;
	}

	// Format image.
	uint32_t EntryPoint = buf[4] |
						( buf[5] << 8 ) |
						( buf[6] << 16 ) |
						( buf[7] << 24 );

	uint8_t TargetI2CAddress = 0x2C;
	uint16_t TargetHIDDescAddr = 0x0020;
	if( status.Version >= 0x09)
	{
		TargetI2CAddress = 0xFF;
		TargetHIDDescAddr = 0xFFFF;
	}

	BL_cmd_format_image( 0, 1, EntryPoint, TargetHIDDescAddr, TargetI2CAddress, BL_REPORT_ID );
	delay( FormatImageDelay );

	// Format regions.
	BL_cmd_format_region( 0, address, numBytes, Fletcher_32( buf, numBytes ) );

	delay( (FormatRegionsPageDelay * ((numBytes / 1024) + 1)) );

	if( !BL_get_status( &status ) || status.LastError != NO_ERROR ) return false;

	// Write data.
	uint32_t Length = numBytes, MaxDataPayloadSize = 520; // must be even, better if multiple of 4 (520 / 4 = 130)

	while (Length > 0)
	{
		uint32_t PayloadSize = ( Length > MaxDataPayloadSize ) ? MaxDataPayloadSize : Length;

		BL_cmd_write( address + numBytes - Length, PayloadSize, &buf[numBytes - Length] );

		delay( ( PageWriteDelay * PayloadSize > 1000 ) ? PageWriteDelay * PayloadSize / 1000 : 1 );

		if( !BL_get_status( &status ) || status.LastError != NO_ERROR ) return false;

		Length -= PayloadSize;
	}

	// Flush.
	BL_cmd_flush();
	delay(10);
	if( !BL_get_status( &status ) || status.LastError != NO_ERROR ) return false;

	// Validate.
	BL_cmd_validate(1);
	delay(10);
	if( !BL_get_status( &status ) || status.LastError != NO_ERROR /* || ( status.Flags & 0x10 ) == 0 */ )
	{
		BL_cmd_format_image( 0, 1, EntryPoint, TargetHIDDescAddr, TargetI2CAddress, BL_REPORT_ID );
		return false;
	}

	// Reset.
	BL_cmd_reset();
	delay(10);

	// Check status.
	if( !BL_get_status( &status ) || status.LastError != NO_ERROR ) return false;

	// Completed.
	return true;
}
