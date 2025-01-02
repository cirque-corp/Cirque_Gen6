// Copyright (c) 2018 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "I2C.h"
#include "Project_Config.h"
#include <Wire.h>
#include <utility\twi.h>

#if BUFFER_LENGTH < 53
// Unfortuantely, the Arduino I2C code uses a fixed 32 byte buffer for I2C communications. 
// You will need to change that since HID and PTP touchpads require longer packet lengths.
// If you are using the new Arduino IDE - 2.x or above, you can find the source in your Arduino15 directory
// Arduino IDE 2.x - find your Arduino15 directory, then find and edit the files
// Finding Arduino15 -->  https://support.arduino.cc/hc/en-us/articles/360018448279-Open-the-Arduino15-folder
//      \Arduino15\packages\arduino\hardware\avr\1.8.6\libraries\Wire\src\Wire.h
//      \Arduino15\packages\teensy\hardware\avr\1.59.0\libraries\Wire\Wire.h
//      \Arduino15\packages\teensy\hardware\avr\1.59.0\libraries\Wire\WireKinetis.h
//      \Arduino15\packages\teensy\hardware\avr\1.59.0\libraries\Wire\WireIMXRT.h
// Arduino IDE 1.x - follow the path in the error message
#error BUFFER_LENGTH must be at least 53 for I2C_HID serial to work correctly. Go to \Program Files (x86)\Arduino\hardware\teensy\avr\libraries\Wire\Wire.h
#error BUFFER_LENGTH must be at least 53 for I2C_HID serial to work correctly. Go to \Program Files (x86)\Arduino\hardware\arduino\avr\libraries\Wire\src\Wire.h
#error BUFFER_LENGTH must be at least 53 for I2C_HID serial to work correctly. Go to \Program Files (x86)\Arduino\hardware\teensy\avr\libraries\Wire\WireIMXRT.h
#error BUFFER_LENGTH must be at least 53 for I2C_HID serial to work correctly. Go to \Program Files (x86)\Arduino\hardware\teensy\avr\libraries\Wire\WireKinetis.h
#endif

// #define TWI_BUFFER_LENGTH 64

#if TWI_BUFFER_LENGTH < 53
// See above note -
// Arduino IDE 2.x - find your Arduino15 directory, then find and edit the files
// Finding Arduino15 -->  https://support.arduino.cc/hc/en-us/articles/360018448279-Open-the-Arduino15-folder
//      \Arduino15\packages\arduino\hardware\avr\1.8.6\libraries\Wire\src\utility\twi.h
//      \Arduino15\packages\teensy\hardware\avr\1.59.0\libraries\Wire\utility\twi.h
// Arduino IDE 1.x - follow the path in the error message
#error TWI_BUFFER_LENGTH must be at least 53 for I2C_HID serial to work correctly. Go to \Program Files (x86)\Arduino\hardware\arduino\avr\libraries\Wire\src\utility\twi.h
#error TWI_BUFFER_LENGTH must be at least 53 for I2C_HID serial to work correctly. Go to \Program Files (x86)\Arduino\hardware\teensy\avr\libraries\Wire\utility\twi.h
#endif

/************************************************************/
/************************************************************/
/********************  PUBLIC FUNCTIONS *********************/

/** Set the Arduino as a I2C Controller if no address is given and sets clock frequency. */
void I2C_init(uint8_t i2c_channel, uint32_t clockFrequency)
{
  if (i2c_channel == 0)
  {
    Wire.begin();                   // Set the arduino as I2C Controller.
    Wire.setClock(clockFrequency);  // call .setClock after .begin
  }
  else if (i2c_channel == 1)
  {
    Wire1.begin();                   // Set the arduino as I2C Controller.
    Wire1.setClock(clockFrequency);  // call .setClock after .begin
  }
}

/** request the number of bytes specified by "count" from the given I2C address
 * specified by "address". After transfer of data, set boolean "stop" as true to 
 * release the line. false will keep the line busy to send a restart. */
uint32_t I2C_request(uint8_t i2c_channel, uint8_t address, uint32_t count, bool stop)
{
  uint32_t numBytes = 0;

  if (i2c_channel == 0)
  {
    numBytes = Wire.requestFrom(address, (uint8_t)count, stop);
  }
  else if (i2c_channel == 1)
  {
    numBytes = Wire1.requestFrom(address, (uint8_t)count, stop);
  }

  return numBytes;
}

/** Returns the number of bytes available for reading. */
uint32_t I2C_available(uint8_t i2c_channel)
{
  uint8_t numBytes = 0;

  if (i2c_channel == 0)
  {
    numBytes = Wire.available();
  }
  else if (i2c_channel == 1)
  {
    numBytes = Wire1.available();
  }
  
  return numBytes;
}

/** returns the next byte. (Reads a byte that was transmitted from I2C 
	peripheral to a controller). */
uint8_t I2C_read(uint8_t i2c_channel)
{
  uint8_t data = 0;

  if (i2c_channel == 0)
  {
    data = Wire.read();
  }
  else if (i2c_channel == 1)
  {
    data = Wire1.read();
  }
  
  return data;
}

/** returns the number of bytes written. Writes data from a I2C peripheral 
	from a request from a controller. */
uint8_t I2C_write(uint8_t i2c_channel, uint8_t data)
{
  uint8_t numBytes = 0;

  if (i2c_channel == 0)
  {
    numBytes = Wire.write(data);
  }
  else if (i2c_channel == 1)
  {
    numBytes = Wire1.write(data);
  }
  
  return numBytes;
}

/** Begins the transmission to a I2C peripheral with the given address. */
void I2C_beginTransmission(uint8_t i2c_channel, uint8_t address)
{
  if (i2c_channel == 0)
  {
    Wire.beginTransmission(address);
  }
  else if (i2c_channel == 1)
  {
    Wire1.beginTransmission(address);
  }
}

/** Ends the transmission to a I2C peripheral that was begun by the begin transmission. 
	Boolean "stop" if true, sends a stop condiction, releasing the bus. If false, 
	sends a restart request, keeping the connection active. */
i2c_status_t I2C_endTransmission(uint8_t i2c_channel, bool stop)
{
  i2c_status_t status = I2C_OTHER_ERROR;

  if (i2c_channel == 0)
  {
    status = (i2c_status_t)Wire.endTransmission(stop);
  }
  else if (i2c_channel == 1)
  {
    status = (i2c_status_t)Wire1.endTransmission(stop);
  }
  
  return status;
}

