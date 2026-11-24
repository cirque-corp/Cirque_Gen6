# Cirque Sample Code

Copyright (c) 2026 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

## Overview

This repository contains sample code (C and C++) that demonstrates Cirque's capacitive touch solutions. 
Many of these sample programs are Arduino projects for the Teensy 4.0.

## Installation

This entire repository can be placed directly into your Arduino projects directory. The contents of the libraries directory go in the Arduino/libraries directory.
As you open each .ino, remember to set the Board and Port to the correct values.

## Which project should I use?

First pick the folder for your development kit:

* **Red Board** (GlidePoint Development Kit, also known as the Circle Sensor Dev Kit, 02-000620-00 Rev A02 or later): `Gen6_Touchpads_RedBoard/`
* **Blue Board** (GlidePoint Gen-6 Development Kit, 02-000658-00 Rev E01 or later): `Gen6_BlueBoard/`, plus the bootloader demo in `Gen6_BL_Demo/`
* **Older boards with a Teensy 3.2**: check out the git tag `Teensy3.2_Final`, or see the `*_oldrev` folders

Then pick a project by what you want the PC to see:

| I want to... | Red Board | Blue Board | Arduino Tools > USB Type |
|---|---|---|---|
| See touch data, reports and settings in the Serial Monitor (best for evaluating and debugging) | `Single_Touchpad`, `Dual_Touchpads` | `CirqueGen6Demo` | Serial |
| Move the PC's cursor: the Teensy converts touch data into a USB mouse (works on any OS, no gestures) | `Single_Touchpad_USB_Mouse`, `Dual_Touchpads_USB_Mouse` | `CirqueGen6Demo_USB_Mouse` | Serial + Keyboard + Mouse + Joystick |
| Use the touchpad as a real Precision Touchpad: the Teensy passes the touchpad's HID interface through to USB, so the PC's touchpad driver provides the cursor, tapping and multi-finger gestures | `Single_Touchpad_USB_PTP`, `Dual_Touchpads_USB_PTP` | `CirqueGen6Demo_USB_PTP` | Serial |
| Read raw capacitive measurements (CustomMeas firmware) | - | `CirqueCustomMeasDemo` | Serial |
| Load new firmware through the Gen6 bootloader | - | `Gen6_BL_Demo` (top level) | Serial |

`Single_` projects use Touchpad #1 only; `Dual_` projects use both touchpad connectors.

"PTP/Absolute mode" in the Serial Monitor and USB mouse projects only changes the format of the
touchpad's reports. Only the `_USB_PTP` projects make the PC see a touchpad.

## The Code

The code is arranged into these directories. 

### Gen6_Touchpads_RedBoard

This contains the projects that are used with the GlidePoint Development Kit (sometimes known as the Circle Sensor Dev Kit), Red Board - 02-000620-00 Rev A02 or later.

### Gen6_BlueBoard

This contains the projects that are used with the GlidePoint Gen-6 Development Kit, Blue Board - 02-000658-00 Rev E01 or later.

### Gen6_BL_Demo

This contains a project that shows how to use the bootloader to update the firmware in a Gen6 device, Blue Board - 02-000658-00 Rev E01 or later.

### libraries

This contains commonly used code for many of the demos. The contents of this directory go in the Arduino/libraries directory.

### *_oldrev

These contain demo code meant for the older revs of the hardware (that feature Teensy3.2 parts).
The git tag `Teensy3.2_Final` has the last version of all the code for those boards.
