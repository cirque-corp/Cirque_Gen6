// Copyright (c) 2018 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license
#include "INA219.h"
#include "I2C.h"

#define MAX_I2C_PACKET_LENGTH (3) // 3 

static uint16_t _slaveAddress = 0x40;

/************************************************************/
/************************************************************/
/******************* HELPER FUNCTIONS ***********************/

static void WriteRegister(uint8_t reg, uint16_t value)
{
  I2C_beginTransmission(_slaveAddress);
  I2C_write(reg);
  I2C_write((value >> 8) & 0xFF);
  I2C_write(value & 0xFF);
  I2C_endTransmission(true);
}

static uint16_t ReadRegister(uint8_t reg)
{
  I2C_beginTransmission(_slaveAddress);
  I2C_write(reg);
  I2C_endTransmission(false);  // NOTE: according to 8.5.6, no STOP condition
  I2C_request(_slaveAddress, 2, true);
  return ((I2C_read() << 8) | I2C_read());
}

// Checks the Conversion Ready bit (CNVR) in INA219, returns true if set
// INA219 sets this bit when conversions, plus any additional calculations are complete
bool INA219_dataReady(void)  // Status Register, CNVR bit
{
  if((ReadRegister(REGISTER__BUS_VOLTAGE) & 0x0002) != 0) 
	  return true;
  return false;
}

// Triggers a conversion for current measurement. The number of samples to be averaged (internal to INA219)
// is set by INA219_shuntADCAveraging()
static void INA219_triggerShuntMeasurement(uint16_t averagingMask)
{
  uint16_t temp = ReadRegister(REGISTER__CONFIG);
  temp &= ~(CONFIG__SHUNT_ADC_AVERAGE_128 | CONFIG__SHUNT_PGA_DIV1);
  temp |= CONFIG__MODE_SHUNT_TRIG | CONFIG__SHUNT_PGA_DIV1 | averagingMask;
  WriteRegister(REGISTER__CONFIG, temp);
}

// Triggers a conversion for current measurement. The number of samples to be averaged (internal to INA219)
// is set by INA219_shuntADCAveraging()
static void INA219_triggerBusMeasurement(uint16_t averagingMask)
{
  uint16_t temp = ReadRegister(REGISTER__CONFIG);
  temp &= ~CONFIG__SHUNT_ADC_AVERAGE_128;
  temp |= CONFIG__MODE_BUS_TRIG | averagingMask;
  WriteRegister(REGISTER__CONFIG, temp);
}

/************************************************************/
/************************************************************/
/********************  PUBLIC FUNCTIONS *********************/

// Assigns an I2C slave address and opens the I2C port
// NOTE: This function must be called before calling any other INA219 functions
bool INA219_init(uint8_t slaveAddress, uint32_t i2cBitRate)
{
  _slaveAddress = slaveAddress;
  
  I2C_init(i2cBitRate);

  WriteRegister(REGISTER__CALIBRATION, 13421);
  // bool initSucceeded = I2C_setBufferLength(MAX_I2C_PACKET_LENGTH);
  // return initSucceeded;
  return true;
  

}

// Writes <data> to the config register (Reg: 0x00)
// NOTE: Use CONFIG__ register masks in INA219.h for easy configuration
void INA219_config(uint16_t data)
{
  WriteRegister(REGISTER__CONFIG, data);
}

// Resets all internal INA219 registers to POR defaults
void INA219_reset(void)
{
  uint16_t temp = ReadRegister(REGISTER__CONFIG);
  temp |= CONFIG__RESET;
  WriteRegister(REGISTER__CONFIG, temp);
}

int32_t INA219_readCurrent()
{
  int32_t temp = (int16_t) ReadRegister(REGISTER__SHUNT_VOLTAGE);
  return temp * 10;
} 

int32_t INA219_readBusVoltage()
{
  return (int32_t)((ReadRegister(REGISTER__BUS_VOLTAGE) >> 3) * 4);
}


// Measures voltage between IN+ and IN- (across shunt resistor) in uV
// NOTE: Use CONFIG__SHUNT_ADC_AVERAGE masks in INA219.h for setting
int32_t INA219_measureShuntVoltage(uint16_t averagingMask)
{
  uint32_t delay = 0;
  
  INA219_triggerShuntMeasurement(averagingMask);
  
  switch(averagingMask)
  {
  	case CONFIG__SHUNT_ADC_AVERAGE_2:
  		delay = CONFIG_DELAY_2;
  		break;
  	case CONFIG__SHUNT_ADC_AVERAGE_4:
  		delay = CONFIG_DELAY_4;
  		break;
  	case CONFIG__SHUNT_ADC_AVERAGE_8:
  		delay = CONFIG_DELAY_8;
  		break;	
  	case CONFIG__SHUNT_ADC_AVERAGE_16:
  		delay = CONFIG_DELAY_16;
  		break;
  	case CONFIG__SHUNT_ADC_AVERAGE_32:
  		delay = CONFIG_DELAY_32;
  		break;
  	case CONFIG__SHUNT_ADC_AVERAGE_64:
  		delay = CONFIG_DELAY_64;
  		break;
  	case CONFIG__SHUNT_ADC_AVERAGE_128:
  		delay = CONFIG_DELAY_128;
  		break;
  }
  
  if (delay > 0)
  {
	  delayMicroseconds(delay);
  }
  else
  {
	  while(!INA219_dataReady());
  }
  
  return INA219_readCurrent();
}

// Measures voltage at IN- pin in mV
// NOTE: Use CONFIG__BUS_ADC_AVERAGE masks in INA219.h for setting
int32_t INA219_measureBusVoltage(uint16_t averagingMask, uint32_t delay)
{
  INA219_triggerBusMeasurement(averagingMask);
  if (delay > 0)
  {
	  delayMicroseconds(delay);
  }
  else
  {
	  while(!INA219_dataReady());
  }
  return INA219_readBusVoltage();
}
