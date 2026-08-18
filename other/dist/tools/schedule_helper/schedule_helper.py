#!/usr/bin/python

# This is a python script that helps a bit with parsing 
# the schedule files from the ccc events

from xml.dom.minidom import parse
import xml.dom.minidom

# Open XML document using minidom parser
DOMTree = xml.dom.minidom.parse("schedule.xml")
schedule = DOMTree.documentElement

# Get all the events in the schedule
events = schedule.getElementsByTagName("event")

# Select the day to process:
# 13 = day 1
# 14 = day 2
# 15 = day 3
# 16 = day 4
# 17 = day 5
day2process="17"

room2proces="Simulacron-3"
roomColor="<mgb>"

#room2proces="Project 2501"
#roomColor="<cnb>"


print room2proces
print "==============================="
for event in events:
	if event.hasAttribute("id"):

		date = event.getElementsByTagName('date')[0]
		title = event.getElementsByTagName('title')[0]
		room = event.getElementsByTagName('room')[0]
		start = event.getElementsByTagName('start')[0]
		day = date.childNodes[0].data[8:10]
		if((day == day2process) and (room.childNodes[0].data == room2proces)):
			line = roomColor + "<sp><trb><grb>" + start.childNodes[0].data + "<trb> " + title.childNodes[0].data + "<apd><apr>"
			line = line.replace (" ", "<sp>")

			print line

