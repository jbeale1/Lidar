#!/usr/bin/python
# get lines of text from serial port, save them to a file

from __future__ import print_function
import serial, io, time, datetime, subprocess
from subprocess import call, Popen

addr  = '/dev/ttyACM0'  # serial port to read data from
baud  = 115200            # baud rate for serial port
fname = 'LIDAR-log2.csv'   # log file to save data in
fmode = 'a'             # log file mode = append
maxRange = 3050         # maximum valid "max-range" reading (larger means some glitch)
lastRange = 0           # previous range reading
rangeDeltaMin = 75      # new event must be at least this much different than last
minInterval = datetime.timedelta(milliseconds=1500)  # minimum interval for events

secMidnight = datetime.datetime.now().replace(hour=0, minute=0, second=0, microsecond=0)
# secStart = datetime.datetime.now()
secOld = datetime.datetime.now()   # set previous event to start time

try:
  rPID = subprocess.Popen(["cat", "/tmp/raspi_PID"], stdout=subprocess.PIPE).communicate()[0]
except:
  rPID = -1

print("Raspistill process is %s " % rPID)
print(rPID)

with serial.Serial(addr,115200) as pt, open(fname,fmode) as outf:
    spb = io.TextIOWrapper(io.BufferedRWPair(pt,pt,1),
        encoding='ascii', errors='ignore', newline='\r',line_buffering=True)
    spb.readline()  # throw away first line; likely to start mid-sentence (incomplete)
    while (1):
        inline = spb.readline()  # read one line of text from serial port

        secNow = datetime.datetime.now()
        if ( 1 ):
        # if ((secNow - secOld) > minInterval):  # only pay attention if exceeded minInterval
         indat = inline.lstrip().rstrip()  # input data
         if indat != "": # add other needed checks to skip titles
           cols = indat.split(",")
           # print( cols)
           try:
            cm = int(cols[0])
	    count = int(cols[1])
            rmin = int(cols[2])
            ravg = int(cols[3])
            rmax = int(cols[4])
           except:
            rmax = 0
            count = 0
           # print( rmax )
         rangeDelta = cm - lastRange
         # print(rangeDelta)
         if (rmax < maxRange) and (cm != 0) and (abs(rangeDelta) > rangeDeltaMin):
          if ((secNow - secOld) > minInterval):  # only pay attention if exceeded minInterval
           secOld = secNow  # remember for next time
           secElapsed = '%.3f' % (secNow - secMidnight).total_seconds()
           stime = datetime.datetime.strftime(datetime.datetime.now(), '%Y-%m-%d %H:%M:%S') # current time

           lastRange = cm
           try:
            subprocess.call(["kill", "-s", "SIGUSR1", rPID])  # send capture command to raspistill
           except:
            print("Error sending signal to %s." % rPID)
           x = stime + ", " + secElapsed + ", " + indat + "\n"  # remove trailing whitespace from input
           print (x,end='')    # echo line of text on-screen
           outf.write(x)       # write line of text to file
           outf.flush()        # make sure it actually gets written out

