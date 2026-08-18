/*
   ####################################################################################
   #                                                                                  #
   #                         Bildschirmtricks miniBTX V1.0.0                          #
   #                      a (poor) cept protocol implementation                       #
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
#include <stdlib.h>
#include <strings.h>
#include <ttybanana.h> 				/* TTY-Functions of libCodebananas */
#include <toolbanana.h> 			/* Tool-Functions of libCodebananas */
#include <unistd.h>
#include <string.h>

#include "cept.h"				/* Include own header file */
/* #################################################################################### */


/* #################################################################################### */

/* Query CEPT terminal */
void systemCeptQuery(char *string, int length, char *response, char *port, int linkLayer)
{
	int i;
	int crc16 = 0;

	memset(response, 0x00, sizeof(response));

	printf("\n\r\n\rPerforming terminal query:\n\r");

	if(length == 0)
		printf("* Length of transmission data is null - omitting transmission pass ...\n\r");
	else
	{
	
		if(linkLayer == CEPT_LINKLAYER)
			printf("* Transmitting CEPT data block (linklayer secured) ...\n\r");
		else
			printf("* Transmitting CEPT data block ...\n\r");
	
	
		if(linkLayer == CEPT_LINKLAYER)
		{
			printf("* Sending STX 0x%02x...\n\r", CEPT_STX);
			ttyPutchar(port,CEPT_STX);			/* Transmit STX-Token, form now on the transmission begins */
		}
	
		printf("* Sending Data...\n\r");
		hexBinAsciiDump(string,length);
	
		for(i=0;i<length;i++)					/* Transmit data and calculate CRC */
		{
			ttyPutchar(port,string[i]);
			crc16 = crcComputationCycle(string[i], 0xA001, crc16);	
		}
	
		if(linkLayer == CEPT_LINKLAYER)
		{
			crc16 = crcComputationCycle(CEPT_ETX, 0xA001, crc16);	/* Send the STX token - the token goes with in the CRC-calculation */
		
			printf("* Sending ETX 0x%02x...\n\r", CEPT_ETX);
			ttyPutchar(port,CEPT_ETX);
		
			printf("* Adding lower CRC16 byte %02hhx...\n\r", (char)crc16);
			ttyPutchar(port,(char)crc16);			/* Transmit the CRC */
		
			printf("* Adding higher CRC16 byte %02hhx...\n\r", (char)(crc16 >> 8));
			ttyPutchar(port,(char)(crc16 >> 8));
		}
	}

	printf("* Receiving response...\n\r");
	usleep(CEPT_RESPONSE_TIMOUT);					/* Wait some time to be sure that some response data is already in buffer when we start receiving) */
	ttyReadNonblocking(port, response, CEPT_RESPONSE_TIMOUT);	/* Read the data */

	if(response[0] == '\0')
		printf("   (no data received)\n\r");
	else
		hexBinAsciiDump(response,strlen(response));

	printf("done!\n\r");
	fflush(stdout);

	return;
}

/* Filter keyboard input from terminal response (Rip off non printable chars and known protocol patterns) */
void systemCeptFilterKeyboardinput(char *responsedata, char *keyboarddata)
{
	printf("\n\r\n\rFilter keyboard-data from Transmission response\n\r");

	char *keyboarddataPointer;
	char *responsedataPointer;
	char *keyboarddataAheadPointer;

	printf("* Ripping protocol fragments...\n\r");

	keyboarddataPointer = keyboarddata;
	responsedataPointer = responsedata;

	do
	{
		/* Rip off the CEPT_DLE, 0x30/0x31 acknoledgement token */
		if((*responsedataPointer == CEPT_DLE)&&((*(responsedataPointer+1) == 0x30)||(*(responsedataPointer+1) == 0x31)))
			responsedataPointer+=2;
		else
		{
			/* Replace Terminator and Initiator with printable characters */
			if(*responsedataPointer == CEPT_INI)
				*keyboarddataPointer = '*';
			else if(*responsedataPointer == CEPT_TER)
				*keyboarddataPointer = '#';
			else		
				*keyboarddataPointer = *responsedataPointer;
	
			/* Note: In a serious Videotex application you should NOT replace terminator and initiator
				with thier printable pendants to avoid missunderstandings. */
	
			keyboarddataPointer++;
			responsedataPointer++;
		}

	} while(*responsedataPointer != '\0');
	*keyboarddataPointer = '\0';


	printf("* Ripping non printable characters...\n\r");

	keyboarddataPointer = keyboarddata;
	keyboarddataAheadPointer = keyboarddata;	

	do
	{
		if((*keyboarddataAheadPointer >= 0x20)&&(*keyboarddataAheadPointer <= 0x7E))
		{
			*keyboarddataPointer = *keyboarddataAheadPointer;
			keyboarddataPointer++;
		}

		keyboarddataAheadPointer++;

	} while(*keyboarddataAheadPointer != '\0');
	*keyboarddataPointer = '\0';


	printf("* The remaining data is:\n\r");
	if(keyboarddata[0] == '\0')
		printf("   (the string contained no keyboard data)\n\r");
	else
		hexBinAsciiDump(keyboarddata,strlen(keyboarddata));

	fflush(stdout);
	return;
}
/* #################################################################################### */
