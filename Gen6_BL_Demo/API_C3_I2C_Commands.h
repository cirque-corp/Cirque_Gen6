/*
 * API_C3_I2C_Commands.h
 *
 * Copyright (c) 2021 Cirque Corp. Restrictions apply.
 * See: www.cirque.com/sw-license
 */

#ifndef BORAX_BL_I2C_COMMAND_H
#define BORAX_BL_I2C_COMMAND_H

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_SLAVE_ADDR (0x2C)
#define BL_REPORT_ID  (0x07)
#define HID_DESC_ADDR (0x0020)

// must be even, better if multiple of 4 (520 / 4 = 130)
#define MAX_DATA_PAYLOAD_SIZE 520
#define BL_REPORT_LEN (0x0215)  // Always 533

//NOTE: Size of header includes:
// length: 2 bytes
// ReportID : 1 byte

// BL_cmd_write = 0x00: 1 byte
// offset: 4 bytes
// NumBytes: 4 bytes
#define WRITE_CMD_HEADER_SIZE (12)

// BL_cmd_flush = 0x01: 1 byte
#define FLUSH_CMD_HEADER_SIZE (4)

// BL_cmd_validate = 0x02: 1 byte
// validation type: 1 byte
#define VALIDATE_CMD_HEADER_SIZE (5)

// BL_cmd_reset = 0x03: 1 byte
#define RESET_CMD_HEADER_SIZE (4)

// BL_cmd_format_image = 0x04: 1 byte
// ImageType: 1 byte
// NumRegions: 1 byte
// EntryPoint: 4 bytes
// HID Desc Address: 2 bytes
// I2C Address: 1 byte
// ReportID: 1 byte
#define FORMAT_IMG_CMD_HEADER_SIZE (14)

// BL_cmd_format_region = 0x05: 1 byte
// region number: 1 byte
// RegionOffset: 4 bytes
// RegionSize: 4 bytes
// RegionChecksum: 4 bytes
#define FORMAT_REGION_CMD_HEADER_SIZE (17)

// BL_cmd_invoke_bootloader = 0x06: 1 byte
#define INVOKE_BOOTLOADER_CMD_HEADER_SIZE (4)

// BL_cmd_write_memory = 0x07: 1 byte
// StartAddress: 4 bytes
// NumBytes: 2 bytes
// data: 0-521 bytes (not counted here)
// checksum: 2 bytes
#define WRITE_MEMORY_CMD_HEADER_SIZE (12)

// BL_cmd_read_memory = 0x08: 1 byte
// StartAddress: 4 bytes
// NumBytes: 2 bytes
#define READ_MEMORY_CMD_HEADER_SIZE (10)

//When reading data:
// status:17 bytes
// data:0-514 bytes
// checksum: 2 bytes
#define READ_STATUS_SIZE (17)
#define READ_CHECKSUM_SIZE (2)
#define MAX_READ_DATA_SIZE (BL_REPORT_LEN - READ_STATUS_SIZE - READ_CHECKSUM_SIZE)



void BL_cmd_write(uint32_t offset, uint32_t numBytes, const uint8_t * dataPtr);
void BL_cmd_flush(void);
void BL_cmd_validate(uint8_t validationType);
void BL_cmd_reset(void);
void BL_cmd_format_image(
        uint8_t imageType,
        uint8_t numRegions,
        uint32_t entryPoint);
void BL_cmd_format_region(
        uint8_t regionNumber,
        uint32_t regionOffset,
        uint32_t regionSize,
        uint32_t regionChecksum);
void BL_cmd_invoke_bootloader(void);
void BL_cmd_write_memory(uint32_t offset, uint16_t numBytes, uint8_t * dataPtr);
uint16_t BL_request_read(uint32_t offset, uint16_t numBytes);
void BL_read(uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif //BORAX_BL_I2C_COMMAND_H
