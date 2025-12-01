#include <Arduino.h>
#include "USB_Serial.h"

void USB_Serial_init(uint32_t baudRate)
{
	Serial.begin(baudRate);
}

uint32_t USB_Serial_available()
{
	return Serial.available();
}

uint8_t USB_Serial_writeByte(uint8_t data)
{
	return Serial.write(data);
}

size_t USB_Serial_writeBytes(uint8_t * data, size_t count)
{  
  return Serial.write(data, count);
}

void USB_Serial_flush()
{
  Serial.flush();
}

uint8_t USB_Serial_readByte()
{
	return Serial.read();
}
