// Copyright (c) 2019 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include <i2c_t3.h>
#include <stdint.h>

#include "Borax_BL.h"

// --------------------
// The tests

#include "FW0202.h"
#include "FW9902.h"

uint32_t address_0202 = 0x0002d420;
uint32_t address_9902 = 0x0002d420;

static void PrintProgramErrors(uint16_t error)
{
    switch (error)
    {
        case BLProgErr_OK:
            Serial.println("Firmware programming successful!\n");
            break;
        case BLProgErr_InvokeBootloader:
            Serial.println("Error during InvokeBootloader\n");
            break;
        case BLProgErr_FormatImage:
            Serial.println("Error during FormatImage\n");
            break;
        case BLProgErr_FormatRegion:
            Serial.println("Error during FormatRegion\n");
            break;
        case BLProgErr_WriteImage:
            Serial.println("Error during WriteImage\n");
            break;
        case BLProgErr_Flush:
            Serial.println("Error during Flush\n");
            break;
        case BLProgErr_Validate:
            Serial.println("Error during Validate\n");
            break;
        case BLProgErr_Reset:
            Serial.println("Error during Reset\n");
            break;
        default:
            Serial.println((String)"Error code: " + error + "\n");
            break;
    }
}


void Test_Gen6_Program_0202()
{
    uint16_t error;

    Serial.println("\nProgramming(02:22) will take about 15 seconds.\nPlease wait...");
    error = BL_program(fw_bin_0202, sizeof(fw_bin_0202), address_0202);
    PrintProgramErrors(error);
}


void Test_Gen6_Program_9902()
{
    uint16_t error;

    Serial.println("\nProgramming(99:02) will take about 15 seconds.\nPlease wait...");
    error = BL_program(fw_bin_9902, sizeof(fw_bin_9902), address_9902);
    PrintProgramErrors(error);
}


void Test_Gen6_get_status()
{
    bl_read_packet status;
    BL_get_status(&status);

    Serial.printf("\nStatus:\n");
    Serial.printf("Sentinel: 0x%04X", status.Sentinel);
    switch(status.Sentinel)
    {
        case 0x6D49:
            Serial.printf("(image)\n");
            break;
        case 0x6C42:
            Serial.printf("(bootloader)\n");
            break;
        default:
            Serial.printf("\n");
            break;
    }
    Serial.printf("Version: 0x%02X\n", status.Version);
    Serial.printf("Error: 0x%02X\n", status.LastError);
}


void Test_Gen6_get_hwid()
{
    uint8_t buffer[6];
    BL_cmd_read_memory(0x2000080A, 6, buffer);

    Serial.printf("\nHardware info:\n");
    Serial.printf("VID: 0x%04X\n", buffer[0] | (buffer[1]<<8));
    Serial.printf("PID: 0x%04X\n", buffer[2] | (buffer[3]<<8));
    Serial.printf("REV: 0x%04X\n", buffer[4] | (buffer[5]<<8));
}
