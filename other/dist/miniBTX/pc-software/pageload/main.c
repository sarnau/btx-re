/*
   ####################################################################################
   #                                                                                  #
   #                             HeardbreakChip V1.0.0                                #
   #                                  Main-Program                                    #
   #                                                                                  #
   #    Copyright (C) 2010 Philipp Fabian Benedikt Maier (aka. Dexter)                #
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

/* Header */
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <libcodebananas/ttybanana.h> 
#include <libcodebananas/toolbanana.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include <unistd.h>
#include <libgen.h>

#include "config.h"
#include "cept.h"

char programName[255];

/* Transmission parameters */
#define MINIBTX_HOOKOFF 0x01				/* Hookoff-token (Do not change) */
#define MINIBTX_CONNECT 0x00				/* Connect-token (Do not change) */
#define MINIBTX_BAUDRATE 1200				/* Baudrate, can not be changed */

/* #################################################################################### */


/* #################################################################################### */
void showUsage(void)
{
	printf(" * Usage:\r\n");
	printf("    %s -p [PORT] -b [BTX-PAGE] -f\r\n",programName);
	printf("    -f = fakeinput\r\n");
	printf("\r\n");
}

/* Main program: Perform some testpages */
int main(int argc, char *argv[])
{
	int getoptOption;
	char *getoptPort = 0;
	char *getoptBtxPage = 0;
	int getoptVerboseMode = 0;
	int getoptFakeInput = 0;
	char ceptHypertextBuffer[BTX_CEPT_HYPERTEXT_BUFFERSIZE];
	char getchar;


	int i;

	btxCeptPageData btxCeptPage;	/* Contains the page information of the last displayed page */

	strcpy(programName,basename(argv[0]));

	printf("_______________________________________________________________________________\n\r");
	printf("Pageload for miniBTX V.1.0\n\r");
	printf("Copyright(c) 2010 Philipp Fabian Benedikt Maier\n\r");
	printf("\n\r");

  	/* Parse Options */
	while ((getoptOption = getopt (argc, argv, ":h?p:b:vf")) != -1)
		switch (getoptOption)
		{
			case 'h':
				showUsage();
				return 0;
			case '?':
				showUsage();
				return 0;
			case 'p':
				getoptPort = optarg;
			break;
			case 'v':
				getoptVerboseMode = 1;
			break;
			case 'f':
				getoptFakeInput = 1;
			break;
			case 'b':
				getoptBtxPage = optarg;
			break;
		}


	/* Check whether there is a serial port specified */
	if(getoptPort)
		printf(" * Serial port is: %s\r\n", getoptPort);
	else
	{
		printf(" * Error: You must specify a serial port!\r\n");
		printf("\r\n");
		exit(1);
	}

	/* Check whether there is a BTX-Page specified */
	if(getoptBtxPage)
	{
		printf(" * Path to selected BTX-Page is: %s\r\n", getoptBtxPage);
		/* Check whether the file is readable */
		if(testFile(getoptBtxPage) != 0)
		{
			printf(" * Error: Can not read BTX-Page from file system - aborting!\r\n");
			printf("\r\n");
			exit(1);
		}
	}
	else
	{
		printf(" * Error: You must specify a BTX-Page!\r\n");
		printf("\r\n");
		exit(1);
	}

	/* Load Hypertext file into buffer */
	if(loadFile(ceptHypertextBuffer, sizeof(ceptHypertextBuffer), getoptBtxPage) != -1)
		printf(" * Info: BTX-Hypertext loaded successfully...\r\n");
	else
	{
		printf(" * Error: Could not load BTX-Hypertext page - aborting!\r\n");
		printf("\r\n");
		exit(1);
	}

	/* In verbose mode display hypertext page on the screen.. */
	if(getoptVerboseMode)
	{
		printf(" * Info: The following BTX-Hypertext has been loaded:\r\n");
		printf("%s\r\n",ceptHypertextBuffer);
	}

	/* Parse hypertext page */
	if(applicationBtxCeptParse(ceptHypertextBuffer, &btxCeptPage) != 1)
		printf(" * Info: BTX-Hypertext parsed successfully...\r\n");
	else
	{
		printf(" * Error: Could not parse BTX-Hypertext page - aborting!\r\n");
		printf("\r\n");
		exit(1);
	}

	/* In verbose mode display additional information: hexdump, metatags ect.. */
	if(getoptVerboseMode)
	{
		printf(" * Info: The page contains the following meta information (has no effect here)\r\n");

		for(i=0; i<btxCeptPage.metaTagCount; i++)
			printf("   %s = %s\r\n", btxCeptPage.metaName[i], btxCeptPage.metaContent[i]);


		printf(" * Info: The following binary data (%i bytes) will be transmitted to therminal:\r\n",  btxCeptPage.ceptPageLength);
		hexBinAsciiDump(btxCeptPage.ceptPage, btxCeptPage.ceptPageLength);
	}


	printf(" * Opening serial port...\n\r");
	ttyInitProf(getoptPort,MINIBTX_BAUDRATE,8,NONE);			/* Initalize tty */
	ttyClearBuffer(getoptPort);						/* Clear buffer */

	printf(" * Serial port is set to %s at %i Baud\n\r",getoptPort,MINIBTX_BAUDRATE);
	printf(" * Transmitting page...\n\r");
	ttyWriteLen(getoptPort, btxCeptPage.ceptPage, btxCeptPage.ceptPageLength);

	if(getoptFakeInput)
	{
		printf(" * Input from terminal: ");
		while(1)
		{
			fflush(stdout);

			getchar = ttyGetchar(getoptPort);

			if(getchar == CEPT_TER)
				getchar = '#';
			else if(getchar == CEPT_INI)
				getchar = '*';

			printf("%c",getchar);

			ttyPutchar(getoptPort,getchar);

			if(getchar == '#')
			{
				printf("\r\n * Terminator (#) caught, exiting...\n\r");
				printf(" * Done!\n\r");	
				exit(0);
			}
		}
	}
	
	printf(" * Done!\n\r");	

	printf("\n\r");
	return -1;
}
/* #################################################################################### */

