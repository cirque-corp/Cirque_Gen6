// Copyright (c) 2018 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "API_Hardware.h"
#include "Project_Config.h" // using PROJECT_I2C_FREQUENCY

/************************************************************/
/************************************************************/
/********************  GLOBAL VARIABLES *********************/

static const uint8_t LED_Pins[] PROGMEM = { LED1_PIN, LED2_PIN };
static const uint8_t NumberOfLeds PROGMEM = sizeof(LED_Pins) / sizeof(LED_Pins[0]);

/************************************************************/
/************************************************************/
/********************  PUBLIC FUNCTIONS *********************/
/** Initialize Hardware information */
void API_Hardware_init(void)
{
    pinMode(V_SEL_PIN, OUTPUT);
    
    /**  Outputs for Debug
		pinMode(SCOPE1_PIN, OUTPUT);
		pinMode(SCOPE2_PIN, OUTPUT);
    */

    pinMode(BTN1_PIN, INPUT);
    pinMode(BTN2_PIN, INPUT);
    pinMode(BTN3_PIN, INPUT);

    byte x;
    for (x = 0; x < NumberOfLeds; x++)
    {
        digitalWrite(LED_Pins[x], HIGH); // LED off
        pinMode(LED_Pins[x], OUTPUT);
    }  


    API_Hardware_PowerOff(); 

    INA219_init(0x40, PROJECT_I2C_FREQUENCY);      // Slave address is 0x40
    INA219_config(CONFIG__FS_RANGE_16V | CONFIG__SHUNT_PGA_DIV8);
}

/** Power Up the Hardware */
void API_Hardware_PowerOn(void)
{
    digitalWrite(V_SEL_PIN, HIGH);
}

/** Power off the Hardware */
void API_Hardware_PowerOff(void)
{
    digitalWrite(V_SEL_PIN, LOW);
}

/** Delay for ms milliseconds. */
void API_Hardware_delay(uint32_t ms)
{
    delay(ms); //wraps the internal Arduino hardware delay
}
