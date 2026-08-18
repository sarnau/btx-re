/*
   ####################################################################################
   #                                                                                  #
   #                                  BTXASM V1.0.0                                   #
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
	printf("    %s -p [PORT] -i [BTXML-INPUT] -o [BINARY-OUTPUT]-f\r\n",programName);
	printf("\r\n");
}

/* Main program: Perform some testpages */
int main(int argc, char *argv[])
{
	int getoptOption;
	char *getoptBtxPage = 0;
	char *getoptBtxPageBinary = 0;
	int getoptVerboseMode = 0;
	char ceptHypertextBuffer[BTX_CEPT_HYPERTEXT_BUFFERSIZE];

	int i;

	btxCeptPageData btxCeptPage;	/* Contains the page information of the last displayed page */

	strcpy(programName,basename(argv[0]));

	printf("_______________________________________________________________________________\n\r");
	printf("Bildschirmtrix BTX Assembler V.1.0\n\r");
	printf("Copyright(c) 2014 Philipp Fabian Benedikt Maier\n\r");
	printf("\n\r");

  	/* Parse Options */
	while ((getoptOption = getopt (argc, argv, ":h?i:o:v")) != -1)
		switch (getoptOption)
		{
			case 'h':
				showUsage();
				return 0;
			case '?':
				showUsage();
				return 0;
			case 'v':
				getoptVerboseMode = 1;
			break;
			case 'i':
				getoptBtxPage = optarg;
			case 'o':
				getoptBtxPageBinary = optarg;
			break;
		}


	/* Check input file */
	if(getoptBtxPage)
	{
		printf(" * Path to BTXML BTX-Page is: %s\r\n", getoptBtxPage);
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

	printf(" * Path to BINARY BTX-Page is: %s\r\n", getoptBtxPageBinary);

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

	printf(" * Writing output file...\n"); 
	if(storeFile(btxCeptPage.ceptPage, btxCeptPage.ceptPageLength, getoptBtxPageBinary) != -1)
		printf(" * Info: Outputfile stored successfully!\r\n");
	else
	{
		printf(" * Error: Writing output file failed!\r\n");
		exit(1);
	}
	printf(" * Done!\n\r");	

	printf("\n\r");
	return 0;
}
/* #################################################################################### */

