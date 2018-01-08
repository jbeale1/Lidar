#!/bin/bash

# location to store audio files
todir="/run/shm/audio/"
#todir="./"

# create storage folder if not already existing
mkdir $todir

# set mic input level to ~ 0 dB (which is strangely 15 out of 100)
sudo amixer -c 1 cset numid=3 15

# Record 5-minute-long MP3 files from audio device
ffmpeg -f alsa -ac 2 -ar 48000 -i plughw:1 -map 0:0 -acodec libmp3lame \
  -b:a 256k -f segment -strftime 1 -segment_time 300 -segment_atclocktime 1 \
   "$todir"ChA_%Y-%m-%d_%H-%M-%S.mp3
