// Copyright (c) 2018 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "HostDR.h"

/************************************************************/
/************************************************************/
/********************  PUBLIC FUNCTIONS *********************/

/** Initialize the Host_DR line as an input.  No pull up is required as the line
	is driven high and low by the touch system. */
void HostDR_init(void)
{
	pinMode(CONFIG_HOST_DR0_PIN, INPUT);
}

/** Read the Host_DR line's state; either 0 or 1. */
bool HostDR_pinState(void)
{
	return digitalRead(CONFIG_HOST_DR0_PIN);
}


