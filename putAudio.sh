#!/bin/bash
# find and move MP3 files older than X seconds to remote host

recdir="/run/shm/audio"

sleep 8
cd $recdir
find . -maxdepth 1 -type f -name "ChA_*.mp3" -not -newermt '-5 seconds' -exec rsync --remove-source-files -av -R '{}' john@192.168.1.112:~/Audio1 \;
