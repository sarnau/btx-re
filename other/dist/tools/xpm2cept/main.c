 /*
   ####################################################################################
   #                                                                                  #
   #                        Bildschirmtricks cept2btx V2.0.0                          #
   #                                graphic converter                                 #
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

#define MAXIMUM_WIDTH 1024		/* Maximum Width Resulution an input file may have */
#define HEADER_DATA_BUFFER_SIZE 255	/* Header data buffer size */
/* #################################################################################### */


/* #################################################################################### */
/* Transform a 6 pixel matrix to the corespondenting G0 Character */
char g0Transform(char g11, char g12, char g21, char g22, char g31, char g32)
{
	int intermediateValue = 0;
	char result = 0;

	/*  Pixel Matrix:
	    g11 g12
	    g21 g22
	    g31 g32
	*/

	if(g11 != ' ')					/* Compute an intermideate value */
		intermediateValue |= 1;
	if(g12 != ' ')
		intermediateValue |= (1 << 1);
	if(g21 != ' ')
		intermediateValue |= (1 << 2);
	if(g22 != ' ')
		intermediateValue |= (1 << 3);
	if(g31 != ' ')
		intermediateValue |= (1 << 4);
	if(g32 != ' ')
		intermediateValue |= (1 << 5);


	if(intermediateValue == 0x3F)			/* Special case: All dots are active */
		result = '_';
	else if(intermediateValue <= 0x1F)		/* Calculate corespondenting ASCII value */
		result = intermediateValue + 0x20;
	else 
		result = intermediateValue + 0x40;


	if(!((result >= 0x20)&&(result <= 0x7E)))	/* This should really not happen! */
	{
		printf("\r\n\r\n  ==> Fatal Error: Calculating the corespondenting ascii value failed!");
		exit(0);
	}

	return result;
}

int main(int argc, char **argv)
{
	FILE *sourceFile;
	char line[3][MAXIMUM_WIDTH];
	char headerDataBuffer[HEADER_DATA_BUFFER_SIZE];
	char *headerDataBufferPointer;
	int freadReturn;
	int i,j;
	char transformResult;

	int imageWidth = 0;		/* Must be devideable by 2 */
	int imageHeight = 0;		/* Must be devideable by 3 */
	int imageColors = 0;		/* Should be 2 */

	printf("__________________________________________________________________\r\n");
	printf("XPM to CEPT-Mosaic (G1) transformer\r\n");
	printf("Copyright (c) 2008 Philipp Fabian Benedikt Maier\r\n\n");

	if(argc == 2)
	{
		printf("* Input file: %s\r\n",argv[1]);
	

		sourceFile = fopen(argv[1],"r");			
		if (sourceFile == NULL)
		{
			printf("  ==> Error: Failed to open source file\r\n");
			exit(0);		
		}

		/* Read header data */
		for(i=0;i<5;i++)
		{
			fgets(headerDataBuffer, HEADER_DATA_BUFFER_SIZE, sourceFile);
			if(headerDataBuffer[0] == '"')
			{
				headerDataBufferPointer = headerDataBuffer;
				headerDataBufferPointer++;

				if(atoi(headerDataBufferPointer) != 0)
				{
					imageWidth = atoi(headerDataBufferPointer);
					headerDataBufferPointer = strchr(headerDataBufferPointer, ' ');
					imageHeight = atoi(headerDataBufferPointer);
					headerDataBufferPointer++;
					headerDataBufferPointer = strchr(headerDataBufferPointer, ' ');
					imageColors = atoi(headerDataBufferPointer);
				}
			}
		}

		if((imageWidth <= 0)||(imageHeight <= 0)||(imageColors <= 0))
		{
			printf("  ==> Error: Header parse error, check file format!\r\n");
			exit(0);
		}


		printf("* Image width is: %ipx\r\n", imageWidth);
		
		if((imageWidth % 2) != 0)
		{
			printf("  ==> Error: The image does not fit, the width must be a multiple of 2\r\n");
			exit(0);
		}

		printf("* Image height is: %ipx\r\n", imageHeight);

		if((imageHeight % 3) != 0)
		{
			printf("  ==> Error: The image does not fit, the height must be a multiple of 3\r\n");
			exit(0);
		}

		printf("* Number of different colors: %i\r\n", imageColors);

		if(imageColors != 2)
			printf("  ==> Warning: The image has multiple colors, this may cause unexpected results!\r\n");


		if(imageColors > 2)	/* Override color definition if the image has more than 2 colors */
			for(i=0;i<5;i++)
				fgets(headerDataBuffer, HEADER_DATA_BUFFER_SIZE, sourceFile);


		/* Read header data */
		printf("* Transforming...\r\n");

		printf("* The result is:\r\n\n");
		printf("----------------------------------8<--------------------------------\r\n\n\n");

		for(j=0;j<imageHeight;j+=3)
		{
			memset(line[0],0,MAXIMUM_WIDTH);			/* Clear buffer before use */
			memset(line[1],0,MAXIMUM_WIDTH);
			memset(line[2],0,MAXIMUM_WIDTH);

			freadReturn =  (int)fgets(line[0], MAXIMUM_WIDTH, sourceFile);
			freadReturn += (int)fgets(line[1], MAXIMUM_WIDTH, sourceFile);
			freadReturn += (int)fgets(line[2], MAXIMUM_WIDTH, sourceFile);

			for(i=1;i<=imageWidth;i+=2)
			{
				transformResult = g0Transform(line[0][i],line[0][i+1],line[1][i],line[1][i+1],line[2][i],line[2][i+1]);

				if(transformResult == ' ')
					printf("<sp>");				/* Replace a space with the btxml tag <sp> */
				else
					printf("%c", transformResult);
			}
			printf("<apd><apr>\n\r");

		}


		printf("\r\n\r\n----------------------------------8<--------------------------------\r\n\n");
		printf("* Image will take a space of %ix%i characters on the screen.\r\n", imageWidth/2,imageHeight/3);
	
	}
	else
		printf("Usage: %s [sourcefile]\n\r",argv[0]);


	printf("* Done!\r\n");
	return 0;
} 
/* #################################################################################### */

