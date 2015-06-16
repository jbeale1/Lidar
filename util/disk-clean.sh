#!/usr/bin/zsh
# Disk space cleanup script  J.Beale  15 June 2015

# maximum percent disk use allowed (when exceeded, delete files until not exceeded)
USELIMIT=60   

# directory of images to weed out
PDIR=/mnt/usb1/pics/full

# returns the percentage disk space in use. sed to remove the trailing '%' symbol
DISKUSE=$(df $PDIR | tail -1 | awk '{ print $5 }' | sed s'/%$//')

while  [[ $DISKUSE -gt $USELIMIT ]];
 do
  echo Disk usage $DISKUSE % is greater than limit of $USELIMIT
  # using zsh, find the single oldest file
  # OLDEST=$(ls $PDIR/*(.Om[1]))
  # echo Now removing: $OLDEST
  rm $PDIR/*(.Om[1])
  DISKUSE=$(df $PDIR | tail -1 | awk '{ print $5 }' | sed s'/%$//')
 done
 
echo Disk use now: $DISKUSE %
