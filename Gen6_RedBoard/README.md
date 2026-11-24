# Cirque Touchpad Sample Code

Copyright (c) 2026 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

## Overview

These projects show how to interface with Cirque touchpads. These demos are for the GlidePoint Development Kit, the Red Board, 02-000620-00 Rev A02 or later. This kit was originally designed for the Circle Sensor Dev Kit but has since been expanded for all Cirque CMI Standard Touchpads.
The dev kit supports up to two touchpads being used at the same time. Each of these projects requires the Teensy4_i2c_CRQMods library to be in the Arduino/libraries directory.

## Which project should I use?

Every project works with one or two touchpads, on either connector (Touchpad #1 or Touchpad #2).
At start-up the Teensy checks both connectors and only uses the touchpads that answer.

| I want to... | Project | Arduino Tools > USB Type |
|---|---|---|
| Have my microcontroller read and handle the touchpad directly (shown in the Serial Monitor): start here if the touchpad goes into your own embedded product. Also best for evaluating settings and debugging | Touchpads_Embedded | Serial |
| Move the PC's cursor: the Teensy converts touch data into a USB mouse (works on any OS, no gestures) | Touchpads_USB_Mouse | Serial + Keyboard + Mouse + Joystick |
| Use the touchpad as a real Precision Touchpad: the Teensy passes the touchpad's HID interface through to USB, so the PC's touchpad driver provides the cursor, tapping and multi-finger gestures | Touchpads_USB_PTP | Serial |

"PTP/Absolute mode" in the Embedded and USB mouse projects only changes the format of the
touchpad's reports. Only Touchpads_USB_PTP makes the PC see a touchpad.

In the Serial Monitor, touchpad commands go to every connected touchpad, or add 1 or 2 to send a
command to one touchpad only (for example 'a2'). Output lines start with [TP1] or [TP2].

## The Code

The code is arranged into these directories. 

* Touchpads_Embedded - the Teensy reads the touchpads and reports information to the Serial Monitor, the way the microcontroller in your own product would.
* Touchpads_USB_Mouse - the Teensy 4.0 enumerates as a mouse and keyboard, and the cursor on the host system moves. Both touchpads move the same cursor.
* Touchpads_USB_PTP - the Teensy 4.0 enumerates as one USB Precision Touchpad per touchpad found, so the host's own touchpad driver provides the cursor and multi-finger gestures.

Each project's README explains how to use one touchpad and how to port the code to your own microcontroller.

### Previously named

This folder was previously named Gen6_Touchpads_RedBoard.

The separate single- and dual-touchpad projects were combined:

* Single_Touchpad, Dual_Touchpads -> Touchpads_Embedded
* Single_Touchpad_USB_Mouse, Dual_Touchpads_USB_Mouse -> Touchpads_USB_Mouse
* Single_Touchpad_USB_PTP, Dual_Touchpads_USB_PTP -> Touchpads_USB_PTP
