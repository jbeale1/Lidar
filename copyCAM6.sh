#!/bin/bash

# Copy jpegs stored by Dahua camera in deep nested dirs to one day's flat dir

fromdir0="/home/user/CAM6/3KPAG9EFxxxxxxx"
todir0="/media/t2/CAM6"

# ================================================================================
#ydate=$(date +%Y-%m-%d -d "yesterday")    # yesterday's date formated as YYYY-MM-DD
ydate=$(date +%Y-%m-%d -d "today")    #  date formated as YYYY-MM-DD

fromdir=$fromdir0/$ydate
todir=$todir0/$ydate

if [ ! -d $todir ]; then  # create directory if it doesn't already exist
  echo "Creating $todir"
  mkdir $todir
fi

echo "From: $fromdir"

find $fromdir -type f -iname "*.jpg" -print0 | while IFS= read -r -d $'\0' line; do
    base=$(echo $line | egrep -o "(/[0-9][0-9]){3}.{11}\.jpg")
    new1=$(echo -n $base | sed 's/\[.*\]//' | tr  '/' '-' )
    newname=${new1:1}  # remove initial '-' character from filename
    fullname=$todir/$newname # add directory to form full filename
    if [ ! -f $fullname ]; then  # if file doesn't exist yet, copy it over
#      cp $line $fullname
      mv $line $fullname
    fi
done

# run ALPR to detect license plate numbers and write to file
# /usr/local/bin/findPlates.sh $todir
