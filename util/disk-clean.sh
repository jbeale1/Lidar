#!/usr/bin/zsh
# Disk space cleanup script  J.Beale

# maximum percent disk use allowed (when exceeded, delete files until not exceeded)
USELIMIT=60   

# directory of images to weed out
PDIR=/mnt/usb1/pics/full

# returns the % disk space in use on /dev/sda1, sed to remove the trailing '%' symbol
DISKUSE=$(df $PDIR | tail -1 | awk '{ print $5 }' | sed s'/%$//')

while  [[ $DISKUSE -gt $USELIMIT ]];
 do
  echo Disk usage $DISKUSE % is greater than limit of $USELIMIT
  # using zsh, find the oldest file
  OLDEST=$(ls $PDIR/*(.Om[1]))
  echo Now removing: $OLDEST
  # sleep 1
  rm $PDIR/*(.Om[1])
  # returns the % disk space in use on /dev/sda1, sed to remove the trailing '%' symbol
  DISKUSE=$(df $PDIR | tail -1 | awk '{ print $5 }' | sed s'/%$//')
 done
 
echo Disk use now: $DISKUSE %
