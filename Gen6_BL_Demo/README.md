# Gen6 Bootloader Demo

// Copyright (c) 2025 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

# Overview
This directory contains a sample program that demonstrates how to interface with Cirque's Gen6 Bootloader with a simple serial terminal.
The program is intended for Gen6 parts (1p3). It will load either CustomMeas or SpiderMeas firmware into the parts.

NOTE: use the Arduino serial monitor (found in the Tools menu) to issue character-based commands.

# Customizing
To load your own firmware into the parts you'll need to convert the hex file into a binary array and provide start address and size information. See FW_*.h for examples of the data required.
You can use the SRecord project (sourceforge) to convert a hex file to a c source file (const array).

* srec_cat hexFileFullPath -intel -o headerFileFullPath -C-Array arrayName


