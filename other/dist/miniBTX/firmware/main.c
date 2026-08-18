/*
   ####################################################################################
   #                                                                                  #
   #                        Bildschirmtricks Firmware V1.0.0                          #
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
#include <avr/io.h>					/* Include I/O definitions */
#include <stdint.h>					/* Include Integer types */
#include <stdio.h>					/* Include standard I/O */
#include <avr/pgmspace.h>				/* Include pgmspace definition */
#include <avr/interrupt.h>				/* Include Interrupt control */

#include "uart.h"					/* Include serial port control */
#include "dbt03.h"					/* Include dbt03 emulator layer */
#include "ctrl.h"					/* Include control line handler */
#include "delay.h"					/* Include delay function set */

#define MINIBTX_HOOKOFF 0x01
#define MINIBTX_CONNECT 0x00
/* #################################################################################### */


/* ## MAIN ############################################################################ */
int main(void)
{
	systemUartInit(832);			/* Initalize UART with 1200 Baud */
	systemCtrlInit();			/* Initalize control line handler (inhibit, ready) */
	systemDbt03Init();			/* Initalize dbt03 emulation (communication line to the terminal */

	systemUartTransmit(MINIBTX_HOOKOFF);	/* Tell the V24 side that terminal hooked off the line */

	systemDbt03ConnectionOk();		/* Make the terminal think that the connection is sucessfully set up */

	systemUartTransmit(MINIBTX_CONNECT);	/* Tell the V24 side that connection to terminal is made */
	systemCtrlReady();			/* Tell the V24 side that connection to terminal is made (the low level method) */ 

	while(1)				/* Enter main-loop (Pass all data from uart to dbt03) */
	{
		systemDbt03Transmit(systemUartRecive());
		
		if(systemCtrlCheckTerminate() == 0)	/* Terminate connection if terminate control line is pulled low */
			systemDbt03ConnectionTerminate();
	}

	return 0;
}
/* #################################################################################### */
