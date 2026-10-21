# Cirque Touchpad Sample Code

Copyright (c) 2026 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

## Overview

These project show how to interface with Cirque touchpads. These demos are for the Glidepoint Development Kit, the Red Board, 02-000620-00 Rev A02 or later. This kit was originally designed for the Circle Sensor Dev Kit but has since been expanded for all Cirque CMI Standard Touchpads.
The dev kit supports up to two trackpads being used at the same time. Each of these projects requires the Teensy4_i2c_CRQMods library to be in the Arduino/libraries directory.

## The Code

The code is arranged into these directories. 

* Dual_Touchpads - both touchpads are used and they report information to a terminal window
* Dual_Touchpads_USB_Mouse - both touchpads are used. The Teensy 4.0 enumerates as a mouse and the cursor on the host system moves.
* Single_Touchpads - only Touchpad #1 is used. It reports information to a terminal window
* Single_Touchpads_USB_Mouse - only Touchpad #1 is used. The Teensy 4.0 enumerates as a mouse and the cursor on the host system moves.

