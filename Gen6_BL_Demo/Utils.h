// Copyright (c) 2019 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

uint8_t Checksum8(const uint8_t* buffer, uint16_t length);
uint16_t Fletcher16(uint8_t const* data, uint32_t bytes);
uint16_t Fletcher16_Continue(uint8_t const* data, uint32_t bytes, uint16_t previousChecksum);
uint32_t Fletcher_32(uint8_t const* data, uint32_t bytes);

#endif //__UTILS_H__

#ifdef __cplusplus
}
#endif
