# Cirque Touchpad Sample Code

Copyright (c) 2026 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

## Overview

These projects show how to interface with Cirque touchpads. These demos are for the GlidePoint Development Kit, the Red Board, 02-000620-00 Rev A02 or later. This kit was originally designed for the Circle Sensor Dev Kit but has since been expanded for all Cirque CMI Standard Touchpads.
The dev kit supports up to two touchpads being used at the same time. Each of these projects requires the Teensy4_i2c_CRQMods library to be in the Arduino/libraries directory.

## Which project should I use?

| I want to... | One touchpad | Two touchpads | Arduino Tools > USB Type |
|---|---|---|---|
| See touch data, reports and settings in the Serial Monitor (best for evaluating and debugging) | Single_Touchpad | Dual_Touchpads | Serial |
| Move the PC's cursor: the Teensy converts touch data into a USB mouse (works on any OS, no gestures) | Single_Touchpad_USB_Mouse | Dual_Touchpads_USB_Mouse | Serial + Keyboard + Mouse + Joystick |
| Use the touchpad as a real Precision Touchpad: the Teensy passes the touchpad's HID interface through to USB, so the PC's touchpad driver provides the cursor, tapping and multi-finger gestures | Single_Touchpad_USB_PTP | Dual_Touchpads_USB_PTP | Serial |

Single_ projects use Touchpad #1 only; Dual_ projects use both touchpad connectors.

"PTP/Absolute mode" in the Serial Monitor and USB mouse projects only changes the format of the
touchpad's reports. Only the _USB_PTP projects make the PC see a touchpad.

## The Code

The code is arranged into these directories. 

* Single_Touchpad - only Touchpad #1 is used. It reports information to a terminal window.
* Dual_Touchpads - both touchpads are used and they report information to a terminal window.
* Single_Touchpad_USB_Mouse - only Touchpad #1 is used. The Teensy 4.0 enumerates as a mouse and the cursor on the host system moves.
* Dual_Touchpads_USB_Mouse - both touchpads are used. The Teensy 4.0 enumerates as a mouse and the cursor on the host system moves.
* Single_Touchpad_USB_PTP - only Touchpad #1 is used. The Teensy 4.0 enumerates as a USB Precision Touchpad, so the host's own touchpad driver provides the cursor and multi-finger gestures.
* Dual_Touchpads_USB_PTP - both touchpads are used. The Teensy 4.0 enumerates as two USB Precision Touchpads, one per touchpad. Not yet tested on hardware with two touchpads.

