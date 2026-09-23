// Copyright (c) 2018 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "HostDR.h"
#include "API_C3.h"

/************************************************************/
/************************************************************/
/********************  PUBLIC FUNCTIONS *********************/

/** Initialize the Host_DR line as an input.  No pull up is required as the line
	is driven high and low by the touch system. */
void HostDR_init()
{
	pinMode(CONFIG_HOST_DR0_PIN, INPUT_PULLUP);
	pinMode(CONFIG_HOST_DR1_PIN, INPUT_PULLUP);
}

/** Read the Host_DR lines' states; either 0 or 1. */
uint8_t HostDR_pinState(void)
{
  uint8_t drMask = 0;

	if (digitalRead(CONFIG_HOST_DR0_PIN) == LOW)
  {
    drMask |= DR0_MASK;
  }

	if (digitalRead(CONFIG_HOST_DR1_PIN) == LOW)
  {
    drMask |= DR1_MASK;
  }

  return drMask;
}

uint8_t HostDR_readDRViaI2C(void)
{
  uint8_t drMask = 0;

  if (API_C3_readRegister(0, REG_DR_STATUS)) // this register is read-only and will return the state of the DR line
  {
    drMask |= DR0_MASK;
  } 

  if (API_C3_readRegister(1, REG_DR_STATUS))
  {
    drMask |= DR1_MASK;
  }

  return drMask;
}
