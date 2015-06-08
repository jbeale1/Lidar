# LIDAR-Lite
Acquire ranges via I2C on LIDAR-Lite module, and trigger Raspberry Pi camera on detected events

There is a .ino file in the 'util' directory which runs on a Teensy T3.1 board, that talks to the LIDAR-Lite via I2C
and acquires range data as fast as possible (around 100 Hz, although it seems to vary with signal strength).
This code computes a rolling average of "minimum", "average" and "maximum" values returned. The min and max values have
a peak-hold behavior in that the increasing and decreasing filter time-constants are different.  It sends data out two
serial ports; in my setup one goes via the USB-serial port to a Raspberry Pi for logging, another goes to a 3.3V-level
serial-input LCD display (Adafruit) which is handy for live feedback during initial setup.

The rest of the code is shell scripts and python commands that run on the Pi to capture an image (using the raspistill
external-trigger function) whenever a significantly different range comes in from the Teensy board.

The LIDAR-Lite unit is made by http://pulsedlight3d.com/ and I got mine from Sparkfun.
