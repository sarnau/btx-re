/*
   ####################################################################################
   #                                                                                  #
   #                        Bildschirmtricks MikroPAD V2.0.0                          #
   #                               http client layer                                  #
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
#include <avr/pgmspace.h>
#include <stdio.h>
#include <stdlib.h>

#include "system/net/ip.h"		/* Include IP protocol layer */
#include "system/net/tcp.h"		/* Include TCP protocol layer */
#include "system/net/dns.h"		/* Include DNS resolver utility */
#include "system/stdout/stdout.h"	/* Include STOUT control utility */
#include "system/clock/clock.h"		/* Include Timing utilities */
#include "btx.h"			/* Include btx service control */

#include "config.h"			/* Include btx configuration */
#include "http.h"			/* include own header file */
/* #################################################################################### */


/* ## HTTP CLIENT ##################################################################### */

/* Download CEPT-Hypertext Page from Ulm */
int applicationBtxHttpGet(char *url, unsigned long ulmIp ,char *data, int *status)
{
	volatile unsigned int httpServerSocket = NO_SOCKET_USED;
	char dataByte;
	char *dataPointer = data;
	unsigned char timeoutTimerHandle;
	int dataCount = 0;							/* Counter to prevent buffer overflows */

	/* Pass 1: Establish server connection */
	httpServerSocket = Connect2IP(ulmIp,BTX_ULM_PORT);			/* Connect http-server (Ulm) */

	if(httpServerSocket == NO_SOCKET_USED)
		return -1;							/* Connection failed */

	/* Pass 2: Perform HTTP-Request */
	STDOUT_Set_TCP_Socket(httpServerSocket);
	printf_P(PSTR("GET %s HTTP/1.0\r\nHost: " BTX_ULM_HOST "\r\n\r\n"),url);
	STDOUT_Flush();

	if(httpServerSocket == NO_SOCKET_USED)
		return -1;							/* Connection closed unexpectetly */

	timeoutTimerHandle = CLOCK_RegisterCoundowntimer();			/* Register timer */

	while(GetBytesInSocketData(httpServerSocket) == 0);			/* Trigger on incoming data */

	while(GetBytesInSocketData(httpServerSocket) > 0)
	{

		dataByte = GetByteFromSocketData(httpServerSocket);

		if(dataCount < BTX_CEPT_HYPERTEXT_BUFFERSIZE-1)
		{
			*dataPointer = dataByte;
			dataPointer++;
			dataCount++;
		}

		/* Handle connection timeout (3 retrys) */
		if(GetBytesInSocketData(httpServerSocket) == 0)
		{
			CLOCK_SetCountdownTimer (timeoutTimerHandle, 8, MSECOUND );
			while(CLOCK_GetCountdownTimer(timeoutTimerHandle) > 0);

			if(GetBytesInSocketData(httpServerSocket) == 0)
			{
				CLOCK_SetCountdownTimer (timeoutTimerHandle, 16, MSECOUND );
				while(CLOCK_GetCountdownTimer(timeoutTimerHandle) > 0);

				if(GetBytesInSocketData(httpServerSocket) == 0)
				{
					CLOCK_SetCountdownTimer (timeoutTimerHandle, 32, MSECOUND );
					while(CLOCK_GetCountdownTimer(timeoutTimerHandle) > 0);

					if(GetBytesInSocketData(httpServerSocket) == 0)
						break;
				}
			}
		}
		
	}

	CLOCK_ReleaseCountdownTimer(timeoutTimerHandle);	/* Free timer */

	*dataPointer = '\0';					/* Add a 0 to make the string functions happy */
	*status = atoi(&data[8]);				/* Calculate status code */

	CloseTCPSocket (httpServerSocket);			/* Close connection */

	STDOUT_Set_RS232();					/* Set STDOUT to console */
	return 0;
} 
/* #################################################################################### */


