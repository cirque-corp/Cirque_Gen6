#include "CustomMeasHardware.h"
#include <i2c_t3.h> // this include Arduino.h, so I don't need to include it manually
#include <limits.h>

#define DR_PIN  (9)
#define DBG_PIN (20)

static uint32_t _minBitRate = UINT_MAX;
static bool _i2cInitialized = false;

void HW_init()
{
  pinMode(DR_PIN, INPUT);
}

uint32_t HW_micros()
{
  return micros();
}

bool HW_DR_asserted()
{
  return digitalRead(DR_PIN) == LOW ? true: false;
}

void HW_AssertDbg()
{
  digitalWrite(DBG_PIN, HIGH);
}

void HW_DeAssertDbg()
{
  digitalWrite(DBG_PIN, LOW);
}

void I2C_init(uint32_t clockFrequency)
{
  if(_i2cInitialized == false)
  {
    _i2cInitialized = true;
    Wire.begin();
  }
  
  if(clockFrequency < _minBitRate)
  {
    _minBitRate = clockFrequency;   // limit the bit rate to that of the slowest device on the bus
    Wire.setClock(_minBitRate);     // setClock() **cannot** be called before begin()
  }

  // TODO: compute timeouts for requests and writes based on the byte-count?
  Wire.setDefaultTimeout(25000);
   
  // Setup programmable input glitch filter
  uint8_t temp = *Wire.i2c->FLT;

  uint32_t maxClocks = F_BUS / (Wire.i2c->currentRate * 2);

  temp &= ~0x1F;
  temp |= (maxClocks / 2) & 0x1F;
  
  *Wire.i2c->FLT = temp;
}

bool I2C_setBufferLength(uint32_t requiredBuffLength)
{
#ifndef I2C_TX_BUFFER_LENGTH
  return false;
#endif

#ifndef I2C_RX_BUFFER_LENGTH
  return false;
#endif

  // NOTE: since we cannot modify the underlying i2c_t3.h library at run-time, all we can do
  // is return false if the i2c_t3.h's buffers aren't large enough. When porting this method, implementers may
  // have more control over their I2C implementation, but it should always return false if <requiredBuffLength> isn't supported.
  return (requiredBuffLength + 1) <= I2C_TX_BUFFER_LENGTH && (requiredBuffLength + 1) <= I2C_RX_BUFFER_LENGTH;  // add 1 for the address byte
}

uint32_t I2C_request(uint8_t address, uint32_t count, bool stopI2C)
{
  return Wire.requestFrom(address, (size_t)count, (i2c_stop)stopI2C); // I don't understand this (i2c_stop) cast, but it is required
}

uint32_t I2C_available(void)
{
  return Wire.available();
}

uint8_t I2C_read(void)
{
  return Wire.read();
}

uint8_t I2C_readBytes(uint8_t * buffer, size_t count)
{
  return Wire.read(buffer, count);
}

uint8_t I2C_write(uint8_t data)
{
  return Wire.write(data);
}

void I2C_beginTransmission(uint8_t address)
{
  Wire.beginTransmission(address);
}

i2c_status_t I2C_endTransmission(bool stopI2C)
{
  return (i2c_status_t)Wire.endTransmission(stopI2C);
}
