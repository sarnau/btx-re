/*
   ####################################################################################
   #                                                                                  #
   #                         Bildschirmtricks miniBTX V2.0.0                          #
   #                                  Main-Program                                    #
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

/* Header */
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <ttybanana.h> 
#include <toolbanana.h>
#include <string.h>

#include "cept.h"					/* Include CEPT protocol layer */

/* Transmission parameters */
#define MINIBTX_HOOKOFF 0x01				/* Hookoff-token (Do not change) */
#define MINIBTX_CONNECT 0x00				/* Connect-token (Do not change) */
#define MINIBTX_LINKLAYER CEPT_NOLINKLAYER		/* Configrue linklayer */
#define MINIBTX_BAUDRATE 1200				/* Baudrate, can not be changed */
#define MINIBTX_PORT "/dev/ttyUSB1"			/* Serial port - change if needed */

/* Special parameters */
#define MINIBTX_FORCE_TERMINAL_KEYBOARD_DATA 0		/* Force the terminal to transmit the keyboard data by sending DCT (1=ON,0=OFF) */

/* Note: The software has been tested with the following terminals:
		* BtxTV (MINIBTX_FORCE_TERMINAL_KEYBOARD_DATA should be set to 1)
		* Multitel11/Bitel */
 
/* #################################################################################### */


/* #################################################################################### */


