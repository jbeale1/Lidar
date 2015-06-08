#!/bin/bash
# dofiles.sh  add time/date to all JPEG files, and move to pics/ directory

export PREFIX="p"
FILES=./*.jpg
for fname in *.jpg;
 do
   echo "processing $fname..."
   D=$(stat -c '%Y' $fname)          # seconds since creation time of file
   DF=$(date -d @$D '+%Y%m%d_%H%M%S')    # formatted creation date/time
   mv $fname pics/"${PREFIX}_${DF}_${fname}"  # add date_time to filename
 done
tail -500 /home/pi/lidar/LIDAR-log2.csv > /home/pi/lidar/pics/LIDAR.txt

#sudo killall sitecopy
# remove JPEG files older than this many minutes
find /home/pi/lidar/pics/*.jpg -mmin +860 -exec rm {} \;
/usr/bin/sitecopy -u cam1   # upload files to remote user-facing website
