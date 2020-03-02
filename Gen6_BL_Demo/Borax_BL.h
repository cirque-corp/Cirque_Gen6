// Copyright (c) 2019 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _BORAX_BL_H_
#define _BORAX_BL_H_

#include <stdbool.h>
#include <stdint.h>

enum ErrorCodes
{
  NO_ERROR = 0,
  NOT_INITIALIZED = 1,
  SECTOR_OUT_OF_RANGE = 2,
  OFFSET_OUT_OF_RANGE = 3,
  NULL_POINTER = 4,
  TIMEOUT = 5,
  UNKNOWN_ERROR = 6,
  NO_RECENT_IMAGE = 7,
  ACCESS_VIOLATION = 8,
  PROTECTION_VIOLATION = 9,
  MISALIGNED_ADDRESS = 10,
  COMMAND_UNKNOWN = 11,
  CHECKSUM_MISMATCH = 12,
};

typedef struct
{
  uint16_t Sentinel;
  uint8_t Version;
  uint8_t LastError;
  uint8_t Flags;
  uint8_t AtomicWriteSize;
  uint8_t WriteDelay;
  uint8_t FormatDelay;
  uint32_t MemAddress;
  uint16_t NumBytes;
  uint8_t  Data[517];
  uint16_t Checksum;
} bl_status_t;

void BL_init(uint8_t i2cSlaveAddr);

void BL_cmd_write(uint32_t offset, uint32_t numBytes, const uint8_t * dataPtr);

void BL_cmd_flush();

void BL_cmd_validate(uint8_t validationType);

void BL_cmd_reset();

void BL_cmd_format_image(uint8_t imageType, uint8_t numRegions, uint32_t entryPoint, uint16_t hidDescriptorAddress, uint8_t i2cAddress, uint8_t reportID);

void BL_cmd_format_region(uint8_t regionNumber, uint32_t regionOffset, uint32_t regionSize, uint32_t regionChecksum);

void BL_cmd_invoke_bootloader();

void BL_cmd_write_memory(uint32_t offset, uint16_t numBytes, uint8_t * dataPtr);

void BL_cmd_read_memory(uint32_t offset, uint16_t numBytes, uint8_t * dataPtr);

bool BL_get_status(bl_status_t * statusPtr);

bool BL_program(const uint8_t * buf, uint32_t numBytes, uint32_t address);

#endif  // _BORAX_BL_H_

#ifdef __cplusplus
}
#endif
