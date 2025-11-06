#ifndef DATA_UTILS_H
#define DATA_UTILS_H

// Copyright (c) 2025 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

uint8_t calculateChecksum(uint8_t * data, uint16_t length);

#ifdef __cplusplus
}
#endif

#endif // DATA_UTILS_H
