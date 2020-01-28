#ifdef __cplusplus
extern "C" {
#endif

#ifndef _API_EXT_MEM_ACCESS_H_
#define _API_EXT_MEM_ACCESS_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum
{
  MEM_SUCCESS = 0,
  MEM_BAD_CHECKSUM = 1,
  MEM_LENGTH_MISMATCH = 2,
} memStatus_t;

extern size_t apiExtMemAccessMaxTransfer;

bool API_ExtMemAccess_Init(uint8_t i2cAddr, uint32_t i2cBitRate, size_t maxTransferSize);
memStatus_t API_ExtMemAccess_ReadMemory(uint32_t registerAddress, uint8_t * data, uint16_t count);
void API_ExtMemAccess_WriteMemory(uint32_t registerAddress, uint8_t * data, uint8_t count);

#endif  // _API_EXT_MEM_ACCESS_H_

#ifdef __cplusplus
}
#endif
