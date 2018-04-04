#!/usr/bin/python

# Print out data stream from 50 Hz single-channel Raspberry Shake
# See also: https://manual.raspberryshake.org/udp.html#udp
# https://groups.google.com/forum/#!topic/raspberryshake/vZOybDRDpHw

import socket as s
import math, time

host = ""                   # can be blank for localhost
port = 8888                             # Port to bind to
sock = s.socket(s.AF_INET, s.SOCK_DGRAM | s.SO_REUSEADDR)
sock.bind((host, port))     # connect to this socket

def plist( list ):
  "Print out elements of list, one per line"
  for elem in list:
    print elem

# from http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
def stdev( dat ):
  "Calculate standard deviation of elements"
  n = int(0)
  mean= float(0)
  m2 = float(0)
  for elem in dat:
    x = float(elem)
    n += 1
    delta = x - mean
    mean += delta/n
    m2 += (delta * (x - mean))
  sdev = math.sqrt( m2/(n-1) )
  return sdev

def pspair( elems ):
  "Print out std.dev of 1st & 2nd half of list"
  n=len(elems)/2
  print("%.1f" % stdev(elems[:n]))
  print("%.1f" % stdev(elems[n:]))

data, addr = sock.recvfrom(1024)    # wait to receive data
elems = data.translate(None, '{}').split(",") # remove brackets, separate elements
channel = elems[0]                  # first elem is channel, eg. 'SHZ'
tstamp = time.strftime("%a, %d %b %Y %H:%M:%S UTC", time.gmtime(float(elems[1])))
print "# " + channel + " " + tstamp
pspair(elems[2:])

while 1:                                # loop forever
    data, addr = sock.recvfrom(1024)    # wait to receive data
    elems = data.translate(None, '{}').split(",") # remove brackets, separate elements
    pspair(elems[2:])

# ===============================================================
