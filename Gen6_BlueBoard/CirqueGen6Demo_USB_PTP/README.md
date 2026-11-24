# Cirque Gen6 Demo - USB Precision Touchpad

Copyright (c) 2026 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

## Overview

This Arduino project uses the Cirque library and makes the touchpad appear on a PC as a
USB Precision Touchpad (PTP), with the Teensy 4.0 acting as a USB-to-I2C bridge. The PC sees a
normal touchpad, so you get the cursor, tap-to-click and multi-finger gestures (two-finger
scroll, pinch zoom, three- and four-finger swipes) through the PC's own touchpad driver.

This code requires the Teensy 4.0 have its USB set to "Serial" (Tools > USB Type > "Serial").
The sketch adds the touchpad interface to that USB type itself; selecting any other USB type
stops the build with an error message.

It needs the same libraries as the other Blue Board projects (Cirque and Teensy4_I2C_CRQMods);
no other libraries are required.

NOTE: The Arduino IDE requires projects to be located in a folder with a name 
that matches that of the .ino file (in this case, CirqueGen6Demo_USB_PTP).

NOTE: This project uses the same USB bridge as the red board's Touchpads_USB_PTP project (formerly
Single_Touchpad_USB_PTP and Dual_Touchpads_USB_PTP, both tested on Windows 11). This Blue Board
version has not yet been tested on hardware.

## How It Works

The Gen6 touchpad is already a PTP device on its I2C-HID interface. Instead of interpreting
touch data, the Teensy passes that interface through to USB:

1. At start-up, the Teensy turns on the touchpad's power, reads the touchpad's HID descriptor
   and report descriptor over I2C, and resets the touchpad, the same way a PC's I2C-HID driver
   would.
2. The Teensy then connects to USB as a composite device: USB Serial (for the Serial Monitor)
   plus one HID interface that uses the touchpad's own report descriptor.
3. Every report the touchpad sends (touch contacts, buttons, mouse reports) is forwarded to the
   PC unchanged.
4. When the PC asks for or sends a report on the touchpad interface (Device Capabilities, the
   PTP certification status, Input Mode, Selective Reporting, ...), the request is forwarded to
   the touchpad over I2C and the touchpad's answer is returned to the PC.

The touchpad starts in Mouse/Relative mode, so the cursor works as soon as the PC finishes
enumerating. When the PC loads its touchpad driver, the driver sets the touchpad to
PTP/Absolute mode through the Input Mode feature report.

From then on the PC controls the mode, just as it would for a laptop's built-in touchpad: it sets
the mode again when the device is plugged in and when the PC wakes from sleep, and if you send
'm' the touchpad stays a Precision Touchpad as far as the PC is concerned. To test
Mouse/Relative mode on Windows, use CirqueGen6Demo_USB_Mouse. The 'm' and 'a' commands are mainly
useful on hosts that have no Precision Touchpad driver, such as macOS.

If no touchpad is found at start-up, only USB Serial is enabled. Check the connection and
reset the Teensy.

## Platform Support

* Windows 10 and 11 use the built-in Precision Touchpad driver. When it is working, the Touchpad
  page in Windows Settings says "Your PC has a precision touchpad" and offers the gesture settings.
* Linux uses the hid-multitouch driver.
* macOS does not support Precision Touchpads; expect basic pointer movement at most.

## Troubleshooting

If the PC treats the touchpad as a plain mouse (no gestures, and no "Your PC has a precision
touchpad" message in Windows Settings):

* Check that the touchpad firmware is a Precision Touchpad build. The bridge gives the PC the
  touchpad's own report descriptor, so firmware that only describes a mouse makes the bridge a
  mouse. Send 'G' in the Serial Monitor and look for the bytes 05 0D 09 05 (Digitizer, Touch Pad);
  if they are missing, the firmware has no touchpad section.
* Send 'u'. When the PC's touchpad driver loads, it asks the touchpad for reports, so
  "PC report requests" should be more than 0.
* After changing the touchpad firmware, reset the Teensy or unplug it and plug it back in. The
  bridge reads the touchpad's descriptors only at start-up.

## USB Identity

The bridge uses the Teensy USB Vendor ID 0x16C0 and Product ID 0x0487 (the Teensy
"Serial + Keyboard + Mouse + Joystick" ID), so the Arduino IDE can still upload automatically.
To give the bridge its own identity, define USB_PTP_VENDOR_ID and USB_PTP_PRODUCT_ID in
usb_ptp.h; you will then have to press the Teensy's program button to upload.

Windows remembers some device details (such as the name) per Vendor ID and Product ID. If you
change the USB descriptors and Windows doesn't pick up the change, uninstall the device in
Device Manager or change the Product ID.

## Files

* CirqueGen6Demo_USB_PTP.ino - powers and reads the touchpad (through the Cirque library),
  forwards reports and PC requests, serial commands
* usb_ptp.h - interface between the sketch and the USB layer
* usb_ptp_desc.c - USB device, configuration, HID and string descriptors
* usb_ptp_dev.c - USB device layer: a modified copy of the Teensyduino 1.59 core's usb.c. Because it
  defines the same functions, the core's own usb.c is not used. Changes from the original are
  marked "PTP bridge". If you use a newer Teensyduino, the build shows a warning; compare this
  file with the new core's usb.c.

## Commands

Using the terminal window of the Arduino IDE you can send commands that control the touchpad.
The touchpad works without the Serial Monitor open.

* Commands (case sensitive)
*   m, M - issue Mouse/Relative reports
*   a, A, p - issue PTP/Absolute reports
*     (On Windows and Linux, the PC's touchpad driver sets the mode and may override these commands)
*   k - HID power off, K - HID power on
*   i - cancel force-sleep, I - force sleep
*   f - enable feed, F - disable feed
*   d - data printing off (default), D - data printing on
*   v - PC request printing off, V - PC request printing on (default)
*   g - get device capabilities
*   G - print report descriptor
*   o - show system information
*   u - show USB bridge status
*   h, ? - print this help

The letters match the other Blue Board demos where the command is the same. G, u, v and V are
specific to this project. Data printing shows each forwarded report as hex bytes. PC request
printing shows each report request from the PC (for example the Input Mode change when the
touchpad driver loads), which is useful when checking how the PC sets up the touchpad.

Commands from CirqueGen6Demo that would change the touchpad's settings behind the PC's back
(contact and button reporting, axis inversion and swap, warm boot and power cycle) are not
included, because the PC's touchpad driver sets up the touchpad itself.
