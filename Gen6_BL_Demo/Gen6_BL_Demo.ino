// Copyright (c) 2019 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "Gen6_BL.h"
#include "I2C.h"
#include "Borax_BL.h"

#define PWR_EN_PIN 1
#define I2C_SLAVE_ADDR 0x2C

void setup()
{
    pinMode(PWR_EN_PIN, OUTPUT);
    digitalWrite(PWR_EN_PIN, LOW);
    delay(100);
    digitalWrite(PWR_EN_PIN, HIGH);
    delay(100);

    Serial.begin(115200);
    delay(1000);

    BL_init(I2C_SLAVE_ADDR);
    I2C_init(400000);

    PrintCommands();
}

void loop()
{
    if(Serial.available())
    {
        char rxChar = Serial.read();

        switch(rxChar)
        {
            case 'g':
                Test_Gen6_get_hwid();
                Serial.println("");
                break;
            case 's':
                Test_Gen6_get_status();
                Serial.println("");
                break;
            case '?':
            case 'h':
            case 'l':
                PrintCommands();
                break;
            case 'p':
                Serial.println("Programming(99:02) will take about 15 seconds.\nPlease wait...");
                Test_Gen6_Program_9902();
                break;
            case 'r':
                Serial.println("Programming(02:22) will take about 15 seconds.\nPlease wait...");
                Test_Gen6_Program_0202();
                break;
            case '\r':
                break;  // ignore carriage-return
            case '\n':
                break;  // ignore line-feed
            default:
                Serial.println("Invalid Command!");
                PrintCommands();
                break;
        }
    }
}

static void PrintCommands()
{
    Serial.println("Cirque Bootloader Host Application Demo");
    Serial.println("Supported Commands:");
    Serial.println("'g' - Get hardware ID");
    Serial.println("'s' - Get bootloader status");
    Serial.println("'p' - Program firmware 99:02");
    Serial.println("'r' - Program firmware 02:02");
    Serial.println("'l' - list commands\n");
}
