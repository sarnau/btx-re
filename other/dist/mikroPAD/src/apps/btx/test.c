/*
   ####################################################################################
   #                                                                                  #
   #                        Bildschirmtricks MikroPAD V2.0.0                          #
   #                            integrated test routines                              #
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

#include "hardware/uart/uart.h"		/* Include Uart hardware drivers */
#include "system/clock/clock.h"		/* Include Timing utilities */

#include "config.h"			/* Include btx configuration */
#include "speaker.h"			/* include own header file */
#include "btx.h"			/* Include btx service control */
#include "termctrl.h"			/* Include termcontrol */
#include "cept.h"			/* Include cept parser */
/* #################################################################################### */


/* ## TEST ROUTINES ################################################################### */

/* Run integrated test routines */
void applicationBtxTestRun(void)
{
	unsigned long ulmIp;					/* ip-adress of ulm (dummy) */
	int testErrors = 0;					/* Counts the test errors */
	int i;
	unsigned char testInput;

	DDRD &= ~(1 << DDD1);					/* Set PD1 (Test-sw) as input */
	PORTD |= (1 << PD1);					/* Enable pullup of PD1 (Test-sw) */
	CLOCK_delay(1000);

	if(((PIND >> PD1) & 1) == 0)				/* Test invoked, proceed with test routines */
	{
#if (DEBUGPORT == 1)
		printf_P(PSTR("TEST invoked\r\n"));
		while(UART_Get_Bytes_in_Tx_Buffer() != 0);
#endif /*DEBUGPORT*/

		applicationBtxSpeakerPlayTone(6,800);
		CLOCK_delay(1000);

#if (DEBUGPORT == 1)
		printf_P(PSTR("* Boot ok\r\n"));	
		while(UART_Get_Bytes_in_Tx_Buffer() != 0);
#endif /*DEBUGPORT*/
				
		/* Check if internet connection works correctly */
		if(applicationBtxResolveUlm(&ulmIp) != -1)
		{
			applicationBtxSpeakerPlayTone(6,800);
			CLOCK_delay(1000);
#if (DEBUGPORT == 1)
			printf_P(PSTR("* Internet ok.\r\n"));	
			while(UART_Get_Bytes_in_Tx_Buffer() != 0);
#endif /*DEBUGPORT*/
		}
		else
		{
			applicationBtxSpeakerPlayTone(16,800);
			CLOCK_delay(1000);
#if (DEBUGPORT == 1)
			printf_P(PSTR("* Error: Internet failed!\r\n"));	
			while(UART_Get_Bytes_in_Tx_Buffer() != 0);
#endif /*DEBUGPORT*/
			testErrors++;
		}

		/* Check if terminal is ready */
		if(applicationBtxTermctrlGetReadyState() == TERMCTRL_TERMINAL_READY)
		{
			applicationBtxSpeakerPlayTone(6,800);
			CLOCK_delay(1000);
#if (DEBUGPORT == 1)
			printf_P(PSTR("* Terminal ready.\r\n"));	
			while(UART_Get_Bytes_in_Tx_Buffer() != 0);
#endif /*DEBUGPORT*/
		}
		else
		{
			applicationBtxSpeakerPlayTone(12,800);
			CLOCK_delay(1000);
#if (DEBUGPORT == 1)
			printf_P(PSTR("* Error: Terminal ready signal not present!\r\n"));	
			while(UART_Get_Bytes_in_Tx_Buffer() != 0);
#endif /*DEBUGPORT*/
			testErrors++;
		}

		/* Perform an interactive terminal check */
#if (DEBUGPORT == 1)
		applicationBtxTermctrlPortSelect(TERMCTRL_BTX_TERMINAL);
#endif /*DEBUGPORT*/
		printf_P(PSTR(F_CEPT_CS "TERMINAL TEST:" F_CEPT_APD F_CEPT_APR));
		printf_P(PSTR("Type something (15 chars) to verify" F_CEPT_APD F_CEPT_APR));
		printf_P(PSTR("terminal function ==>" F_CEPT_CON));
		while(UART_Get_Bytes_in_Tx_Buffer() != 0);

		CLOCK_delay(CEPT_ECHOPLEX_GUARDTIME);

		for(i=0; i<15; i++)
		{
			do
			{
			testInput = UART_Get_Byte();
			CLOCK_delay(CEPT_ECHOPLEX_GUARDTIME);
			} while(!((testInput >= 0x20)&&(testInput <= 0x7E)));

			UART_Send_Byte(testInput);
			while(UART_Get_Bytes_in_Tx_Buffer() != 0);
		}
		printf_P(PSTR(F_CEPT_COF F_CEPT_APD F_CEPT_APR "THANK YOU!"));

#if (DEBUGPORT == 1)
		applicationBtxTermctrlPortSelect(TERMCTRL_V24_TERMINAL);
#endif /*DEBUGPORT*/

		applicationBtxSpeakerPlayTone(6,800);
		CLOCK_delay(1000);

#if (DEBUGPORT == 1)
		printf_P(PSTR("* Interactive terminal test done.\r\n"));	
		while(UART_Get_Bytes_in_Tx_Buffer() != 0);

		printf_P(PSTR("\r\n"));	

		if(testErrors == 0)
			printf_P(PSTR("Test successfully passed -- now powercycle the device\r\n"));
		else
			printf_P(PSTR("Test passed with %i error(s) -- now powercycle the device\r\n"),testErrors);
#endif /*DEBUGPORT*/

		while(1);
	}
	else
		return;					/* Test not invoked, abort! */
}
/* #################################################################################### */
