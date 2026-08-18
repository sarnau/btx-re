/*
   ####################################################################################
   #                                                                                  #
   #                        Bildschirmtricks MikroPAD V2.0.0                          #
   #                                terminal control                                  #
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

#include "system/clock/clock.h"		/* Include Timing utilities */
#include "hardware/uart/uart.h"		/* Include Uart hardware drivers */
#include "termctrl.h"			/* include own header file */
/* #################################################################################### */


/* ## TERMINAL CONTROL ################################################################ */
/* Initialize terminal control */
void applicationBtxTermctrlInit(void)
{
	PORTB |= (1 << PB0);				/* Set initial state of PB0 (Terminal Select) */
	DDRB |= (1 << DDB0);				/* Set PB0 (Terminal Select) as output */

	PORTB |= (1 << PB7);				/* Set initial state of PB7 (Inhibit) */
	DDRB |= (1 << DDB7);				/* Set PB7 (Inhibit) as output */

	PORTB |= (1 << PB5);				/* Set initial state of PB7 (Terminate) */
	DDRB |= (1 << DDB5);				/* Set PB5 (Terminate) as output */

	DDRB &= ~(1 << DDB6);				/* Set PB1 (Ready) as input */
	PORTB |= (1 << PB6);				/* Enable pullup of PB1 (Ready) */

	return;
}


/* Select serial communication port (we have only one hardware Uart, so we need to multiplex) */
void applicationBtxTermctrlPortSelect(int port)
{
	while(UART_Get_Bytes_in_Tx_Buffer() != 0);	/* Wait until all data is transmit */

CLOCK_delay(TERMCTRL_MULTIPLEXER_GUARDTIME);
	if(port == TERMCTRL_V24_TERMINAL)
		PORTB |= (1 << PB0);
	else if(port == TERMCTRL_BTX_TERMINAL)
		PORTB &= ~(1 << PB0);

CLOCK_delay(TERMCTRL_MULTIPLEXER_GUARDTIME);
	return;
}

/* Avoid unwanted transmissions from the terminal */
void applicationBtxTermctrlInhibit(int status)
{
	if(status == TERMCTRL_INHIBIT_OFF)
		PORTB |= (1 << PB7);
	else if(status == TERMCTRL_INHIBIT_ON)
		PORTB &= ~(1 << PB7);
	return;
}

/* Read status of the ready line (1=Terminal not ready, 0=Terminal ready */
int applicationBtxTermctrlGetReadyState(void)
{
	if(((PINB >> PB6) & 1) == 0)
		return TERMCTRL_TERMINAL_READY;
	else
		return TERMCTRL_TERMINAL_ABSENT;
}

/* Force connection termination */
int applicationBtxTermctrlTerminateConnection(void)
{
	applicationBtxTermctrlPortSelect(TERMCTRL_BTX_TERMINAL);
	PORTB &= ~(1 << PB5);
	UART_Send_Byte(0x00);		/* Send a 0x00 to be sure that termination signal is proper detected */
	applicationBtxTermctrlPortSelect(TERMCTRL_V24_TERMINAL);

	return 0;
}


/* #################################################################################### */

 
