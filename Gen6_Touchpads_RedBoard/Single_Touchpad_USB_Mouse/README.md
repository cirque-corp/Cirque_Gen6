# Gen6 Touchpad Dev Kit

Copyright (c) 2026 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

(Initial Release)

### Overview

This directory contains a sample program that demonstrates the usage of Cirque's
Gen6 touchpad solutions with a Teensy 4.0 development board acting as the host.

This project is intended to run on the Cirque 02-000620-00 Rev A02 (or later) PCB, with a 
Teensy 4.0 on-board. The dev kit is also known as the red board or Circle Sensor Dev Kit.

This project presents how to read and interpret data from the Gen6 touchpads. 
This is presented via Serial Port (Use the Serial Monitor in the Arduino IDE).
You can also change various configuration and print settings by sending commands
via Serial Port. Send 'h' for a full list of commands. 

It also enables the dev kit to act as a USB mouse by reading touch data from
an attached Cirque Gen6 touchpad and sending it to the PC as USB-HID mouse data.

NOTE: The Arduino IDE requires projects to be located in a folder with a name 
that matches that of the .ino file (in this case, Single_Touchpad_USB_Mouse).

NOTE: In the Arduino IDE, you must select Tools > USB Type > "Serial + Keyboard + Mouse + Joystick".

### Key Command Options
There is a simple command menu available upon entering the 'h' character into 
the serial monitor. 
```
Available Commands (case sensitive)

f	-	Enable Feed (default)
F	-	Disable Feed
a, p	-	Set to PTP Mode
r	-	Set to Relative Mode (default)
s	-	Print System Info
b	-	Enable Adaptive Touch
B	-	Disable Adaptive Touch

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

### Sample Output
Sample output from the serial monitor. 
```
System Information
Hardware ID:	A
Firmware ID:	0
Vendor ID:	488
Product ID:	CC35
Version ID:	1

Relative Mode Set
ReportID: 0x06, Buttons: ___, X_Delta:  -13, Y_Delta:   -9, Scroll Delta:  0
ReportID: 0x06, Buttons: ___, X_Delta:  -70, Y_Delta:  -25, Scroll Delta:  0
ReportID: 0x06, Buttons: ___, X_Delta:  -40, Y_Delta:    5, Scroll Delta:  0
ReportID: 0x06, Buttons: ___, X_Delta:    2, Y_Delta:   40, Scroll Delta:  0
ReportID: 0x06, Buttons: ___, X_Delta:   37, Y_Delta:   67, Scroll Delta:  0
ReportID: 0x06, Buttons: ___, X_Delta:   20, Y_Delta:   15, Scroll Delta:  0
Button 1 Pressed
ReportID: 0x06, Buttons: L__, X_Delta:    0, Y_Delta:    0, Scroll Delta:  0
Button 1 Released
ReportID: 0x06, Buttons: ___, X_Delta:    0, Y_Delta:    0, Scroll Delta:  0
PTP Mode Set
ReportID: 0x01, Time: 32261, Buttons: 0, Contact Count: 1
  Contact[0]: ID=0, Tip=1, Confidence=1, X=1005, Y=1177
ReportID: 0x01, Time: 32529, Buttons: 0, Contact Count: 1
  Contact[0]: ID=0, Tip=1, Confidence=1, X=1005, Y=1177
ReportID: 0x01, Time: 32594, Buttons: 0, Contact Count: 1
  Contact[0]: ID=0, Tip=1, Confidence=1, X=1012, Y=1171
ReportID: 0x01, Time: 32659, Buttons: 0, Contact Count: 1
  Contact[0]: ID=0, Tip=1, Confidence=1, X=1028, Y=1155
ReportID: 0x01, Time: 32724, Buttons: 0, Contact Count: 1
  Contact[0]: ID=0, Tip=1, Confidence=1, X=1032, Y=1139
ReportID: 0x01, Time: 32789, Buttons: 0, Contact Count: 1
  Contact[0]: ID=0, Tip=1, Confidence=1, X=1025, Y=1128
```
