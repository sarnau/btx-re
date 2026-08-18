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
#ifndef TERMCTRL_H
#define TERMCTRL_H

#define TERMCTRL_BTX_TERMINAL 1			/* Select BTX-Terminal for communication (videotex-console) */
#define TERMCTRL_V24_TERMINAL 2			/* Select V24-Terminal for communication (debug-console) */
#define TERMCTRL_MULTIPLEXER_GUARDTIME 200	/* Multiplexer guard time, depends on the Mikrowebserver release */

#define TERMCTRL_INHIBIT_ON 1			/* Inhibit on: Ignore all data comming from the terminal */
#define TERMCTRL_INHIBIT_OFF 2			/* Inhibit off: Normal operation */

#define TERMCTRL_TERMINAL_READY 1		/* Terminal is ready for receiving data */
#define TERMCTRL_TERMINAL_ABSENT 2		/* Terminal is absent and not able to receive data */

/* Initialize terminal control */
void applicationBtxTermctrlInit(char *port);

/* Avoid unwanted transmissions from the terminal */
void applicationBtxTermctrlInhibit(int status);

/* Read status of the ready line (1=Terminal not ready, 0=Terminal ready */
int applicationBtxTermctrlGetReadyState(void);

/* Force connection termination */
int applicationBtxTermctrlTerminateConnection(void);

#endif /*TERMCTRL_H*/
/* #################################################################################### */
