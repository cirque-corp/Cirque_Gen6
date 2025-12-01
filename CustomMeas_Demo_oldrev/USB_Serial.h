#ifdef __cplusplus
extern "C" {
#endif

#ifndef _USB_SERIAL_H_
#define _USB_SERIAL_H_

#include <stddef.h>
#include <stdint.h>

void USB_Serial_init(uint32_t baudRate);
uint32_t USB_Serial_available();
uint8_t USB_Serial_writeByte(uint8_t data);
size_t USB_Serial_writeBytes(uint8_t * data, size_t count);
void USB_Serial_flush();
uint8_t USB_Serial_readByte();

#endif  // _USB_SERIAL_H_

#ifdef __cplusplus
}
#endif
