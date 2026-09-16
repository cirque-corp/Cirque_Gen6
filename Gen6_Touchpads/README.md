# Cirque Circular Trackpad Sample Code

Copyright (c) 2025 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

## Overview

These project show how to use the circular trackpads. These demos are for the Glidepoint Development Kit, the Red Board, 02-000620-00 Rev A02 or later.
The dev kit supports up to two trackpads being used at the same time. Each of these projects requires the Teensy4_i2c_CRQMods library to be in the Arduino/libraries directory.

## The Code

The code is arranged into these directories. 

* Dual_Circle_Pads - both trackpads are used and they report information to a terminal window
* Dual_Circle_Pads_USB_Mouse - both trackpads are used. The Teensy 4.0 enumerates as a mouse and the cursor on the host system moves.
* Single_Circle_Pads - only Trackpad #1 is used. It reports information to a terminal window
* Single_Circle_Pads_USB_Mouse - only Trackpad #1 is used. The Teensy 4.0 enumerates as a mouse and the cursor on the host system moves.

