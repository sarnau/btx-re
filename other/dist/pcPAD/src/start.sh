#!/bin/bash

# This is a startscript that starts the pcPAD software
# automatically, assuming that your terminal is connected
# to port /dev/ttyUSB0.

# If the your videotex pages are loaded from a local
# directory use:
#./pcpad -p /dev/ttyUSB0 -u ../../srv

# If you want to use a webserver use:
./pcpad -p /dev/ttyUSB0 -u http://btx.runningserver.com
