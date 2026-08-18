#!/usr/bin/python

####################################################################################
#                                                                                  #
#                                Bildschirmtrix                                    #
#                           Image to DRCS converter                                #
#                                                                                  #
#    Copyright (C) 2014 Philipp Fabian Benedikt Maier                              #
#                                                                                  #
#    This program is free software; you can redistribute it and/or modify          #
#    it under the terms of the GNU General Public License as published by          #
#    the Free Software Foundation; either version 2 of the License, or             #
#    (at your option) any later version.                                           #
#                                                                                  #
#    This program is distributed in the hope that it will be useful,               #
#    but WITHOUT ANY WARRANTY; without even the implied warranty of                #
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 #
#    GNU General Public License for more details.                                  #
#                                                                                  #
#    You should have received a copy of the GNU General Public License             #
#    along with this program; if not, write to the Free Software                   #
#    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA    #
#                                                                                  #
####################################################################################


## HEADER ##########################################################################
from PIL import Image
import sys
import getopt

DRCS_DSIZE=6
DRCS_YSIZE=10

# Note: The way, the tile scanning/translating is done is not fully correct. If
#       the tile fits not in the 6 pixel with raster. The scanning should just
#       break continue in the next line. This is not correctly implemented here.
#       The program expects a tile that has a multiple of 6 width.
#
#       The program works fine with 6x10 and 12x10 tiles. If you need other
#       formats, you will have to rewrite the scanning algorithm implemented
#       in convertImageTile()

####################################################################################


####################################################################################

# Transform the png color value binary value
def colorBitDecision(rgbcode, invert):

	if invert:
		if rgbcode[0] < 125 and rgbcode[1] < 125 and rgbcode[2] < 125:
			return 0;
		else:
			return 1;
	else:
		if rgbcode[0] < 125 and rgbcode[1] < 125 and rgbcode[2] < 125:
			return 1;
		else:
			return 0;

