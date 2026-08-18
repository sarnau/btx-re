/***************************************************************************
 *            udp-echo.h
 *
 *  Sun Sep 10 13:48:24 2006
 *  Copyright  2006  User
 *  Email
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#ifndef _UDP-ECHO_H
	#define _UDP-ECHO_H

	void UDP_echo( void );
	void UDP_echo_init( void );
	
	#define UDP_Bufferlen 	1500
	#define UDPPORT_ECHO	7
	
#endif /* _UDP-ECHO_H */
