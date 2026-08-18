/***************************************************************************
 *            led_0.c
 *
 *  Tue Mar 11 21:11:55 2008
 *  Copyright  2008  sharan
 *  <sharan@bastard>
 ****************************************************************************/

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */
 
#include <avr/io.h>
#include "led_core.h"
#include "led_1.h"

void LED_1_init( void )
{
	LED_DDR |= ( 1<<LED_PIN );
	LED_Register ( LED_1_ON, LED_1_OFF, LED_1_TOGGLE );
	LED_1_OFF ();
}

void LED_1_ON ( void )
{
	LED_PORT &= ~( 1<<LED_PIN );
}

void LED_1_OFF ( void )
{
	LED_PORT |= ( 1<<LED_PIN );
}

void LED_1_TOGGLE ( void )
{
	if ( ( LED_PORT & ( 1<<LED_PIN )) != 0 ) 
	{
		LED_PORT &= ~( 1<<LED_PIN );
	}
	else 
	{
		LED_PORT |= ( 1<<LED_PIN );
	}
}
