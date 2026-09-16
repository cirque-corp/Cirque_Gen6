#ifndef I2C_H
#define I2C_H

// Copyright (c) 2019 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license
/** @file */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  I2C_SUCCESS = 0,
  I2C_BUFFER_OVERFLOW = 1,
  I2C_ADDRESS_NACKED = 2,
  I2C_DATA_NACKED = 3,
  I2C_OTHER_ERROR = 4
} i2c_status_t;

/** Required I2C API - The touch system requires the following 
	I2C functionality from the host: */

/************************************************************/
/************************************************************/
/********************  PUBLIC FUNCTIONS *********************/

void I2C_init(uint8_t i2c_channel, uint32_t clockFrequency);

uint32_t I2C_request(uint8_t i2c_channel, uint8_t address, uint32_t count, bool stop);

uint32_t I2C_available(uint8_t i2c_channel);

uint8_t I2C_read(uint8_t i2c_channel);

uint8_t I2C_readBytes(uint8_t i2c_channel, uint8_t * buffer, size_t count);

uint8_t I2C_write(uint8_t i2c_channel, uint8_t data);

void I2C_beginTransmission(uint8_t i2c_channel, uint8_t address);

i2c_status_t I2C_endTransmission(uint8_t i2c_channel, bool stop);

/************************************************************/
/************************************************************/
/******************** SPECIAL FUNCTIONS *********************/

bool I2C_setBufferLength(uint32_t requiredBuffLength);

#ifdef __cplusplus
}
#endif

#endif
