# Gen6 Sample Code

Copyright (c) 2026 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

## Overview

This directory contains sample code (C and C++) that demonstrates Cirque's
Gen6 touchpad and CustomMeas solutions.

These projects run on the Cirque Blue Board, PCB# 02-000658-00 Rev E01 or later, with a 
Teensy 4.0 on-board. If you have a prior version of that board (Rev C03 or earlier)
see the tag "Teensy3.2_Final" for the code.

## Which project should I use?

| I want to... | Project | Arduino Tools > USB Type |
|---|---|---|
| See touch data, reports and settings in the Serial Monitor (best for evaluating and debugging) | CirqueGen6Demo | Serial |
| Move the PC's cursor: the Teensy converts touch data into a USB mouse and keyboard (works on any OS, no gestures) | CirqueGen6Demo_USB_Mouse | Serial + Keyboard + Mouse + Joystick |
| Use the touchpad as a real Precision Touchpad: the Teensy passes the touchpad's HID interface through to USB, so the PC's touchpad driver provides the cursor, tapping and multi-finger gestures | CirqueGen6Demo_USB_PTP | Serial |
| Read raw capacitive measurements from a device running the CustomMeas firmware | CirqueCustomMeasDemo | Serial |

To load new firmware through the Gen6 bootloader, see Gen6_BL_Demo (up a directory level).

"PTP/Absolute mode" in CirqueGen6Demo and CirqueGen6Demo_USB_Mouse only changes the format of the
touchpad's reports. Only CirqueGen6Demo_USB_PTP makes the PC see a touchpad.

CirqueGen6Demo_USB_Mouse was previously named CirqueGen6Demo_USB.

All of these projects need the Arduino libraries in the libraries directory (up a directory
level): Cirque (Gen6 touchpads and CustomMeas) and Teensy4_I2C_CRQMods (I2C on the Teensy 4.0).

Code description and installation instructions are at:

  www.cirque.com/documentation
	
  * GP-AN-251013 Gen6 Sample Code

