// Copyright (c) 2019 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include <i2c_t3.h>

#include "Borax_BL.h"

// --------------------
// The tests

#include "FW0202.h"
#include "FW9902.h"

uint32_t address_0202 = 0x0002d420;
uint32_t address_9902 = 0x0002d420;

void Test_Gen6_Program_0202()
{
    bool res = BL_program(fw_bin_0202, sizeof(fw_bin_0202), address_0202);
    if( res )
    {
        Serial.println("Firmware programming successful!\n");
    }
    else
    {
        Serial.println("Firmware programming failed!\n");
    }
}

void Test_Gen6_Program_9902()
{
    bool res = BL_program(fw_bin_9902, sizeof(fw_bin_9902), address_9902);
    if( res )
    {
        Serial.println("Firmware programming successful!\n");
    }
    else
    {
        Serial.println("Firmware programming failed!\n");
    }
}

void Test_Gen6_get_status()
{
    bl_read_packet status;
    BL_get_status(&status);

    Serial.printf("Sentinel: 0x%04X\n", status.Sentinel);
    Serial.printf("Version: 0x%02X\n", status.Version);
    Serial.printf("Error: 0x%02X\n", status.LastError);
}

void Test_Gen6_get_hwid()
{
    uint8_t buffer[6];
    BL_cmd_read_memory(0x2000080A, 6, buffer);

    Serial.printf("VID: 0x%04X\n", buffer[0] | (buffer[1]<<8));
    Serial.printf("PID: 0x%04X\n", buffer[2] | (buffer[3]<<8));
    Serial.printf("REV: 0x%04X\n", buffer[4] | (buffer[5]<<8));
}
