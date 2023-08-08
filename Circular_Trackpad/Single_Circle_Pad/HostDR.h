#ifndef HostDR_H
#define HostDR_H

// Copyright (c) 2018 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license
/** @file */
#include "Arduino.h"

#include "Project_Config.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Required Host_DR API - The touch system requires the following Host_DR
	functionality: */

/************************************************************/
/************************************************************/
/********************  PUBLIC FUNCTIONS *********************/
void HostDR_init(void);

bool HostDR_pinState(void);

#ifdef __cplusplus
}
#endif

#endif
