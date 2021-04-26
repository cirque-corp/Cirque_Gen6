/*
 * Borax_BL_I2C_Commands.c
 *
 * Copyright (c) 2021 Cirque Corp. Restrictions apply.
 * See: www.cirque.com/sw-license
 */

#include <stdint.h>

#include "I2C.h"
#include "Fletcher32.h"
#include "Fletcher16.h"

#include "API_C3_I2C_Commands.h"

#define BL_REPORT_CMD_REG  (0x0005)
#define BL_REPORT_SET_FEAT (0x0337)
#define BL_REPORT_GET_FEAT (0x0237)
#define BL_REPORT_DATA_REG (0x0006)

#define CMD_WRITE     (0x00)
#define CMD_FLUSH     (0x01)
#define CMD_VALIDATE  (0x02)
#define CMD_RESET     (0x03)
#define CMD_FORMAT_IMG  (0x04)
#define CMD_FORMAT_REG  (0x05)
#define CMD_INVOKE_BL   (0x06)
#define CMD_WRITE_MEM   (0x07)
#define CMD_READ_MEM    (0x08)

#define DUMMY_BYTE  (0xDB)

/**** Private Functions  ****/
static void I2C_write_16(uint16_t data)
{
    I2C_write((uint8_t)(data >> 0));
    I2C_write((uint8_t)(data >> 8));
}

static void I2C_write_32(uint32_t data)
{
    I2C_write((uint8_t)(data >> 0));
    I2C_write((uint8_t)(data >> 8));
    I2C_write((uint8_t)(data >> 16));
    I2C_write((uint8_t)(data >> 24));
}

static void I2C_write_SetFeatureReport(uint8_t cmd)
{
    I2C_write_16(BL_REPORT_CMD_REG);
    I2C_write_16(BL_REPORT_SET_FEAT);
    I2C_write_16(BL_REPORT_DATA_REG);
    I2C_write_16(BL_REPORT_LEN);
    I2C_write(BL_REPORT_ID);
    I2C_write(cmd);
}

static void I2C_write_GetFeatureReport(void)
{
    I2C_write_16(BL_REPORT_CMD_REG);
    I2C_write_16(BL_REPORT_GET_FEAT);
    I2C_write_16(BL_REPORT_DATA_REG);
}

static void I2C_write_DummyBytes(uint16_t count)
{
    uint16_t i;

    for(i = 0; i < count; i++)
    {
        I2C_write(DUMMY_BYTE);
    }
}

/**** Public Functions  ****/

void BL_cmd_write(uint32_t offset, uint32_t numBytes, const uint8_t * dataPtr)
{
    uint32_t i;

    I2C_beginTransmission(I2C_SLAVE_ADDR);
    I2C_write_SetFeatureReport(CMD_WRITE);
    I2C_write_32(offset);
    I2C_write_32(numBytes);

    for(i = 0; i < numBytes; i++)
    {
        I2C_write(dataPtr[i]);
    }

    I2C_write_DummyBytes(BL_REPORT_LEN - WRITE_CMD_HEADER_SIZE - numBytes);
    I2C_endTransmission(true);
}

void BL_cmd_flush(void)
{
    I2C_beginTransmission(I2C_SLAVE_ADDR);
    I2C_write_SetFeatureReport(CMD_FLUSH);
    I2C_write_DummyBytes(BL_REPORT_LEN - FLUSH_CMD_HEADER_SIZE);

    I2C_endTransmission(true);
}

void BL_cmd_validate(uint8_t validationType)
{
    I2C_beginTransmission(I2C_SLAVE_ADDR);
    I2C_write_SetFeatureReport(CMD_VALIDATE);
    I2C_write(validationType);
    I2C_write_DummyBytes(BL_REPORT_LEN - VALIDATE_CMD_HEADER_SIZE);
    I2C_endTransmission(true);
}

void BL_cmd_reset(void)
{
    I2C_beginTransmission(I2C_SLAVE_ADDR);
    I2C_write_SetFeatureReport(CMD_RESET);
    I2C_write_DummyBytes(BL_REPORT_LEN - RESET_CMD_HEADER_SIZE);
    I2C_endTransmission(true);
}


