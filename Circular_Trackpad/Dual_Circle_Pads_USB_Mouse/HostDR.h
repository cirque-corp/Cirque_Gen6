#ifndef HostDR_H
#define HostDR_H

// Copyright (c) 2018 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license
/** @file */
#include "Arduino.h"

#include "Project_Config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DR0_MASK (0x01)
#define DR1_MASK (0x02)

/** Required Host_DR API - The touch system requires the following Host_DR
	functionality: */

/************************************************************/
/************************************************************/
/********************  PUBLIC FUNCTIONS *********************/
void HostDR_init(void);

uint8_t HostDR_pinState(void);

#ifdef __cplusplus
}
#endif

#endif