# Transform a DRCS-Line (6 bit vector) to a "D-byte"
def drcsline2dbyte(drcsline):

	if   drcsline == [0,0,0,0,0,0]:
		return "<0x40>"
	elif drcsline == [0,0,0,0,0,1]:
		return "<0x41>"
	elif drcsline == [0,0,0,0,1,0]:
		return "<0x42>"
	elif drcsline == [0,0,0,0,1,1]:
		return "<0x43>"
	elif drcsline == [0,0,0,1,0,0]:
		return "<0x44>"
	elif drcsline == [0,0,0,1,0,1]:
		return "<0x45>"
	elif drcsline == [0,0,0,1,1,0]:
		return "<0x46>"
	elif drcsline == [0,0,0,1,1,1]:
		return "<0x47>"
	elif drcsline == [0,0,1,0,0,0]:
		return "<0x48>"
	elif drcsline == [0,0,1,0,0,1]:
		return "<0x49>"
	elif drcsline == [0,0,1,0,1,0]:
		return "<0x4a>"
	elif drcsline == [0,0,1,0,1,1]:
		return "<0x4b>"
	elif drcsline == [0,0,1,1,0,0]:
		return "<0x4c>"
	elif drcsline == [0,0,1,1,0,1]:
		return "<0x4d>"
	elif drcsline == [0,0,1,1,1,0]:
		return "<0x4e>"
	elif drcsline == [0,0,1,1,1,1]:
		return "<0x4f>"

	elif drcsline == [0,1,0,0,0,0]:
		return "<0x50>"
	elif drcsline == [0,1,0,0,0,1]:
		return "<0x51>"
	elif drcsline == [0,1,0,0,1,0]:
		return "<0x52>"
	elif drcsline == [0,1,0,0,1,1]:
		return "<0x53>"
	elif drcsline == [0,1,0,1,0,0]:
		return "<0x54>"
	elif drcsline == [0,1,0,1,0,1]:
		return "<0x55>"
	elif drcsline == [0,1,0,1,1,0]:
		return "<0x56>"
	elif drcsline == [0,1,0,1,1,1]:
		return "<0x57>"
	elif drcsline == [0,1,1,0,0,0]:
		return "<0x58>"
	elif drcsline == [0,1,1,0,0,1]:
		return "<0x59>"
	elif drcsline == [0,1,1,0,1,0]:
		return "<0x5a>"
	elif drcsline == [0,1,1,0,1,1]:
		return "<0x5b>"
	elif drcsline == [0,1,1,1,0,0]:
		return "<0x5c>"
	elif drcsline == [0,1,1,1,0,1]:
		return "<0x5d>"
	elif drcsline == [0,1,1,1,1,0]:
		return "<0x5e>"
	elif drcsline == [0,1,1,1,1,1]:
		return "<0x5f>"

	elif drcsline == [1,0,0,0,0,0]:
		return "<0x60>"
	elif drcsline == [1,0,0,0,0,1]:
		return "<0x61>"
	elif drcsline == [1,0,0,0,1,0]:
		return "<0x62>"
	elif drcsline == [1,0,0,0,1,1]:
		return "<0x63>"
	elif drcsline == [1,0,0,1,0,0]:
		return "<0x64>"
	elif drcsline == [1,0,0,1,0,1]:
		return "<0x65>"
	elif drcsline == [1,0,0,1,1,0]:
		return "<0x66>"
	elif drcsline == [1,0,0,1,1,1]:
		return "<0x67>"
	elif drcsline == [1,0,1,0,0,0]:
		return "<0x68>"
	elif drcsline == [1,0,1,0,0,1]:
		return "<0x69>"
	elif drcsline == [1,0,1,0,1,0]:
		return "<0x6a>"
	elif drcsline == [1,0,1,0,1,1]:
		return "<0x6b>"
	elif drcsline == [1,0,1,1,0,0]:
		return "<0x6c>"
	elif drcsline == [1,0,1,1,0,1]:
		return "<0x6d>"
	elif drcsline == [1,0,1,1,1,0]:
		return "<0x6e>"
	elif drcsline == [1,0,1,1,1,1]:
		return "<0x6f>"

	elif drcsline == [1,1,0,0,0,0]:
		return "<0x70>"
	elif drcsline == [1,1,0,0,0,1]:
		return "<0x71>"
	elif drcsline == [1,1,0,0,1,0]:
		return "<0x72>"
	elif drcsline == [1,1,0,0,1,1]:
		return "<0x73>"
	elif drcsline == [1,1,0,1,0,0]:
		return "<0x74>"
	elif drcsline == [1,1,0,1,0,1]:
		return "<0x75>"
	elif drcsline == [1,1,0,1,1,0]:
		return "<0x76>"
	elif drcsline == [1,1,0,1,1,1]:
		return "<0x77>"
	elif drcsline == [1,1,1,0,0,0]:
		return "<0x78>"
	elif drcsline == [1,1,1,0,0,1]:
		return "<0x79>"
	elif drcsline == [1,1,1,0,1,0]:
		return "<0x7a>"
	elif drcsline == [1,1,1,0,1,1]:
		return "<0x7b>"
	elif drcsline == [1,1,1,1,0,0]:
		return "<0x7c>"
	elif drcsline == [1,1,1,1,0,1]:
		return "<0x7d>"
	elif drcsline == [1,1,1,1,1,0]:
		return "<0x7e>"
	elif drcsline == [1,1,1,1,1,1]:
		return "<0x7f>"

	print ""
	print " * Error: Invalid bit pattern."
	exit(1)

