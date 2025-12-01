// Copyright (c) 2019 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

//#include <i2c_t3.h>
#include "i2c_driver_wire.h"
#include <stdint.h>

#include "API_C3_BL.h"

// --------------------
// The tests

#include "FW_CustomMeas.h"  // Firmware #1
#include "FW_SpiderMeas.h"  // Firmware #0

// FW1 and FW0 are const arrays that are created using the open source SRecord project
// srec_cat inputfile.hex -intel -o header.h -C-Array arrayname
// You can direct this code to use some other hex file by changing these defines
#define FW1_NAME "CustomMeas"
#define FW1_BIN Oly1p3_CustomMeas_BL
#define FW1_BIN_SIZE sizeof(Oly1p3_CustomMeas_BL)
#define FW1_BASEADDRESS Oly1p3_CustomMeas_BL_start

#define FW0_NAME "SpiderMeas"
#define FW0_BIN Cirque_Oly1p2_SpiderMeas_BL
#define FW0_BIN_SIZE sizeof(Cirque_Oly1p2_SpiderMeas_BL)
#define FW0_BASEADDRESS Cirque_Oly1p2_SpiderMeas_BL_start

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


void Test_Gen6_Program_FW1()
{
    uint16_t error;

    Serial.print("\nProgramming ");
    Serial.print(FW1_NAME);
    Serial.println(", will take about 15 seconds.\nPlease wait...");
    error = BL_program(FW1_BIN, FW1_BIN_SIZE, FW1_BASEADDRESS);
    PrintProgramErrors(error);
}


void Test_Gen6_Program_FW0()
{
    uint16_t error;

    Serial.print("\nProgramming ");
    Serial.print(FW0_NAME);
    Serial.println(", will take about 15 seconds.\nPlease wait...");
    error = BL_program(FW0_BIN, FW0_BIN_SIZE, FW0_BASEADDRESS);
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
