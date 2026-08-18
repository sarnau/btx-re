/*
   ####################################################################################
   #                                                                                  #
   #                        Bildschirmtricks Firmware V1.0.0                          #
   #                               Control-line-Handler                               #
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
#include <stdint.h>					/* Include Integer types */
/* #################################################################################### */


/* #################################################################################### */
/* Initalize Control lines */
void systemCtrlInit(void)
{
	PORTB |= (1 << PB1);				/* Set PB1 to high level */
	DDRB |= (1 << DDB1);				/* Set PB1 as output */
	PORTB |= (1 << PB2);				/* Enable PB2 (inhibit) pullup resistor */
	PORTD |= (1 << PD4);				/* Enable PD4 (terminate) pullup resistor */
	return;
}

/* Toggle ready signal (ready => low) */
void systemCtrlReady(void)
{
	PORTB &= ~(1 << PB1);				/* Set PB1 to low level */
}

/* Check if inhibit signal is present (Low=Inhibit=0 , High=Normal=1) */
uint8_t systemCtrlCheckInhibit(void)
{
	return ((PINB >> PB2) & 1);
}

/* Check if terminate signal is present (Low=Terminate=0 , High=Normal=1 */ 
uint8_t systemCtrlCheckTerminate(void)
{
	return ((PIND >> PD4) & 1);
}

/* #################################################################################### */