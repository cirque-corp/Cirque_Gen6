#ifdef __cplusplus
extern "C" {
#endif

#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#include <stdbool.h>
#include <stdint.h>

#include "I2C.h"
#include "USB_Serial.h"

uint32_t HW_micros();

/**
* Initializes GPIO pin connected to DR as an input
*/
void HW_init();

/**
* Reads GPIO pin connected to device data-ready (DR) pin
* @return Returns true if DR is asserted (active-low)
*/
bool HW_DR_asserted();

void HW_AssertDbg();
void HW_DeAssertDbg();

#endif  // _HARDWARE_H_

#ifdef __cplusplus
}
#endif