/* Main program: Perform some testpages */
int main(void)
{
	char transmissionBuffer[255];			/* Here we store the data that is sent to the terminal */
	char trsnsmissionResponseBuffer[255];		/* Here we store the date that comes from the terminal */
	char keyboardInput[255];			/* The users keyboard input */

	printf("_____________________________________________________________________________________________\n\r");
	printf("MiniBTX Copyright(c) Philipp Fabian Benedikt Maier\n\r");
	printf("\n\r");
	printf("Note: This is a small example application to explore some features of CEPT/BTX Terminals. \n\r");
	printf("      Please notify that this tools requies the miniBTX hardware. \n\r");
	printf("\n\r");

	printf("Initalizing...\n\r");

	printf("* Opening serial port...\n\r");
	ttyInitProf(MINIBTX_PORT,MINIBTX_BAUDRATE,8,NONE);			/* Initalize tty */
	ttyClearBuffer(MINIBTX_PORT);						/* Clear buffer */

	printf("* Serial port is set to %s at %i Baud - edit the sourcecode to alter this setting.\n\r",MINIBTX_PORT,MINIBTX_BAUDRATE);
	printf("\n\r");


	printf("Waiting for BTX-Terminal...\n\r");

	if(ttyGetchar(MINIBTX_PORT) == MINIBTX_HOOKOFF)				/* Wait for hook-off token */
		printf("* Hook off\n\r");
	else
		printf("* Warning: Incorrect hook off token (should be %02hhx)\n\r",MINIBTX_HOOKOFF);

	if(ttyGetchar(MINIBTX_PORT) == MINIBTX_CONNECT)				/* Wait for connection-ok token */
		printf("* Connection ok\n\r");
	else
		printf("* Warning: Incorrect connection start token (should be %02hhx)\n\r",MINIBTX_CONNECT);

	printf("\n\r");
	printf("\n\r");

	printf("__________________________________\n\r");
	printf("Transmitting testpage...\n\r");					/* Transmit testpage */

	while(1)
	{
	
		/* ==TRANSMISSION TEST== */

		/* Note: for a better understanding of the concept of CEPT control codes the most control
                         codes are sent seperately. This causes a leak of performance because every single
                         block that is transmitted causes an startup code which must be sent back through 
                         the slow 75-baud line. If you pack the controlcodes within the textblocks you can
                         get a much faster page transmission */

		sprintf(transmissionBuffer, F_CEPT_CS CEPTSEQ_G0);		/* Clear screen and switch to G0 set */
		systemCeptQuery(transmissionBuffer,strlen(transmissionBuffer),trsnsmissionResponseBuffer,MINIBTX_PORT,MINIBTX_LINKLAYER);
	
		sprintf(transmissionBuffer, F_CEPT_ANW CEPTSEQ_C1P F_CEPT_DBS F_CEPT_BLB F_CEPT_APR "*BILDSCHIRMTRIX#    ");
		systemCeptQuery(transmissionBuffer,strlen(transmissionBuffer),trsnsmissionResponseBuffer,MINIBTX_PORT,MINIBTX_LINKLAYER);
	
		sprintf(transmissionBuffer, CEPTSEQ_CRLF);				/* The headline is double hight and covers the next line, so we must add a blank line */
		systemCeptQuery(transmissionBuffer,strlen(transmissionBuffer),trsnsmissionResponseBuffer,MINIBTX_PORT,MINIBTX_LINKLAYER);
	
		sprintf(transmissionBuffer, CEPTSEQ_C1P F_CEPT_NSZ "miniBTX Testbild nach CEPT T/CD 6-1     ");
		systemCeptQuery(transmissionBuffer,strlen(transmissionBuffer),trsnsmissionResponseBuffer,MINIBTX_PORT,MINIBTX_LINKLAYER);
	
		sprintf(transmissionBuffer, CEPTSEQ_CRLF);				/* Add a blank line */
		systemCeptQuery(transmissionBuffer,strlen(transmissionBuffer),trsnsmissionResponseBuffer,MINIBTX_PORT,MINIBTX_LINKLAYER);
	
		sprintf(transmissionBuffer, "!\"#$%%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" );
		systemCeptQuery(transmissionBuffer,strlen(transmissionBuffer),trsnsmissionResponseBuffer,MINIBTX_PORT,MINIBTX_LINKLAYER);
	
		sprintf(transmissionBuffer, CEPTSEQ_CRLF);				/* Add a blank line */
		systemCeptQuery(transmissionBuffer,strlen(transmissionBuffer),trsnsmissionResponseBuffer,MINIBTX_PORT,MINIBTX_LINKLAYER);
	
		sprintf(transmissionBuffer, CEPTSEQ_CRLF);				/* Add a blank line */
		systemCeptQuery(transmissionBuffer,strlen(transmissionBuffer),trsnsmissionResponseBuffer,MINIBTX_PORT,MINIBTX_LINKLAYER);
	
		sprintf(transmissionBuffer, CEPTSEQ_G1);				/* Switch to G1 character set */
		systemCeptQuery(transmissionBuffer,strlen(transmissionBuffer),trsnsmissionResponseBuffer,MINIBTX_PORT,MINIBTX_LINKLAYER);
	
		sprintf(transmissionBuffer, "!\"#$%%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" );
		systemCeptQuery(transmissionBuffer,strlen(transmissionBuffer),trsnsmissionResponseBuffer,MINIBTX_PORT,MINIBTX_LINKLAYER);
	
		sprintf(transmissionBuffer, CEPTSEQ_CRLF);				/* Add a blank line */
		systemCeptQuery(transmissionBuffer,strlen(transmissionBuffer),trsnsmissionResponseBuffer,MINIBTX_PORT,MINIBTX_LINKLAYER);
	
		sprintf(transmissionBuffer, CEPTSEQ_CRLF);				/* Add a blank line */
		systemCeptQuery(transmissionBuffer,strlen(transmissionBuffer),trsnsmissionResponseBuffer,MINIBTX_PORT,MINIBTX_LINKLAYER);
	
		sprintf(transmissionBuffer, CEPTSEQ_G2);				/* Switch to G2 character set */
		systemCeptQuery(transmissionBuffer,strlen(transmissionBuffer),trsnsmissionResponseBuffer,MINIBTX_PORT,MINIBTX_LINKLAYER);
	
		sprintf(transmissionBuffer, "!\"#$%%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" );
		systemCeptQuery(transmissionBuffer,strlen(transmissionBuffer),trsnsmissionResponseBuffer,MINIBTX_PORT,MINIBTX_LINKLAYER);
	
		sprintf(transmissionBuffer, CEPTSEQ_CRLF);				/* Add a blank line */
		systemCeptQuery(transmissionBuffer,strlen(transmissionBuffer),trsnsmissionResponseBuffer,MINIBTX_PORT,MINIBTX_LINKLAYER);
	
		sprintf(transmissionBuffer, CEPTSEQ_CRLF);				/* Add a blank line */
		systemCeptQuery(transmissionBuffer,strlen(transmissionBuffer),trsnsmissionResponseBuffer,MINIBTX_PORT,MINIBTX_LINKLAYER);
	
		sprintf(transmissionBuffer, CEPTSEQ_G3);				/* Switch to G2 character set */
		systemCeptQuery(transmissionBuffer,strlen(transmissionBuffer),trsnsmissionResponseBuffer,MINIBTX_PORT,MINIBTX_LINKLAYER);
	
		sprintf(transmissionBuffer, "!\"#$%%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~" );
		systemCeptQuery(transmissionBuffer,strlen(transmissionBuffer),trsnsmissionResponseBuffer,MINIBTX_PORT,MINIBTX_LINKLAYER);
	
		sprintf(transmissionBuffer, CEPTSEQ_CRLF CEPTSEQ_CRLF CEPTSEQ_CRLF CEPTSEQ_CRLF CEPTSEQ_CRLF CEPTSEQ_CRLF);	/* Add some blank lines blank line */
		systemCeptQuery(transmissionBuffer,strlen(transmissionBuffer),trsnsmissionResponseBuffer,MINIBTX_PORT,MINIBTX_LINKLAYER);
	
		sprintf(transmissionBuffer, CEPTSEQ_G0);				/* Switch to G0 character set */
		systemCeptQuery(transmissionBuffer,strlen(transmissionBuffer),trsnsmissionResponseBuffer,MINIBTX_PORT,MINIBTX_LINKLAYER);
	
		sprintf(transmissionBuffer, F_CEPT_DBH "Testbild neu aufbauen mit # (Terminator)" CEPTSEQ_CRLF);		/* Add a blank line */
		systemCeptQuery(transmissionBuffer,strlen(transmissionBuffer),trsnsmissionResponseBuffer,MINIBTX_PORT,MINIBTX_LINKLAYER);







		/* ==RECEIVE TEST== */

		printf("__________________________________\n\r");
		printf("Retrieve keyboard input...\n\r");				/* Receive keyboard data */

		/* Note: BTX Terminals are a bit more complex than normal dump terminals - they store the keyboard
                         input in a buffer. The content of this buffer is transmitted when a DCT-Token is sent
                         to the terminal. Otherwise the terminal will only transmit the ACK tokens. */

		/* Note: Many BTX-Terminal (for example the BtxTV) are equipped with an auto-login function
                         so the first thing you might see is a long forgotton login password. */

		for(;;)
		{

			/* Note: A terminal can transmit more than one character at a time if you 
                                 give the terminal some time it is able to gather some chars */

			if(MINIBTX_FORCE_TERMINAL_KEYBOARD_DATA == 1)
			{
				/* Here we force the terminal to transmit the keyboard buffer data
				   by sending a DCT token. Warning! This is a cludge! */

				sprintf(transmissionBuffer, F_CEPT_DCT);
				systemCeptQuery(transmissionBuffer,strlen(transmissionBuffer),trsnsmissionResponseBuffer,MINIBTX_PORT,MINIBTX_LINKLAYER);
			}
			else
			{
				/* Here we got a friendly terminal that talks wihout kicking it */
				systemCeptQuery(0,0,trsnsmissionResponseBuffer,MINIBTX_PORT,MINIBTX_LINKLAYER);
			}	


			/* Now we filter out printable characters */
			systemCeptFilterKeyboardinput(trsnsmissionResponseBuffer, keyboardInput);

			/* Show keyboard data if some data received */
			if(keyboardInput[0] != '\0')
				printf("\n\r                                           Keyboard input ==>%s<==", keyboardInput);

			/* When terminator is pressed - redraw screen */
			if(keyboardInput[0] == '#')					/* Quick and dirty Terminator detection */
				break;
	
			fflush(stdout);
		}

	}
	return 0;
}

/* #################################################################################### */
