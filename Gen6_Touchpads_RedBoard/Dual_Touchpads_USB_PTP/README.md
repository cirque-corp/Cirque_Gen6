# Gen6 Touchpad Dev Kit - Dual USB Precision Touchpads

Copyright (c) 2026 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

### Overview

This directory contains a sample program that makes two Cirque Gen6 touchpads appear on a PC as
two USB Precision Touchpads (PTP), with a Teensy 4.0 development board acting as a USB-to-I2C
bridge. Each touchpad is passed through to the PC on its own USB interface, so the PC's own
touchpad driver provides the cursor, tap-to-click and multi-finger gestures.

This project is intended to run on the Cirque 02-000620-00 Rev A02 (or later) PCB, with a 
Teensy 4.0 on-board. The dev kit is also known as the red board or Circle Sensor Dev Kit.
Both touchpads are used: Touchpad #1 on I2C channel 0 and Touchpad #2 on I2C channel 1.

For a single touchpad, see Single_Touchpad_USB_PTP.

NOTE: The Arduino IDE requires projects to be located in a folder with a name 
that matches that of the .ino file (in this case, Dual_Touchpads_USB_PTP).

NOTE: In the Arduino IDE, you must select Tools > USB Type > "Serial". The sketch adds the
touchpad interfaces to that USB type itself; selecting any other USB type stops the build with
an error message.

NOTE: This project has not yet been tested on hardware with two touchpads connected at the same
time. How Windows handles two Precision Touchpads on one PC (for example, whether gestures work
on both, and how the Touchpad settings apply to each) has not been verified and may vary between
Windows versions. Single_Touchpad_USB_PTP, which uses the same bridge design for one touchpad,
has been tested on Windows 11.

### How It Works

The Gen6 touchpad is already a PTP device on its I2C-HID interface. Instead of interpreting
touch data, the Teensy passes that interface through to USB, once for each touchpad:

1. At start-up, the Teensy reads each touchpad's HID descriptor and report descriptor over I2C,
   and resets the touchpad, the same way a PC's I2C-HID driver would.
2. The Teensy then connects to USB as a composite device: USB Serial (for the Serial Monitor)
   plus one HID interface per touchpad found, each using that touchpad's own report descriptor.
   The touchpad on channel 0 uses USB interface 2; the touchpad on channel 1 uses interface 3,
   or interface 2 if it is the only touchpad found.
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
'r#' or 'm#' the touchpad stays a Precision Touchpad as far as the PC is concerned. To test
Mouse/Relative mode on Windows, use Dual_Touchpads_USB_Mouse. The 'a#', 'r#' and 'm#' commands
are mainly useful on hosts that have no Precision Touchpad driver, such as macOS.

If only one touchpad is found at start-up, only that touchpad is given a USB interface. If no
touchpad is found, only USB Serial is enabled. Check the connections and reset the Teensy.

### Platform Support

* Windows 10 and 11 use the built-in Precision Touchpad driver. When it is working, the Touchpad
  page in Windows Settings says "Your PC has a precision touchpad" and offers the gesture settings.
  See the note in the Overview about using two Precision Touchpads.
* Linux uses the hid-multitouch driver.
* macOS does not support Precision Touchpads; expect basic pointer movement at most.

### Troubleshooting

If the PC treats a touchpad as a plain mouse (no gestures, and no "Your PC has a precision
touchpad" message in Windows Settings):

* Check that the touchpad firmware is a Precision Touchpad build. The bridge gives the PC each
  touchpad's own report descriptor, so firmware that only describes a mouse makes the bridge a
  mouse. Send 'G0' or 'G1' in the Serial Monitor and look for the bytes 05 0D 09 05 (Digitizer,
  Touch Pad); if they are missing, the firmware has no touchpad section.
* Send 'i'. When the PC's touchpad driver loads, it asks each touchpad for reports, so
  "PC report requests" should be more than 0 for both touchpads.
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

Windows remembers some device details (such as the name) per Vendor ID and Product ID. If you
change the USB descriptors (or switch the same Teensy between this project and
Single_Touchpad_USB_PTP) and Windows doesn't pick up the change, uninstall the device in
Device Manager or change the Product ID.

### Files

* Dual_Touchpads_USB_PTP.ino - reads the touchpads, forwards reports and PC requests, serial commands
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
```
Available Commands (case sensitive)
(Replace '#' with I2C channel num [0,1])

a#, A#, p#	-	Set to PTP/Absolute Mode
r#, m#	-	Set to Mouse/Relative Mode
		(On Windows and Linux, the PC's touchpad driver sets the mode
		 and may override this command)
f#	-	Enable Feed (default)
F#	-	Disable Feed
s#	-	Print System Info
g#	-	Print HID Descriptor
G#	-	Print Report Descriptor
i	-	Print USB Bridge Status (both touchpads)

h, H, ?	-	Print this Table
d	-	Turn on Data Printing
D	-	Turn off Data Printing (default)
v	-	Turn on PC Request Printing (default)
V	-	Turn off PC Request Printing
```

Data Printing shows each forwarded report as hex bytes. PC Request Printing shows each report
request from the PC (for example the Input Mode change when the touchpad driver loads), which is
useful when checking how the PC sets up the touchpads. Each line starts with the I2C channel of
the touchpad it belongs to.

At start-up, the Serial Monitor shows which touchpads were found and which USB interface each
one uses. Send 'i' at any time to see the same information plus how many reports have been
forwarded to the PC and how many PC requests have been handled for each touchpad.