# Convert one image tile to a drcs character
def convertImageTile(filename, verbose, xoffset, yoffset, invert, multiplier):
	try:
		image = Image.open(filename) #Can be many different formats.
		drcsmap = image.load()
	except:
		print " * Error: Could not load specified image - abort"
		exit(1)

	# Determine character code
	if verbose:
		print " * CEPT Pattern coding is:"
	patterncode = "<0x30>"

	# Go through all character lines and transform each line into a d-byte (defines 6 pixels) code character 
	for k in range (0,DRCS_YSIZE):

		for j in range (0,multiplier):
			# Go through the image and extract the next DRCS_DSIZE bits
			drcsline = []
			previewline = ""
			for i in range (0,DRCS_DSIZE):
				if i+xoffset+DRCS_DSIZE*j < image.size[0] and k+yoffset < image.size[1]:
					if colorBitDecision(drcsmap[i+xoffset+DRCS_DSIZE*j,k+yoffset],invert):
						drcsline.extend([1])
						previewline = previewline + "#"
					else:
						drcsline.extend([0])
						previewline = previewline + "_"
				else:
					drcsline.extend([0])

			# Lookup the matching d-byte character
			patterncode = patterncode + drcsline2dbyte(drcsline)

			# Print verbose information 
			if verbose:
				print "  ", previewline, drcsline, "==>", drcsline2dbyte(drcsline),

		if verbose:
			print ""

	return(patterncode)

# Split image into tiles, then convert each tile into a drcs character
def convertImage(filename, verbose, invert, multiplier):
	try:
		image = Image.open(filename) #Can be many different formats.
		drcsmap = image.load()
	except:
		print " * Error: Could not load specified image - abort"
		exit(1)

	# Check if character size is correct, currently we only support 6x10 characters 
	if verbose:
		print " * Image size is:", image.size[0] , "x", image.size[1]

	# Calculate tile geometry
	xTiles = int(round((float(image.size[0]) / (DRCS_DSIZE * multiplier)))+0.5)
	yTiles = int(round((float(image.size[1]) / DRCS_YSIZE))+0.5)

	if verbose:
		print " * Tile geometry:"
		print "   X:", xTiles, "tiles" 
		print "   Y:", yTiles, "tiles"

	for k in range (0,yTiles):
		for i in range(0,xTiles):
			if verbose:
				print ""
			patterncode = convertImageTile(filename, verbose, i * DRCS_DSIZE * multiplier, k * DRCS_YSIZE, invert, multiplier)
			if verbose:
				print ""
				print " * Final result:"
				print "  ",
			print patterncode


def printHeadline():
	print "_______________________________________________________________________________"
	print "Bildschirmtrix picture to DRCS converter V.1.0"
	print "Copyright(c) 2014 Philipp Fabian Benedikt Maier"
	print ""

def printHelp():
	printHeadline()
	print " * Parameters:"
	print "   -h .................. Print this screen"
	print "   -v .................. Verbose mode - prints debug information"
	print "   -i [FILE] ........... image input file"
	print "   -x [INT] ............ X-Offset"
	print "   -y [INT] ............ Y-Offset"
	print "   -b .................. Bulk mode, convert a full image into drcs tile"
	print "   -n .................. Negative (Invert)"
	print "   -m [INT] ............ Width Multiplier (1 or 2, default is 1)"
	print ""
	exit(0)

def main():
	verbose = False
	xoffset = 0
	yoffset = 0
	bulk = 0
	image = 0
	invert = 0
	multiplier = 1

	try:
		opts, args = getopt.getopt(sys.argv[1:], "hi:vx:y:bnm:", ["help", "image=","verbose","xoffset=","yoffset=","bulk","invert","multiply="])
	except getopt.GetoptError as err:
		print str(err)
		printHelp()
		exit(1)
	output = None
	verbose = False
	for o, a in opts:
		if o == "-v":
			verbose = True
		elif o in ("-h", "--help"):
			printHelp()
			exit(0)
		elif o in ("-i", "--image"):
			image = a
		elif o in ("-x", "--xoffset"):
			xoffset = int(a)
		elif o in ("-y", "--yoffset"):
			yoffset = int(a)
		elif o in ("-b", "--bulk"):
			bulk = 1
		elif o in ("-n", "--invert"):
			invert = 1
		elif o in ("-m", "--multiply"):
			multiplier = int(a)
		else:
			assert False, "invalid option"

	if verbose:
		printHeadline()
	
	if bulk:
		convertImage(image, verbose, invert, multiplier)
	else:
		patterncode = convertImageTile(image, verbose, xoffset, yoffset, invert, multiplier)
		if verbose:
			print ""
			print " * Final result:"
			print "  ",
		print patterncode

if __name__ == "__main__":
    main()
####################################################################################

