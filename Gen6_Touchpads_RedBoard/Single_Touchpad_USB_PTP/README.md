# Gen6 Touchpad Dev Kit - USB Precision Touchpad (Single Touchpad)

Copyright (c) 2026 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

### Overview

This directory contains a sample program that makes a Cirque Gen6 touchpad appear on a PC as a
USB Precision Touchpad (PTP), with a Teensy 4.0 development board acting as a USB-to-I2C bridge.
The PC sees a normal touchpad, so you get the cursor, tap-to-click and multi-finger gestures
(two-finger scroll, pinch zoom, three- and four-finger swipes) through the PC's own touchpad driver.

This project is intended to run on the Cirque 02-000620-00 Rev A02 (or later) PCB, with a 
Teensy 4.0 on-board. The dev kit is also known as the red board or Circle Sensor Dev Kit.
Only Touchpad #1 is used.

NOTE: The Arduino IDE requires projects to be located in a folder with a name 
that matches that of the .ino file (in this case, Single_Touchpad_USB_PTP).

NOTE: In the Arduino IDE, you must select Tools > USB Type > "Serial". The sketch adds the
touchpad interface to that USB type itself; selecting any other USB type stops the build with
an error message.

### How It Works

The Gen6 touchpad is already a PTP device on its I2C-HID interface. Instead of interpreting
touch data, the Teensy passes that interface through to USB:

1. At start-up, the Teensy reads the touchpad's HID descriptor and report descriptor over I2C,
   and resets the touchpad, the same way a PC's I2C-HID driver would.
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
'r' or 'm' the touchpad stays a Precision Touchpad as far as the PC is concerned. To test Mouse/Relative mode on Windows,
use Single_Touchpad_USB_Mouse. The 'a', 'r' and 'm' commands are mainly useful on hosts that
have no Precision Touchpad driver, such as macOS.

If no touchpad is found at start-up, only USB Serial is enabled. Check the connection and
reset the Teensy.

### Platform Support

* Windows 10 and 11 use the built-in Precision Touchpad driver. When it is working, the Touchpad
  page in Windows Settings says "Your PC has a precision touchpad" and offers the gesture settings.
* Linux uses the hid-multitouch driver.
* macOS does not support Precision Touchpads; expect basic pointer movement at most.

### Troubleshooting

If the PC treats the touchpad as a plain mouse (no gestures, and no "Your PC has a precision
touchpad" message in Windows Settings):

* Check that the touchpad firmware is a Precision Touchpad build. The bridge gives the PC the
  touchpad's own report descriptor, so firmware that only describes a mouse makes the bridge a
  mouse. Send 'G' in the Serial Monitor and look for the bytes 05 0D 09 05 (Digitizer, Touch Pad);
  if they are missing, the firmware has no touchpad section.
* Send 'i'. When the PC's touchpad driver loads, it asks the touchpad for reports, so
  "PC report requests" should be more than 0.
* After changing the touchpad firmware, reset the Teensy or unplug it and plug it back in. The
  bridge reads the touchpad's descriptors only at start-up.

### USB Identity

The bridge uses the Teensy USB Vendor ID 0x16C0 and Product ID 0x0487 (the Teensy
"Serial + Keyboard + Mouse + Joystick" ID), so the Arduino IDE can still upload automatically.
To give the bridge its own identity, define USB_PTP_VENDOR_ID and USB_PTP_PRODUCT_ID in
usb_ptp.h; you will then have to press the Teensy's program button to upload.

Windows remembers some device details (such as the name) per Vendor ID and Product ID. If you
change the USB descriptors and Windows doesn't pick up the change, uninstall the device in
Device Manager or change the Product ID.

### Files

* Single_Touchpad_USB_PTP.ino - reads the touchpad, forwards reports and PC requests, serial commands
* API_HostBus.c - adds generic I2C-HID GET_REPORT and SET_REPORT commands
* usb_ptp.h - interface between the sketch and the USB layer
* usb_ptp_desc.c - USB device, configuration, HID and string descriptors
* usb_ptp_dev.c - USB device layer: a modified copy of the Teensyduino 1.59 core's usb.c. Because it
  defines the same functions, the core's own usb.c is not used. Changes from the original are
  marked "PTP bridge". If you use a newer Teensyduino, the build shows a warning; compare this
  file with the new core's usb.c.

### Key Command Options
There is a simple command menu available upon entering the 'h' character into 
the serial monitor. The touchpad works without the Serial Monitor open.
```
Available Commands (case sensitive)

a, A, p	-	Set to PTP/Absolute Mode
r, m	-	Set to Mouse/Relative Mode
		(On Windows and Linux, the PC's touchpad driver sets the mode
		 and may override this command)
f	-	Enable Feed (default)
F	-	Disable Feed
s	-	Print System Info
g	-	Print HID Descriptor
G	-	Print Report Descriptor
i	-	Print USB Bridge Status

h, H, ?	-	Print this Table
d	-	Turn on Data Printing
D	-	Turn off Data Printing (default)
v	-	Turn on PC Request Printing (default)
V	-	Turn off PC Request Printing
```

Data Printing shows each forwarded report as hex bytes. PC Request Printing shows each report
request from the PC (for example the Input Mode change when the touchpad driver loads), which is
useful when checking how the PC sets up the touchpad.

### Sample Output
Sample output from the serial monitor at start-up.
```
Single Pad USB PTP Demo
Touchpad found, VID 0x488 PID 0x10C0, report descriptor 475 bytes
USB: configured by the PC
Send 'h' for a list of commands
```

With PC Request Printing on, each request from the PC is printed as it arrives. Send 'i' to see
how many reports have been forwarded to the PC and how many PC requests have been handled.
