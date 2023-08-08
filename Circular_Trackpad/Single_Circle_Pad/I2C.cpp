// Copyright (c) 2018 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "I2C.h"
#include "Project_Config.h"
#include <Wire.h>
#include <utility\twi.h>
//#include <i2c_t3.h>

#if BUFFER_LENGTH < 53
#error BUFFER_LENGTH must be at least 53 for I2C_HID serial to work correctly. Go to \Program Files (x86)\Arduino\hardware\arduino\avr\libraries\Wire\src\Wire.h
#error BUFFER_LENGTH must be at least 53 for I2C_HID serial to work correctly. Go to \Program Files (x86)\Arduino\hardware\teensy\avr\libraries\Wire\Wire.h
#error BUFFER_LENGTH must be at least 53 for I2C_HID serial to work correctly. Go to \Program Files (x86)\Arduino\hardware\teensy\avr\libraries\Wire\WireIMXRT.h
#error BUFFER_LENGTH must be at least 53 for I2C_HID serial to work correctly. Go to \Program Files (x86)\Arduino\hardware\teensy\avr\libraries\Wire\WireKinetis.h
#endif

// #define TWI_BUFFER_LENGTH 64

#if TWI_BUFFER_LENGTH < 53
#error TWI_BUFFER_LENGTH must be at least 53 for I2C_HID serial to work correctly. Go to \Program Files (x86)\Arduino\hardware\arduino\avr\libraries\Wire\src\utility\twi.h
#error TWI_BUFFER_LENGTH must be at least 53 for I2C_HID serial to work correctly. Go to \Program Files (x86)\Arduino\hardware\teensy\avr\libraries\Wire\utility\twi.h
#endif

/************************************************************/
/************************************************************/
/********************  PUBLIC FUNCTIONS *********************/

/** Set the Arduino as a I2C Controller if no address is given and sets clock frequency. */
void I2C_init(uint32_t clockFrequency)
{
  Wire.begin();                   // Set the arduino as I2C Controller.
  Wire.setClock(clockFrequency);  // call .setClock after .begin
}

/** request the number of bytes specified by "count" from the given I2C address
 * specified by "address". After transfer of data, set boolean "stop" as true to 
 * release the line. false will keep the line busy to send a restart. */
uint32_t I2C_request(uint8_t address, uint32_t count, bool stop)
{
  return Wire.requestFrom(address, (uint8_t)count, stop);
  // return Wire.requestFrom((int16_t)address, (int16_t)count, stop);
}

/** Returns the number of bytes available for reading. */
uint32_t I2C_available()
{
  return Wire.available();
}

/** returns the next byte. (Reads a byte that was transmitted from I2C 
	peripheral to a controller). */
uint8_t I2C_read()
{
  return Wire.read();
}

/** returns the number of bytes written. Writes data from a I2C peripheral 
	from a request from a controller. */
uint8_t I2C_write(uint8_t data)
{
  return Wire.write(data);
}

/** Begins the transmission to a I2C peripheral with the given address. */
void I2C_beginTransmission(uint8_t address)
{
  Wire.beginTransmission(address);
}

/** Ends the transmission to a I2C peripheral that was begun by the begin transmission. 
	Boolean "stop" if true, sends a stop condiction, releasing the bus. If false, 
	sends a restart request, keeping the connection active. */
i2c_status_t I2C_endTransmission(bool stop)
{
  return (i2c_status_t)Wire.endTransmission(stop);
}

