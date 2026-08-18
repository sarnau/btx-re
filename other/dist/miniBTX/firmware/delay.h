/*
   ####################################################################################
   #                                                                                  #
   #                        Bildschirmtricks Firmware V1.0.0                          #
   #                                 Delay-Routines                                   #
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
#ifndef DELAY_H
#define DELAY_H

void systemDelay1s(uint8_t value);
void systemDelay10ms(uint8_t value);
void systemDelay100ms(uint8_t value);
void systemDelay1ms(uint8_t value);
void systemDelay10us(uint8_t value);
void systemDelay100us(uint8_t value);
void systemDelay1us(uint8_t value);
void systemDelay05us(uint8_t value);
void systemDelay01us(uint8_t value);
void systemDelay16(uint16_t value);
void systemDelay8(uint8_t value);

#endif /*DELAY_H*/
/* #################################################################################### */
