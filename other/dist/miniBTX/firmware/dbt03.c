/*
   ####################################################################################
   #                                                                                  #
   #                        Bildschirmtricks Firmware V2.0.0                          #
   #                              dbt03 emulator layer                                #
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
#include <stdbool.h>					/* Include Boolean types */
#include <avr/interrupt.h>				/* Include Interrupt control */
#include <util/parity.h>				/* Include parity calculation functions */

#include "dbt03.h"					/* Include own header file */
#include "uart.h"					/* Include serial port control */
#include "delay.h"					/* Include Delay control mechanisms */
#include "ctrl.h"					/* Include control line handler */
/* #################################################################################### */


/* #################################################################################### */

/* Internal subroutine: Toggle display I/O pin */
static void dbt03IoCtrl(const uint8_t status)
{
	if(status == 1)
		PORTB |= (1 << PB0);
	else
		PORTB &= ~(1 << PB0);
	
	return;
}

/* Internal subroutine: Perform 425Hz tone (as a TTL signal) */
static void dbt03Dialtone(const uint16_t cycles)
{
	uint16_t i;

	for(i=0; i<cycles; i++)
	{
		dbt03IoCtrl(0);
		systemDelay100us(10);
		dbt03IoCtrl(1);
		systemDelay100us(10);
	}
}

/* Internal subroutine: Perform 1300Hz tone (as a TTL signal) */
static void dbt03Carriertone(const uint16_t cycles)
{
	uint16_t i;

	for(i=0; i<cycles; i++)
	{
		dbt03IoCtrl(0);
		systemDelay10us(38);
		dbt03IoCtrl(1);
		systemDelay10us(38);
	}
}

/* Initalize dbt03 emulation layer */
void systemDbt03Init(void)
{
	dbt03IoCtrl(1);					/* Set initial state of serial output line */

	DDRB |= (1 << DDB0);				/* Set PB0 as output */
	PORTD &= ~(1 << PD2);				/* Disable pullup resistor on PB2 */

	GIMSK |= (1<<INT0);				/* Configure interrupt driven receiver */
	MCUCR |= (1<<ISC01);
	sei();

	return;
}

/* Send character to the BTX-Terminal */
void systemDbt03Transmit(uint8_t character)
{
	uint8_t i;

	cli();

	dbt03IoCtrl(1);					/* Perform start bit */
	systemDelay10us(DBT03_TX_BAUDRATE);

	for(i=0; i<=7; i++)				/* Transmit data bits */
	{
		dbt03IoCtrl((~character >> i) & 1);
		systemDelay10us(DBT03_TX_BAUDRATE);
	}

	dbt03IoCtrl(0);
	systemDelay10us(DBT03_TX_BAUDRATE);

	sei();

	return;
}

/* Receive a character from the BTX-Terminal */
uint8_t systemDbt03Receive(void)
{
	uint8_t i;
	uint8_t result = 0;

	if(systemCtrlCheckInhibit() == 0)
	{
		cli();
	
		while(((PIND >> PD2) & 1) == 1);		/* Wait for incoming start bit */
		systemDelay100us(DBT03_RX_BAUDRATE);
	
		for(i=0; i<=7; i++)				/* Read data bits */
		{
			systemDelay100us(DBT03_RX_BAUDRATE / 2);
	
			result |= (((PIND >> PD2) & 1) << i);
	
			systemDelay100us(DBT03_RX_BAUDRATE / 2);
		}
	
		sei();
	}

	return result;
}

/* Tell the BTX-Terminal that connection went ok */
void systemDbt03ConnectionOk(void)
{
	systemDelay100ms(DBT03_HOOK_OFF_DELAY);		/* Modem startup time (about 3s) */
	dbt03Dialtone(DBT03_DAILTONE_CYCLES);		/* Perform dialtone */
	systemDelay100ms(DBT03_DIALIN_DELAY);		/* Delay dialintime (Terminal beleves now that the modem dials) */
	dbt03Dialtone(DBT03_RINGTONE_CYCLES);		/* Perform ringtone */
	systemDelay100ms(DBT03_POST_RINGTONE_DELAY);	/* Delay aftr ringtone */
	dbt03Carriertone(DBT03_CARRIERTONE_CYCLES);	/* Perform carrier tone */
	dbt03IoCtrl(0);					/* Pull I/O line to low level */
	systemDelay100ms(DBT03_POST_DIALIN_DELAY);	/* Delay time after dailin */

	return;
}

/* Tell the BTX-Terminal that the connection has terminated */
void systemDbt03ConnectionTerminate(void)
{
	dbt03IoCtrl(1);					/* Pull I/O line to high level */
	while(1);					/* Halt in endless loop (Terminal will cause reset of power off, so do not worry about deadlock) */
	return;
}

/* Receive a character from the BTX-Terminal via interrupt and pass it through the UART */
SIGNAL(INT0_vect)
{
	uint8_t i;
	uint8_t result = 0;

	if(systemCtrlCheckInhibit() == 1)
	{
		systemDelay100us(DBT03_RX_BAUDRATE/2);		/* let start bit pass by and check whether it is a real start bit */
		if(((PIND >> PD2) & 1) != 0)
			return;					
		systemDelay100us(DBT03_RX_BAUDRATE / 2);
	
		for(i=0; i<=7; i++)				/* Read data bits */
		{
			systemDelay100us(DBT03_RX_BAUDRATE / 2);
			result |= (((PIND >> PD2) & 1) << i);
			systemDelay100us(DBT03_RX_BAUDRATE / 2);
		}
	
		systemDelay100us(DBT03_RX_BAUDRATE/2);		/* let start bit pass by */
		systemUartTransmit(result);
	}
}

/* #################################################################################### */


