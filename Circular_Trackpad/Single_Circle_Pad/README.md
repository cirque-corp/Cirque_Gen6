# Gen 6 Circle Sensor Dev Kit

Copyright (c) 2023 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

(Initial Release)

### Overview

This directory contains a sample program that demonstrates the usage of Cirque's
Gen6 touchpad solutions with a Teensy 4.0 development board acting as the host.

This project is intended to run on the Cirque 02-000620-00 PCB, with a 
Teensy 4.0 on-board.

This project presents how to read and interpret data from the Gen6 touchpads. 
This is presented via Serial Port (Use the Serial Monitor in the Arduino IDE).
You can also change various configuration and print settings by sending commands
via Serial Port. Send 'h' for a full list of commands. 

NOTE: The Arduino IDE requires projects to be located in a folder with a name 
that matches that of the .ino file (in this case, Single_Circle_Pad).

### Key Command Options
There is a simple command menu available upon entering the 'h' character into 
the serial monitor. 
```
Available Commands (case sensitive)

c	-	Force Compensation
C	-	Factory Calibrate
f	-	Enable Feed (default)
F	-	Disable Feed
a, p	-	Set to PTP Mode
r	-	Set to Relative Mode (default)
S	-	Save Settings to Flash
s	-	Print System Info
t	-	Enable Tracking (default)
T	-	Disable Tracking
v	-	Enable Compensation (default)
V	-	Disable Compensation

h, H, ?	-	Print this Table
d	-	Turn on Data Printing (default)
D	-	Turn off Data Printing 
e	-	Turn on Event Printing (default)
E	-	Turn off Event Printing 
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
ReportID: 0x01, Time: 32261, ContactID: 0, Confidence: 1, Tip: 1, X: 1005, Y: 1177, Buttons: 0, Contact Count: 1
ReportID: 0x01, Time: 32529, ContactID: 0, Confidence: 1, Tip: 1, X: 1005, Y: 1177, Buttons: 0, Contact Count: 1
ReportID: 0x01, Time: 32594, ContactID: 0, Confidence: 1, Tip: 1, X: 1012, Y: 1171, Buttons: 0, Contact Count: 1
ReportID: 0x01, Time: 32659, ContactID: 0, Confidence: 1, Tip: 1, X: 1028, Y: 1155, Buttons: 0, Contact Count: 1
ReportID: 0x01, Time: 32724, ContactID: 0, Confidence: 1, Tip: 1, X: 1032, Y: 1139, Buttons: 0, Contact Count: 1
ReportID: 0x01, Time: 32789, ContactID: 0, Confidence: 1, Tip: 1, X: 1025, Y: 1128, Buttons: 0, Contact Count: 1
```
