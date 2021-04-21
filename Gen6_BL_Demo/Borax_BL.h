// Copyright (c) 2019 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#ifndef _BORAX_BL_H_
#define _BORAX_BL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "Borax_BL_I2C_Commands.h"

#define STATUS_BUSY_BIT 0x08
#define STATUS_VALID_IMAGE 0x10

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

enum BLProgramErrors
{
    BLProgErr_OK = 0,
    BLProgErr_InvokeBootloader = 101,
    BLProgErr_FormatImage = 102,
    BLProgErr_FormatRegion = 103,
    BLProgErr_WriteImage = 104,
    BLProgErr_Flush = 105,
    BLProgErr_Validate = 106,
    BLProgErr_Reset = 107,
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
    uint8_t  Data[MAX_READ_DATA_SIZE];
    uint16_t Checksum;
} bl_read_packet;

bool BL_get_status(bl_read_packet *status);
uint16_t BL_program(const uint8_t * buf, uint32_t numBytes, uint32_t address);
uint16_t BL_cmd_read_memory(uint32_t offset, uint16_t numBytes, uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif  // _BORAX_BL_H_
