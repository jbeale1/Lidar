#!/bin/bash
# gocam.sh   start serial logger & save raspistill PID for cross-process signalling

/home/pi/lidar/dofiles.sh
sudo killall raspistill
sudo killall serlog1.py
raspistill -ex night -rot 270 -w 800 -h 600 -t 0 -s -o %03d.jpg &
# raspistill -ex night -roi 0,0.3,0.55,0.55 -w 800 -h 600 -t 0 -s -o %03d.jpg &
echo -n $! > /tmp/raspi_PID
./serlog1.py &

# kill -s SIGUSR1 $(cat /tmp/raspi_PID)  # trigger image capture
