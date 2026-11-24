# Gen6 Touchpad Dev Kit - USB Mouse

Copyright (c) 2026 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

### Overview

This directory contains a sample program that demonstrates the usage of Cirque's
Gen6 touchpad solutions with a Teensy 4.0 development board acting as the host.

The Teensy reads the touchpads' reports over I2C and converts them into USB mouse and keyboard
data, so the PC's cursor moves when you touch a touchpad. This works on any operating system,
but the PC sees a mouse, not a touchpad, so there are no multi-finger gestures (use
Touchpads_USB_PTP for those). In Mouse/Relative mode the touchpad's movement, buttons and
scrolling are passed on; in PTP/Absolute mode the Teensy turns the first finger's position into
cursor movement. Keyboard reports from the touchpad are sent as USB key presses.

The reports are also shown in the Serial Monitor, and you can change various configuration and
print settings by sending commands via Serial Port. Send 'h' for a full list of commands.

This project is intended to run on the Cirque 02-000620-00 Rev A02 (or later) PCB, with a 
Teensy 4.0 on-board. The dev kit is also known as the red board or Circle Sensor Dev Kit.

It works with one or two touchpads, in either position. At start-up the Teensy checks both
touchpad connectors and only uses the touchpads that answer. With two touchpads, both move the
same USB mouse cursor, and the mouse button is pressed while either touchpad's button is pressed.
Every Serial Monitor line about a touchpad starts with [TP1] or [TP2], so you can see which
position it is in:

| Board label | Connector | I2C channel in the code | Serial Monitor label |
|---|---|---|---|
| Touchpad #1 | J7 | 0 (Wire: SDA pin 18, SCL pin 19, DR pin 9) | [TP1] |
| Touchpad #2 | J9 | 1 (Wire1: SDA pin 17, SCL pin 16, DR pin 7) | [TP2] |

NOTE: The Arduino IDE requires projects to be located in a folder with a name 
that matches that of the .ino file (in this case, Touchpads_USB_Mouse).

NOTE: In the Arduino IDE, you must select Tools > USB Type > "Serial + Keyboard + Mouse + Joystick".
With any other USB type the build stops with an error message.

The sketch waits for the Serial Monitor to be opened before it starts.

### Key Command Options
There is a simple command menu available upon entering the 'h' character into 
the serial monitor. Touchpad commands (the first group) go to every connected touchpad; add 1
or 2 to send one to a single touchpad. For example, 'a' sets every connected touchpad to
PTP/Absolute mode, and 'a2' sets only Touchpad #2. The other commands take no number.
```
Available Commands (case sensitive)
Touchpad commands go to every connected touchpad. To send one to a single
touchpad, add its number (1 or 2), for example 'a2' for Touchpad #2 only.

a, A, p	-	Set to PTP/Absolute Mode
r, m	-	Set to Mouse/Relative Mode (default)
f	-	Enable Feed (default)
F	-	Disable Feed
s	-	Print System Info
g	-	Print HID Descriptor
b	-	Enable Adaptive Touch
B	-	Disable Adaptive Touch
c	-	Force Compensation
C	-	Factory Calibrate (nothing may touch the touchpad)
v	-	Enable Compensation (default)
V	-	Disable Compensation
t	-	Enable Tracking (default)
T	-	Disable Tracking
i	-	Print Compensation Matrix
S	-	Save Settings to Flash

h, H, ?	-	Print this Table
d	-	Turn on Data Printing (default)
D	-	Turn off Data Printing
e	-	Turn on Event Printing (default)
E	-	Turn off Event Printing
z	-	Turn on decode failure diagnostics (default)
Z	-	Turn off decode failure diagnostics
x	-	Turn on raw HID packet dump
X	-	Turn off raw HID packet dump
```

Commands work with or without a line ending selected in the Serial Monitor.

### Using one touchpad / porting to your own microcontroller

To use one touchpad on the dev kit, connect it to either connector and reset the Teensy; no
code changes are needed.

Every API_C3_ and HB_ function takes an `i2c_channel` parameter that selects the touchpad:
0 for Touchpad #1 and 1 for Touchpad #2. (The Serial Monitor shows the touchpad number, which
is the I2C channel + 1.) When you port this code to a product with one touchpad, pass 0
everywhere, or remove the parameter if you prefer. The files to adapt are:

* I2C.cpp - the I2C bus functions; I2C channel 0 and 1 are the Teensy's Wire and Wire1 buses
* HostDR.cpp and Project_Config.h - the Data Ready (DR) pin of each touchpad (DR0_MASK and
  DR1_MASK bits), the I2C pins and the I2C clock
* API_Hardware.c - dev kit hardware set-up and the millisecond delay used by API_C3.c
* Touchpads_USB_Mouse.ino - start-up, the main loop, the Serial Monitor output, and
  performMouseReport() / performKeyboardReport(), which use the Teensy's USB Mouse and
  Keyboard objects; replace those with your own USB stack's mouse and keyboard functions

API_C3.c (touchpad settings), API_HostBus.c (I2C-HID transfers) and HID_Reports.c (report
decoding) only use the functions above, so they can usually be used unchanged.

### Sample Output
Start-up with both touchpads connected, a finger moving on each touchpad in Mouse/Relative
mode, then 'a' to switch to PTP/Absolute mode (reports shortened):

```
Gen6 Touchpad Dev Kit - USB Mouse Demo
[TP1] Touchpad #1 found
[TP2] Touchpad #2 found
[TP1] System Information
Hardware ID:	0xA
Firmware ID:	0x0
Vendor ID:	0x488
Product ID:	0xC002
Version ID:	0xB001

[TP2] System Information
Hardware ID:	0xA
Firmware ID:	0x0
Vendor ID:	0x488
Product ID:	0x10C0
Version ID:	0x1

Send 'h' for a list of commands

[TP1] ReportID: 0x06, Buttons: ___, X_Delta:   -5, Y_Delta:   38, Scroll Delta:  0
[TP1] ReportID: 0x06, Buttons: ___, X_Delta:  -56, Y_Delta:  127, Scroll Delta:  0
[TP1] ReportID: 0x06, Buttons: ___, X_Delta:   23, Y_Delta:   31, Scroll Delta:  0
[TP2] ReportID: 0x06, Buttons: ___, X_Delta:   -3, Y_Delta:   -9, Scroll Delta:  0
[TP2] ReportID: 0x06, Buttons: ___, X_Delta:  -12, Y_Delta:  -44, Scroll Delta:  0
[TP2] ReportID: 0x06, Buttons: ___, X_Delta:  -25, Y_Delta:  -44, Scroll Delta:  0
[TP1] Setting PTP/Absolute Mode...
Done
[TP2] Setting PTP/Absolute Mode...
Done

[TP1] ReportID: 0x01, Time:  4525, Buttons: 0, Contact Count: 1
  Contact[0]: ID=0, Tip=1, Confidence=1, X= 444, Y= 566
[TP1] ReportID: 0x01, Time:  4606, Buttons: 0, Contact Count: 1
  Contact[0]: ID=0, Tip=1, Confidence=1, X= 443, Y= 577
[TP1] ReportID: 0x01, Time:  4686, Buttons: 0, Contact Count: 1
  Contact[0]: ID=0, Tip=1, Confidence=1, X= 440, Y= 608
[TP2] ReportID: 0x01, Time: 47725, Buttons: 0, Contact Count: 1
  Contact[0]: ID=0, Tip=1, Confidence=1, X=1651, Y= 851
[TP2] ReportID: 0x01, Time: 47790, Buttons: 0, Contact Count: 1
  Contact[0]: ID=0, Tip=1, Confidence=1, X=1651, Y= 850
[TP2] ReportID: 0x01, Time: 47855, Buttons: 0, Contact Count: 1
  Contact[0]: ID=0, Tip=1, Confidence=1, X=1651, Y= 848
```

Start-up with only Touchpad #1 connected:

```
Gen6 Touchpad Dev Kit - USB Mouse Demo
[TP1] Touchpad #1 found
[TP2] Touchpad #2 not connected (no answer at I2C address 0x2C, I2C error 2)
[TP1] System Information
Hardware ID:	0xA
Firmware ID:	0x0
Vendor ID:	0x488
Product ID:	0xC002
Version ID:	0xB001

Send 'h' for a list of commands
```
