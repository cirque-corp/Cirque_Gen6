#ifndef API_HARDWARE_H
#define API_HARDWARE_H

// Copyright (c) 2023 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

/** This code controls the extra hardware functions provided
	by the 02-000620-00 RevA01 Glidepoint Dev Kit board 
	(Red Soldermask version) */

#include <stdint.h>
#include <Arduino.h>
#include "Project_Config.h"
// #include "INA219.h"
	
#ifdef __cplusplus
extern "C" {
#endif

// Demo board hardware definition
// Power supply control
#define V_SEL_PIN  (1)

// Button injection control
#define BTN1_PIN      (3)
#define BTN2_PIN      (4)
#define BTN3_PIN      (5)

// Haptic control
#define HAPTIC_PIN    (6)
// "Scope Trigger lines"
#define SCOPE1_PIN    (7)
#define SCOPE2_PIN    (8)

// LED Pins
#define LED1_PIN      (20)
#define LED2_PIN      (21)

/** These functions must be implemented for the API_C2 to work. */
/************************************************************/
/************************************************************/
/********************  PUBLIC FUNCTIONS *********************/
void API_Hardware_init(void);

void API_Hardware_delay(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif

