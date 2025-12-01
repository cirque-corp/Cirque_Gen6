#include "API_ExtMemAccess.h"
#include "I2C.h"

static uint8_t _deviceAddress = 0x2C;
size_t apiExtMemAccessMaxTransfer;

bool API_ExtMemAccess_Init(uint8_t i2cAddr, uint32_t i2cBitRate, size_t maxTransferSize)
{
  _deviceAddress = i2cAddr;

  I2C_init(i2cBitRate);

  apiExtMemAccessMaxTransfer = maxTransferSize;

  return I2C_setBufferLength(maxTransferSize);
}

memStatus_t API_ExtMemAccess_ReadMemory(uint32_t registerAddress, uint8_t * data, uint16_t count)
{
  uint8_t checksum = 0;
  memStatus_t result = MEM_SUCCESS;
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
  
  // Read requested data from device, plus overhead 
  // (3 extra bytes for lengthLow, lengthHigh, & checksum)
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
    result |= MEM_BAD_CHECKSUM;
  }

  if(++bytesRead != (lengthBytes[0] | (lengthBytes[1] << 8)))
  {
    result |= MEM_LENGTH_MISMATCH;
  }

  return result;
}

void API_ExtMemAccess_WriteMemory(uint32_t registerAddress, uint8_t * data, uint8_t count)
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
