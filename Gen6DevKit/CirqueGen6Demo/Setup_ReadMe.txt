The contents of the zip file should be:
  CirqueGen6Demo
    CirqueGen6Demo.ino
    Setup_ReadMe.txt (this file)
  libraries
    Cirque
      a lot of source files
    __Teensy4_I2C_CRQMods
      a lot of subdirectories and source files  

For both the Arduino IDE 1.8.x and 2.x you should be able to unzip into your Arduino directory
(Documents\Arduino) and it should put all the files in the expected locations.

Internal notes:
  The zip file is built with the CoreFW\Environment\SampleHost\move_arduino_gen6demo.bat batch file

  Details on manually moving files from the main repository:
    1) Make a sub-directory in the Arduino projects directory called CirqueGen6Demo.
    2) Copy the ino file into that directory
    3) Make a sub-directory in the Arduino Library directory (typically that is Arduino/libraries) called Cirque
    4) Copy all the files from CoreFW\Environment\SampleHost\I2CHIDLayer\src into that Cirque directory
    5) From CoreFW\Environment\SampleHost\HostBusLayer\src, copy HostBusLayer.* and Teensy4_HostBusLayer.* into that Cirque directory
    6) Make a sub-directory in the Arduino Library directory called __Teensy4_I2C_CRQMods
    7) Copy everything in CoreFW\Environment\Arduino_Teensy4_1.8.19\__Teensy4_I2C_CRQMods into the libraries\__Teensy4_I2C_CRQMods
    8) Double click on the ino file (or start the IDE and load the INO file.
    9) You may have to set the Board to be Teensy4.0, the USB to Serial, and Port


