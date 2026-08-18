/*
   ####################################################################################
   #                                                                                  #
   #                          Bildschirmtricks pcPAD V1.0.0                           #
   #                                terminal control                                  #
   #                                                                                  #
   #    Copyright (C) 2008-2014 Philipp Fabian Benedikt Maier (aka. Dexter)           #
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
#include <stdio.h>
#include <stdlib.h>
#include <libcodebananas/ttybanana.h>
#include <libcodebananas/toolbanana.h>
#include "termctrl.h"			/* include own header file */
#include "config.h"
/* #################################################################################### */


/* ## TERMINAL CONTROL ################################################################ */
/* Initialize terminal control */
void applicationBtxTermctrlInit(char *port)
{
	printf(" * Initalizing terminal port:\n");
	ttyInitProf(port,TERMINAL_BAUDRATE,TERMINAL_DATABITS,NONE);
	ttyClearBuffer(port);

	printf("   Serial port is set to %s at %i Baud\n",port,TERMINAL_BAUDRATE);

	return;
}

/* Avoid unwanted transmissions from the terminal */
void applicationBtxTermctrlInhibit(int status)
{
	/* Note: This has been ported from the mikroPAD firmware. The
	         microPAD has control lines to inhibit unwanted input
		 while the page is being transmitted. miniBTX does not
		 have such control lines in its current version. Thats
		 why you need to keep your fingers off the terminal
		 while it is receiving the page! */

	return;
}

/* Read status of the ready line (1=Terminal not ready, 0=Terminal ready */
int applicationBtxTermctrlGetReadyState(void)
{
	/* Note: This has been ported from the mikroPAD firmware. The
	         microPAD has control lines to determine if the terminal
		 is ready or not. miniBTX does not have such control 
		 lines in its current version. We just assume that the
		 terminal is connected and ready */

	return TERMCTRL_TERMINAL_READY;
}

/* Force connection termination */
int applicationBtxTermctrlTerminateConnection(void)
{
	/* Note: This has been ported from the mikroPAD firmware. The
	         microPAD has control lines to control the termianl
		 connection status. miniBTX does not have such control 
		 lines in its current version. So we just assume that
                 the terminal is disconnected */

	return 0;
}
/* #################################################################################### */

 
