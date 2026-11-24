# Gen6 Touchpad Dev Kit - USB PTP

Copyright (c) 2026 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

### Overview

This directory contains a sample program that makes Cirque Gen6 touchpads appear on a PC as
USB Precision Touchpads (PTP), with a Teensy 4.0 development board acting as a USB-to-I2C
bridge. Each touchpad is passed through to the PC on its own USB interface, so the PC sees a
normal touchpad and its own touchpad driver provides the cursor, tap-to-click and multi-finger
gestures (two-finger scroll, pinch zoom, three- and four-finger swipes).

This project is intended to run on the Cirque 02-000620-00 Rev A02 (or later) PCB, with a 
Teensy 4.0 on-board. The dev kit is also known as the red board or Circle Sensor Dev Kit.

It works with one or two touchpads, in either position. At start-up the Teensy checks both
touchpad connectors and gives a USB interface only to the touchpads that answer. Every Serial
Monitor line about a touchpad starts with [TP1] or [TP2], so you can see which position it is in:

| Board label | Connector | I2C channel in the code | Serial Monitor label |
|---|---|---|---|
| Touchpad #1 | J7 | 0 (Wire: SDA pin 18, SCL pin 19, DR pin 9) | [TP1] |
| Touchpad #2 | J9 | 1 (Wire1: SDA pin 17, SCL pin 16, DR pin 7) | [TP2] |

NOTE: The Arduino IDE requires projects to be located in a folder with a name 
that matches that of the .ino file (in this case, Touchpads_USB_PTP).

NOTE: In the Arduino IDE, you must select Tools > USB Type > "Serial". The sketch adds the
touchpad interfaces to that USB type itself; selecting any other USB type stops the build with
an error message.

The bridge was tested on Windows 11 with one touchpad, and with two touchpads connected at the
same time; both work as Precision Touchpads.

### How It Works

The Gen6 touchpad is already a PTP device on its I2C-HID interface. Instead of interpreting
touch data, the Teensy passes that interface through to USB, once for each touchpad:

1. At start-up, the Teensy checks both connectors with a quick I2C ping. For each touchpad that
   answers, it reads the touchpad's HID descriptor and report descriptor over I2C, and resets the
   touchpad, the same way a PC's I2C-HID driver would. A connector with no touchpad is skipped
   straight away, so it doesn't slow down start-up.
2. The Teensy then connects to USB as a composite device: USB Serial (for the Serial Monitor)
   plus one HID interface per touchpad found, each using that touchpad's own report descriptor.
   The interfaces are numbered in order from the touchpads found: Touchpad #1 uses USB
   interface 2; Touchpad #2 uses interface 3, or interface 2 if it is the only touchpad found.
   Each interface is named "Touchpad #1" or "Touchpad #2" after the board label.
3. Every report a touchpad sends (touch contacts, buttons, mouse reports) is forwarded to the PC
   unchanged, on that touchpad's interface.
4. When the PC asks for or sends a report on a touchpad's interface (Device Capabilities, the
   PTP certification status, Input Mode, Selective Reporting, ...), the request is forwarded to
   that touchpad over I2C and the touchpad's answer is returned to the PC.

The touchpads start in Mouse/Relative mode, so the cursor works as soon as the PC finishes
enumerating. When the PC loads its touchpad driver, the driver sets each touchpad to
PTP/Absolute mode through the Input Mode feature report.

From then on the PC controls the mode, just as it would for a laptop's built-in touchpad: it sets
the mode again when the device is plugged in and when the PC wakes from sleep, and if you send
'r' or 'm' the touchpad stays a Precision Touchpad as far as the PC is concerned. To test
Mouse/Relative mode on Windows, use Touchpads_USB_Mouse. The 'a', 'r' and 'm' commands are
mainly useful on hosts that have no Precision Touchpad driver, such as macOS.

If no touchpad is found at start-up, only USB Serial is enabled. Check the connections and
reset the Teensy.

### Platform Support

* Windows 10 and 11 use the built-in Precision Touchpad driver. When it is working, the Touchpad
  page in Windows Settings says "Your PC has a precision touchpad" and offers the gesture settings.
* Linux uses the hid-multitouch driver.
* macOS does not support Precision Touchpads; expect basic pointer movement at most.

### Troubleshooting

If the PC treats a touchpad as a plain mouse (no gestures, and no "Your PC has a precision
touchpad" message in Windows Settings):

* Check that the touchpad firmware is a Precision Touchpad build. The bridge gives the PC each
  touchpad's own report descriptor, so firmware that only describes a mouse makes the bridge a
  mouse. Send 'G' (or 'G1' / 'G2' for one touchpad) in the Serial Monitor and look for the bytes
  05 0D 09 05 (Digitizer, Touch Pad); if they are missing, the firmware has no touchpad section.
* Send 'i'. When the PC's touchpad driver loads, it asks each touchpad for reports, so
  "PC report requests" should be more than 0 for every touchpad found.
* After changing the touchpad firmware, reset the Teensy or unplug it and plug it back in. The
  bridge reads the touchpads' descriptors only at start-up.
* If one touchpad works and the other doesn't, try each touchpad on its own (disconnect the
  other and reset the Teensy) to tell a touchpad problem from a problem with two Precision
  Touchpads on the same PC.

