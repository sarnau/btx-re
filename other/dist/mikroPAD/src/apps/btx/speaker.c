/*
   ####################################################################################
   #                                                                                  #
   #                        Bildschirmtricks MikroPAD V2.0.0                          #
   #                                 speaker control                                  #
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
#include <avr/interrupt.h>
#include <util/delay.h>

#include "system/clock/clock.h"		/* Include Timing utilities */
#include "speaker.h"			/* include own header file */
/* #################################################################################### */


/* ## SPEAKER CONTROL ################################################################# */

/* Initialize speaker control */
void applicationBtxSpeakerInit(void)
{
	PORTD |= (1 << PD0);				/* Set initial state of PD0 (Speaker) */
	DDRD |= (1 << DDD0);				/* Set PD0 (Speaker) as output */

	return;
}

/* Play a tone (tone must be between 0 and 255) */
void applicationBtxSpeakerPlayTone(int tone, int length)
{
	int i;

	cli();

	for(i=0; i<=length/tone; i++)
	{
		PORTD &= ~(1 << PD0);
		_delay_ms(tone);
		PORTD |= (1 << PD0);
		_delay_ms(tone);
	}

	sei();

	return;
}
/* #################################################################################### */
