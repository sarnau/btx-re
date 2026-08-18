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
#ifndef CTRL_H
#define CTRL_H

void systemCtrlInit(void);			/* Initalize Control lines */
void systemCtrlReady(void);			/* Toggle ready signal (ready => low) */
uint8_t systemCtrlCheckInhibit(void);		/* Check if inhibit signal is present (Low=Inhibit=0 , High=Normal=1) */
uint8_t systemCtrlCheckTerminate(void);		/* Check if terminate signal is present (Low=Terminate=0 , High=Normal=1 */ 

#endif /*CTRL_H*/
/* #################################################################################### */
 
 