### USB Identity

The bridge uses the Teensy USB Vendor ID 0x16C0 and Product ID 0x0487 (the Teensy
"Serial + Keyboard + Mouse + Joystick" ID), so the Arduino IDE can still upload automatically.
To give the bridge its own identity, define USB_PTP_VENDOR_ID and USB_PTP_PRODUCT_ID in
usb_ptp.h; you will then have to press the Teensy's program button to upload.

Windows remembers some device details (such as the names) per Vendor ID and Product ID. If you
change the USB descriptors, change which connectors have a touchpad, or switch the same Teensy
from an older version of this project, and Windows doesn't pick up the change, uninstall the
device in Device Manager or change the Product ID.

### Files

* Touchpads_USB_PTP.ino - finds the touchpads, forwards reports and PC requests, serial commands
* API_HostBus.c - adds generic I2C-HID GET_REPORT and SET_REPORT commands for either I2C channel
* I2C.cpp - I2C reads are not limited to 255 bytes, because report descriptors and the PTP
  certification report are longer
* usb_ptp.h - interface between the sketch and the USB layer
* usb_ptp_desc.c - USB device, configuration, HID and string descriptors
* usb_ptp_dev.c - USB device layer: a modified copy of the Teensyduino 1.59 core's usb.c. Because it
  defines the same functions, the core's own usb.c is not used. Changes from the original are
  marked "PTP bridge". If you use a newer Teensyduino, the build shows a warning; compare this
  file with the new core's usb.c.

### Key Command Options
There is a simple command menu available upon entering the 'h' character into 
the serial monitor. The touchpads work without the Serial Monitor open.

Touchpad commands (the first group) go to every connected touchpad; add 1 or 2 to send one to a
single touchpad. For example, 'g' prints the HID descriptor of every connected touchpad, and
'g2' prints only Touchpad #2's. The other commands take no number.
```
Available Commands (case sensitive)
Touchpad commands go to every connected touchpad. To send one to a single
touchpad, add its number (1 or 2), for example 'a2' for Touchpad #2 only.

a, A, p	-	Set to PTP/Absolute Mode
r, m	-	Set to Mouse/Relative Mode
		(On Windows and Linux, the PC's touchpad driver sets the mode
		 and may override this command)
f	-	Enable Feed (default)
F	-	Disable Feed
s	-	Print System Info
g	-	Print HID Descriptor
G	-	Print Report Descriptor

i	-	Print USB Bridge Status (all touchpads)
h, H, ?	-	Print this Table
d	-	Turn on Data Printing
D	-	Turn off Data Printing (default)
v	-	Turn on PC Request Printing (default)
V	-	Turn off PC Request Printing
```

Commands work with or without a line ending selected in the Serial Monitor.

Data Printing shows each forwarded report as hex bytes. PC Request Printing shows each report
request from the PC (for example the Input Mode change when the touchpad driver loads), which is
useful when checking how the PC sets up the touchpads.

At start-up, the Serial Monitor shows which touchpads were found and which USB interface each
one uses. Send 'i' at any time to see the same information plus how many reports have been
forwarded to the PC and how many PC requests have been handled for each touchpad.

### Using one touchpad / porting to your own microcontroller

To use one touchpad on the dev kit, connect it to either connector and reset the Teensy; no
code changes are needed. The PC then sees one Precision Touchpad.

Every API_C3_, HB_ and usb_ptp_ function takes an I2C channel parameter that selects the
touchpad: 0 for Touchpad #1 and 1 for Touchpad #2. (The Serial Monitor shows the touchpad
number, which is the I2C channel + 1.) When you port this code to a product with one touchpad,
pass 0 everywhere. The files to adapt are:

* I2C.cpp - the I2C bus functions; I2C channel 0 and 1 are the Teensy's Wire and Wire1 buses
* HostDR.cpp and Project_Config.h - the Data Ready (DR) pin of each touchpad (DR0_MASK and
  DR1_MASK bits), the I2C pins and the I2C clock
* API_Hardware.c - dev kit hardware set-up and the millisecond delay used by API_C3.c
* usb_ptp_dev.c and usb_ptp_desc.c - Teensy 4 specific; on another microcontroller, use your own
  USB stack with one HID interface per touchpad and follow Touchpads_USB_PTP.ino: give the host
  each touchpad's report descriptor, forward input reports, and forward GET_REPORT / SET_REPORT
  requests with HB_HID_getReport() / HB_HID_setReport()

### Sample Output
<!-- TODO before publishing: paste real Serial Monitor output from Touchpads_USB_PTP here, in a
     ``` code block (start-up with Touchpad #1 only, Touchpad #2 only and both, plus the output of
     'i'). The previous sample was removed because the output format changed. -->
