#!/bin/bash

# set working directory
WD="/mnt/usb1/RCWL"

export PATH="/usr/local/bin:/usr/bin:/bin:/sbin"

echo -n `date` > $WD/doppler5.txt
echo -n '   ' >> $WD/doppler5.txt
echo `vcgencmd measure_temp` >> $WD/doppler5.txt
iwconfig wlan0 | grep "Quality" >>  $WD/doppler5.txt
uptime >> $WD/doppler5.txt
df | grep /dev/sda1 >> $WD/doppler5.txt
echo '-------' >> $WD/doppler5.txt
cat $WD/*.csv | tail -300 >> $WD/doppler5.txt

put1 $WD/doppler5.txt doppler5.txt
