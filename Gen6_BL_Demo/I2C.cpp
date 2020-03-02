// Copyright (c) 2018 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include <i2c_t3.h>
#include "I2C.h"

#if I2C_TX_BUFFER_LENGTH < 1026
#error I2C_TX_BUFFER_LENGTH must be at least 1026 for I2C_HID_Serial to work correctly. Go to \Program Files (x86)\Arduino\hardware\teensy\avr\libraries\i2c_t3\i2c_t3.h and change I2C_TX_BUFFER_LENGTH to 1026.
#endif

#if I2C_RX_BUFFER_LENGTH < 1026
#error I2C_RX_BUFFER_LENGTH must be at least 1026 for I2C_HID_Serial to work correctly. Go to \Program Files (x86)\Arduino\hardware\teensy\avr\libraries\i2c_t3\i2c_t3.h and change I2C_RX_BUFFER_LENGTH to 1026.
#endif


/************************************************************/
/************************************************************/
/********************  PUBLIC FUNCTIONS *********************/

/** Set the Arduino as a master if no address is given and sets clock frequency. */
void I2C_init(uint32_t clockFrequency)
{
  Wire.begin();                   // Set the arduino as master.
  Wire.setClock(clockFrequency);  // call .setClock after .begin
}

/** request the number of bytes specified by "count" from the given slave address
 * specified by "address". After transfer of data, set boolean "stop" as true to 
 * release the line. false will keep the line busy to send a restart. */
void I2C_request(int16_t address, int16_t count, bool stop)
{
  Wire.requestFrom(address, count, (i2c_stop)stop);
}

/** Returns the number of bytes available for reading. */
uint16_t I2C_available()
{
  return Wire.available();
}

/** returns the next byte. (Reads a byte that was transmitted from slave 
	device to a master). */
uint8_t I2C_read()
{
  return Wire.read();
}

/** returns the number of bytes written. Writes data from a slave device 
	from a request form a master. */
void I2C_write(uint8_t data)
{
  Wire.write(data);
}

/** Begins the transmission to a I2C slave device with the given address. */
void I2C_beginTransmission(uint8_t address)
{
  Wire.beginTransmission(address);
}

/** Ends the transmission to a slave deivce that was begun by the begin transmission. 
	Boolean "stop" if true, sends a stop condiction, releasing the bus. If false, 
	sends a restart request, keeping the connection active. */
void I2C_endTransmission(bool stop)
{
  Wire.endTransmission(stop);
}
