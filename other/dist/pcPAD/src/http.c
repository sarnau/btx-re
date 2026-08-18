/*
   ####################################################################################
   #                                                                                  #
   #                          Bildschirmtricks pcPAD V1.0.0                           #
   #                               http client layer                                  #
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

#include "config.h"			/* Include btx configuration */
#include "http.h"			/* include own header file */
#include <limits.h>
#include <string.h>
#include <libcodebananas/toolbanana.h>

/* Note: This is stub implementation that only pretends to do something with http,
         in reality it just loads the files from the filesystem. This is because
	 this software is ported from the mikroPAD firmware. Maybe one day i will
	 implement a real http engine here to have it networked again. For now
	 reading the pages from filesystem is ok. */

/* #################################################################################### */


/* ## HTTP CLIENT ##################################################################### */

/* Download  */
static int wget(char *url)
{
	FILE *fd;
	char result[255];
	char sprintfBuffer[255];
	int rc;

	/* Prepare wget commandline */
	sprintf(sprintfBuffer,"wget %s -O %s 2>&1",url,BTX_ULM_WGET_TEMP);

	/* Open arfcncalc */
	fd = popen(sprintfBuffer, "r");
	if (fd == NULL)
	{
		printf(" * Error: Unable to execue wget, make sure that this\n");
		printf("          utility. is installed properly. -- exiting.\n");
		return -1;
	}

	/* Read the output a line at a time - output it. */
	while (fgets(result, sizeof(result)-1, fd) != NULL)
	{
		if(strstr(result, "awaiting response"))
		{
			strCutOut("... ", "", result);
			trimString(result);
			replaceChar(result, ' ', '\0', sizeof(result));
			rc = atoi(result);

			/* When atoi() fails to convert, something is odd,
			   we better set the returncode to -1 then */
			if(rc == 0)
				rc = -1;
		}
	}

	/* Close arfcncalc */
	pclose(fd);

	return rc;
}

/* Fetch CEPT-Hypertext from local filesystem */
static int applicationBtxHttpGetFromFilesystem(char *filePath, char *data, int *status)
{
	*status = 200;

	printf(" * Path to selected BTX-Page is: %s (local)\n", filePath);
	/* Check whether the file is readable */
	if(testFile(filePath) != 0)
	{
		printf(" * Error: Can not read BTX-Page from file system - aborting with error 404!\n");
		*status = 404;
		return 0;
	}

	/* Zero out the buffer befor use */
	memset(data,0,BTX_CEPT_HYPERTEXT_BUFFERSIZE);

	/* Load Hypertext file into buffer */
	if(loadFile(data, BTX_CEPT_HYPERTEXT_BUFFERSIZE, filePath) != -1)
		printf(" * Info: BTX-Hypertext loaded successfully...\n");
	else
	{
		printf(" * Error: Could not load BTX-Hypertext page - aborting with error 100!\n");
		*status = 100;
		return -1;
	}

	return 0;
}

/* Fetch CEPT-Hypertext from a remote webservr */
static int applicationBtxHttpGetFromWebserver(char *url, char *data, int *status)
{
	printf(" * Path to selected BTX-Page is: %s (remote)\n", url);

	/* Download btx page from ULM via http */
	*status = wget(url);

	printf("   wget returncode: %i\n",*status);

	/* Stop here, on serious erros */	
	if(*status == -1)
		return -1;

	/* Stop on server errors */
	if(*status != 200)
		return 0;

	/* Now load the temporary file from filesystem */
	return applicationBtxHttpGetFromFilesystem(BTX_ULM_WGET_TEMP, data, status);
}

/* Download CEPT-Hypertext Page from Ulm or from the local filesystem */
int applicationBtxHttpGet(char *url, char *ulmAddr , char *data, int *status)
{
	char path[PATH_MAX];

	printf(" * Loading BTX page from ULM...\n");
	printf("   url = %s\n",url);
	printf("   ulmAddr = %s\n",ulmAddr);

	strcpy(path,ulmAddr);
	strcat(path,url);

	/* Path references an url on a webserver */
	if(strstr(path, "http://"))
		return applicationBtxHttpGetFromWebserver(path , data, status);
	/* Path references a file on the local filesystem */
	else
		return applicationBtxHttpGetFromFilesystem(path , data, status);

	return -1;
} 
/* #################################################################################### */


