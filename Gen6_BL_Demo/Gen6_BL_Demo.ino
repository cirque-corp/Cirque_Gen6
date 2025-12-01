// Copyright (c) 2019 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

// You must select something containing "Serial" from the "Tools > USB Type" menu.

#include "API_C3_BL.h"
#include "Gen6_BL.h"
#include "I2C.h"

#define PWR_EN_PIN 1

void setup()
{
    //Power cycle the Touchpad
    pinMode(PWR_EN_PIN, OUTPUT);
    digitalWrite(PWR_EN_PIN, LOW);
    delay(100);
    digitalWrite(PWR_EN_PIN, HIGH);
    delay(100);

    Serial.begin(115200);
    delay(1000);

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
                break;
            case 's':
                Test_Gen6_get_status();
                break;
            case '?':
            case 'h':
            case 'l':
                PrintCommands();
                break;
            case 'p':
                Test_Gen6_Program_FW0();
                break;
            case 'r':
                Test_Gen6_Program_FW1();
                break;
            case '\r':
                break;  // ignore carriage-return
            case '\n':
                break;  // ignore line-feed
            default:
                Serial.println(F("Invalid Command!"));
                PrintCommands();
                break;
        }
    }
}

static void PrintCommands()
{
    Serial.println(F("Cirque Bootloader Host Application Demo"));
    Serial.println(F("Supported Commands:"));
    Serial.println(F("'g' - Get hardware ID"));
    Serial.println(F("'s' - Get bootloader status"));
    Serial.println(F("'p' - Program firmware FW0"));
    Serial.println(F("'r' - Program firmware FW1"));
    Serial.println(F("'l' - list commands\n"));
}
