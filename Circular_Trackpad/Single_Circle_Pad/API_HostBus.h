#ifndef API_HOSTBUS_H
#define API_HOSTBUS_H

// Copyright (c) 2018 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license
/** @file */

#include "I2C.h"
#include "HostDR.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SUCCESS           0x00
#define BAD_CHECKSUM      0x01
#define LENGTH_MISMATCH   0x02
#define CIRQUE_I2C_ADDR 0x2A
#define ALPS_I2C_ADDR   0x2C

#define CIRQUE_HID_COMMAND_REGISTER 0x0005

typedef struct
{
    uint16_t wHIDDescLength;
    uint16_t bcdVersion;
    uint16_t wReportDescLength;
    uint16_t wReportDescRegister;
    uint16_t wInputRegister;
    uint16_t wMaxInputLength;
    uint16_t wOutputRegister;
    uint16_t wMaxOutputLength;
    uint16_t wCommandRegister;
    uint16_t wDataRegister;
    uint16_t wVendorID;
    uint16_t wProductID;
    uint16_t wVersionID;
    uint32_t Reserved;
} HIDDescriptor_t;

/************************************************************/
/************************************************************/
/********************  PUBLIC FUNCTIONS *********************/

/** @defgroup host_interface The I2C Host Interface
	The I2C Host Interface to a Cirque Touch System consists of three parts:
	1) Power supply
	2) I2C interface (SCL, SDA)
	3) Host Data Ready line (Host_DR)
	This section defines the operation of those three parts. */

void HB_init(int I2CFrequency, uint8_t I2CAddress);

bool HB_DR_Asserted(void);

void HB_readReport(uint8_t * packet, uint16_t readLength);

uint8_t HB_readExtendedMemory(uint32_t, uint8_t *, uint16_t);

void HB_writeExtendedMemory(uint32_t, uint8_t *, uint8_t);

void HB_HID_GetHidDescriptor(uint16_t HidDescAddr, HIDDescriptor_t * hidDescriptor);
void HB_HID_SetPower(bool powerOn);
void HB_HID_Reset(void);
void HB_HID_readRegister(uint16_t hidRegister, uint8_t * buffer, uint16_t readLength);
bool HB_HID_readReset(void);
void HB_HID_getFeatureReport(uint8_t reportID, uint16_t dataRegister, uint8_t *inputBuffer, uint16_t inputLength);
void HB_HID_setFeatureReport(uint8_t reportID, uint16_t dataRegister, uint16_t data);

#ifdef __cplusplus
}
#endif

#endif
