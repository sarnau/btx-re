/*
   ####################################################################################
   #                                                                                  #
   #                          Bildschirmtricks pcPAD V1.0.0                           #
   #                                   Main program                                   #
   #                                                                                  #
   #    Copyright (C) 2008-2014 Philipp Fabian Benedikt Maier (aka. Dexter)           #
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
   #################################################################################### */


/* ## HEADER ########################################################################## */
#include <stdio.h>
#include <stdlib.h>
#include <libcodebananas/ttybanana.h>
#include <libcodebananas/confbanana.h>
#include <libcodebananas/toolbanana.h> 
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <getopt.h>
#include "pad.h"
#include "config.h"

char programName[255];

/* #################################################################################### */

/* #################################################################################### */

/* Print copyright/programname headline */
void printHeadline(void)
{
	printf("\n\a");
	printf("      ------------------\n");
	printf("      |  PPPPP   CCCC  |\n");
	printf("      |  P    P C      |\n");
	printf("      |  P    P C      |\n");
	printf("      |  PPPPP  C      |\n");
	printf("      |  P      C      |\n");	
	printf("      |  P       CCCC  |      PPPPP     AAAAA    DDDDD\n");
	printf("      ------------------      PP  PP   AA   AA   DD  DDD\n");
	printf("     oooooooooooooooooooo     PPPPP    AAAAAAA   DD   DD\n");
	printf("    oooooooooooooooooooooo    PP       AA   AA   DD   DD\n");
	printf("   oooooooooooooooooooooooo   PP       AA   AA   DDDDDDD\n");
	printf("  ******************************************************\n");
	printf("  *\t\t\t\t\t\t       *\n");
	printf("  *\t      ******************************\t       *\n");
	printf("  *\t      *         **********         *\t       *\n");
	printf("  *\t      *       *\t\t   *       *\t       *\n");
	printf("  *\t      *     *\t\t     *     *\t       *\n");
	printf("  *         *   *** *\t\t     * ***   *         *\n");
	printf("  *       *   *   * *\t\t     * *   *   *       *\n");
	printf("    *   *   *     * *\t\t     * *     *   *   *\n");
	printf("      *   *       * *\t\t     * *       *   *\n");
	printf("\t*\t  *   *\t\t   *   *         *\n");
	printf("\t*\t    *   **********   *           *\n");
	printf("\t*\t      **************             *\n");
	printf("\t*\t\t\t\t\t *\n");
	printf("\t*\t\t\t\t\t *\n");
	printf("\t*\t\t\t\t\t *\n");
	printf("\t*\t\t\t\t\t *\n");
	printf("\t******************************************\n");
	printf("\n");
	printf("_______________________________________________________________________________\n\r");
	printf("Bildschirmtrix Videotex - pcPAD - PAD for PC %s\n\r",VERSIONSTRING);
	printf("Copyright (c) 2008-2014 Philipp Fabian Benedikt Maier\n\r");
}

/* Print online help (Commandline mode) */
void printHelp(void)
{
	printHeadline();
	printf("\n");
	printf(" * Parameters:\n");
	printf("   -h or -? ............ Print this screen\n");
	printf("   -p [PATH] ........... Serial port e.g. /dev/ttyUSB0\n");
	printf("   -u [PATH] ........... ULM Directory (BTX-Pages)\n");

	printf("\n");
	exit(0);
}

int main(int argc, char *argv[])
{
	int getoptOption;
	char *getoptPort = "/dev/ttyUSB0";
	char *getoptULM = "./ulm";

	/* Global var with program name */
	strcpy(programName,basename(argv[0]));

	while ((getoptOption = getopt (argc, argv, "h?p:u:")) != -1)
		switch (getoptOption)
		{
			case 'h':printHelp();
			break;
			case '?':printHelp();
			break;
			case 'p':getoptPort = optarg;
			break;
			case 'u':getoptULM = optarg;
			break;
		}

	printHeadline();
	applicationBtx(getoptULM,getoptPort);
	return 0;
} 
/* #################################################################################### */

