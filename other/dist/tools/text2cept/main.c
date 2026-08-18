 /*
   ####################################################################################
   #                                                                                  #
   #                        Bildschirmtricks cept2btx V2.0.0                          #
   #                                 text converter                                   #
   #                                                                                  #
   #    Copyright (C) 2008 Philipp Fabian Benedikt Maier (aka. Dexter)                #
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
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAXIMUM_WORD_LEN 1024		/* Maximum length a word in the text can have */

/* #################################################################################### */


/* #################################################################################### */
int main(int argc, char **argv)
{
	int readStatus;
	int textWidth;
	int lineCount;
	char character;
	char wordBuffer[MAXIMUM_WORD_LEN];
	FILE *sourceFile;

	printf("__________________________________________________________________\r\n");
	printf("TEXT to CEPT transformer\r\n");
	printf("Copyright (c) 2008 Philipp Fabian Benedikt Maier\r\n\n");

	if(argc == 3)
	{
		printf("* Input file: %s\r\n",argv[1]);

		textWidth = atoi(argv[2]);

		printf("* The selected width is: %i\r\n",textWidth);

		if(textWidth == 40)
		{
			printf("  ==> The selected Text-Width is 40, omitting <apd><apr> \r\n");
  			printf("      when end of line is touched\r\n");
		}
		else if(textWidth > 40)
		{
			printf("  ==> Invalid width! The maximum width on a cept screen is 40 -- aborting! \r\n");
			exit(0);
		}

		sourceFile = fopen(argv[1],"r");			
		if (sourceFile == NULL)
		{
			printf("  ==> Error: Failed to open source file\r\n");
			exit(0);		
		}

		/* Read header data */
		lineCount=0;
		do
		{

			readStatus = fscanf(sourceFile, "%s",wordBuffer);

			if(strlen(wordBuffer) > textWidth)
			{
				printf("\r\n\n * Error: The word ""%s"" is longer then the selected width!\r\n", wordBuffer);
				exit(0);
			}

			if(lineCount+strlen(wordBuffer) < textWidth)
			{
				printf("%s<sp>",wordBuffer);
				lineCount=lineCount+strlen(wordBuffer)+1;
			}
			else if(lineCount+strlen(wordBuffer) == textWidth)
			{
				if(textWidth < 40)
					printf("%s<apd><apr>\r\n",wordBuffer);
				if(textWidth == 40)
					printf("%s\r\n",wordBuffer);

				lineCount = 0;
			}
			else
			{
				printf("<apd><apr>\r\n");
				lineCount = 0;
			}


		}while(readStatus != EOF);

	
	}
	else
		printf("Usage: %s [sourcefile] [width]\n\r",argv[0]);


	printf("* Done!\r\n");
	return 0;
} 
/* #################################################################################### */

