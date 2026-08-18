/*
   ####################################################################################
   #                                                                                  #
   #                        Bildschirmtricks Firmware V1.0.0                          #
   #                                  Uart-Handler                                    #
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
#include <avr/io.h>					/* Include I/O definitions */
#include <avr/interrupt.h>				/* Include Interrupt control */
#include <stdint.h>					/* Include Integer types */
#include <stdbool.h>					/* Include Boolean types */
#include <util/delay.h>					/* Include delay functions */

#include "uart.h"					/* Include own header file */
/* #################################################################################### */


/* ## INITALIZE UART INTERFACE ######################################################## */
/* Initalize UART */
void systemUartInit(uint16_t UartBaudrate)
{
	UCSRC |= (1<<USBS);				/* Configure UART to operate in asynchronous mode */

	UCSRC &= ~(0<<UCSZ2);				/* Configure UART ... */
	UCSRC |= (1<<UCSZ1);				/* ...to operate */
	UCSRC |= (1<<UCSZ0);				/* ...with 8 data bits */

	UCSRC &= ~(0<<UPM1);				/* don't know what... */
	UCSRC &= ~(0<<UPM0);				/* ...this funny bits do */

	UBRRH = (uint8_t)(UartBaudrate >> 8); 		/* Configure UART0... */
	UBRRL = (uint8_t)(UartBaudrate & 0x00FF);	/* ... to selected baudrate */

	UCSRB |= (1<<TXEN);				/* Enable transmitter of UART */
	UCSRB |= (1<<RXEN);				/* Enable reviver of UART */

	/* DISABLED: UCSRB |= (1<<RXCIE); */		/* Enable reciver interrupt */
	/* DISABLED: UCSRA |= (1<<U2X); */		/* Enable doubble speed */
}
/* #################################################################################### */


/* ## TRANSMIT TROUGH UART INTERFACES ################################################# */
void systemUartTransmit(uint8_t byte2transmit)
{
		while ( !( UCSRA & (1<<UDRE)) )		/* Wait till the interface becomes ready */
		{
		}
		UDR = byte2transmit;			/* Transmit byte */
}
/* #################################################################################### */


/* ## RECIVE TROUGH UART INTERFACES ################################################### */
uint8_t systemUartRecive(void)
{
		while (!(UCSRA & (1<<RXC)));		/* Wait until a byte is recived, always true on ISR call */
		return UDR;				/* Return the recived byte */
}
/* #################################################################################### */


/* ## RECIVER INTERRUPT SERVICE ROUTINES ############################################## */
/* ISR for UART0 */
ISR(USART0_RX_vect) 
{
	/* This interrupt service routine is not used, so it is left empty */
}
/* #################################################################################### */