void BL_cmd_format_image(
        uint8_t imageType,
        uint8_t numRegions,
        uint32_t entryPoint)
{
    I2C_beginTransmission(I2C_SLAVE_ADDR);
    I2C_write_SetFeatureReport(CMD_FORMAT_IMG);
    I2C_write(imageType);
    I2C_write(numRegions);
    I2C_write_32(entryPoint);
    I2C_write_16(HID_DESC_ADDR);
    I2C_write(I2C_SLAVE_ADDR);
    I2C_write(BL_REPORT_ID);
    I2C_write_DummyBytes(BL_REPORT_LEN - FORMAT_IMG_CMD_HEADER_SIZE);
    I2C_endTransmission(true);
}

void BL_cmd_format_region(uint8_t regionNumber, uint32_t regionOffset, uint32_t regionSize, uint32_t regionChecksum)
{
    I2C_beginTransmission(I2C_SLAVE_ADDR);
    I2C_write_SetFeatureReport(CMD_FORMAT_REG);
    I2C_write(regionNumber);
    I2C_write_32(regionOffset);
    I2C_write_32(regionSize);
    I2C_write_32(regionChecksum);
    I2C_write_DummyBytes(BL_REPORT_LEN - FORMAT_REGION_CMD_HEADER_SIZE);

    I2C_endTransmission(true);
}

void BL_cmd_invoke_bootloader(void)
{
    I2C_beginTransmission(I2C_SLAVE_ADDR);
    I2C_write_SetFeatureReport(CMD_INVOKE_BL);
    I2C_write_DummyBytes(BL_REPORT_LEN - INVOKE_BOOTLOADER_CMD_HEADER_SIZE);
    I2C_endTransmission(true);
}

void BL_cmd_write_memory(uint32_t offset, uint16_t numBytes, uint8_t * dataPtr)
{
    uint32_t i;
    uint16_t checksum;
    uint8_t preamble[7] =
    {
        CMD_WRITE_MEM,
        (uint8_t)(offset >> 0),
        (uint8_t)(offset >> 8),
        (uint8_t)(offset >> 16),
        (uint8_t)(offset >> 14),
        (uint8_t)(numBytes >> 0),
        (uint8_t)(numBytes >> 8)
    };

    I2C_beginTransmission(I2C_SLAVE_ADDR);
    I2C_write_SetFeatureReport(CMD_WRITE_MEM);
    I2C_write_32(offset);
    I2C_write_16(numBytes);

    for(i = 0; i < numBytes; i++)
    {
        I2C_write(dataPtr[i]);
    }

    checksum = Fletcher16(preamble, 7);
    checksum = Fletcher16_Continue(dataPtr, numBytes, checksum);
    I2C_write_16(checksum);
    I2C_write_DummyBytes(BL_REPORT_LEN - WRITE_MEMORY_CMD_HEADER_SIZE - numBytes);

    I2C_endTransmission(true);
}


//returns
// '1': numBytes exceeds MAX_READ_DATA_SIZE
// '0': success
uint16_t BL_request_read(uint32_t offset, uint16_t numBytes)
{
    if (numBytes > MAX_READ_DATA_SIZE)
    {
        return 1;
    }

    I2C_beginTransmission(I2C_SLAVE_ADDR);
    I2C_write_SetFeatureReport(CMD_READ_MEM);
    I2C_write_32(offset);
    I2C_write_16(numBytes);
    I2C_write_DummyBytes(BL_REPORT_LEN - READ_MEMORY_CMD_HEADER_SIZE);
    I2C_endTransmission(true);

    return 0;
}


void BL_read(uint8_t *data)
{
    uint16_t i;
    I2C_beginTransmission(I2C_SLAVE_ADDR);
    I2C_write_GetFeatureReport();
    I2C_endTransmission(false);
    I2C_request(I2C_SLAVE_ADDR, BL_REPORT_LEN, true);

    i = 0;
    while(I2C_available() && (i < BL_REPORT_LEN))
    {
        data[i++] = I2C_read();
    }
}
