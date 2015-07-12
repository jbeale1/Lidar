#!/bin/bash
# gocam.sh   start serial logger & save raspistill PID for cross-process signalling

cd /mnt/usb1/pics
./dofiles.sh
#/home/pi/lidar/dofiles.sh
sudo killall raspistill
sudo killall serlog1.py

cd /mnt/usb1/pics
raspistill -n -ex sports -ev -4 -t 0 -s -o %03d.jpg &
#raspistill -n -ex night -roi .15,0,1.0,0.80 -rot 270 -w 800 -h 700 -t 0 -s -o %03d.jpg &
#raspistill -n -ex night -rot 270 -w 600 -h 800 -t 0 -s -o %03d.jpg &
#raspistill -ex night -rot 270 -w 800 -h 600 -t 0 -s -o %03d.jpg &
# raspistill -ex night -roi 0,0.3,0.55,0.55 -w 800 -h 600 -t 0 -s -o %03d.jpg &
echo -n $! > /tmp/raspi_PID
/home/pi/Lidar/serlog1.py &

# kill -s SIGUSR1 $(cat /tmp/raspi_PID)  # trigger image capture
