#!/bin/bash
# dofiles.sh  add time/date to all JPEG files, and move to pics/ directory

export PREFIX="p"

for fname in *.jpg;
 do
   echo "processing $fname..."
   PSIZE=`identify $fname | awk '{print $3}'`
   if [ "$PSIZE" == "2592x1944" ]; then
     D=$(stat -c '%Y' $fname)          # seconds since creation time of file
     DF=$(date -d @$D '+%Y%m%d_%H%M%S')    # formatted creation date/time
     # crop out middle third, rotate 90 deg. CCW, scale 50%, rename with original time/date, in ./pics/
     convert $fname -crop 864x1944+864x0 -rotate "-90<" -resize 50% pics/"${PREFIX}_${DF}_${fname}" 
     mv $fname full/"${DF}_${fname}"  # save original full-size image
   fi
 done

tail -500 /mnt/usb1/pics/LIDAR-log2.csv > /mnt/usb1/pics/pics/LIDAR.txt

# remove JPEG files older than this many minutes
find /mnt/usb1/pics/pics/*.jpg -mmin +860 -exec rm {} \;
# remove full-size JPEG files older than X days
find /mnt/usb1/pics/full/*.jpg -mtime +5 -exec rm {} \;

/usr/bin/sitecopy -u cam2   # upload files to remote user-facing website

