/***************************************************************************
 *            udp-echo.c
 *
 *  Sun Sep 10 13:47:20 2006
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
#include <avr/pgmspace.h>
#include <stdio.h>
#include <string.h>

#include "hardware/uart/uart.h"

#include "system/net/udp.h"
#include "udp-echo.h"

unsigned int UDP_Socket;

void UDP_echo_init( void )
{
	UDP_Socket = 0xffff;
	printf_P( PSTR("UDP-Echo Service gestartet auf Port %d.\r\n"),UDPPORT_ECHO);
	return;
}

void UDP_echo( void )
{
	// make an static UDPbuffer
	// warning, do not use an stack-allocated buffer! it will be damage the udp-packet
	static unsigned char UDPBuffer[ UDP_Bufferlen ];

	// if an Socket created or opened, if not, create them ?
	if ( UDP_Socket == 0xffff )
		UDP_Socket = UDP_ListenOnPort( UDPPORT_ECHO, UDP_Bufferlen, UDPBuffer );
	else
	{
		if ( UDP_GetSocketState( UDP_Socket ) == SOCKET_BUSY )
		{
			LockEthernet();
			UDP_SendPacket( UDP_Socket, UDP_GetByteInBuffer( UDP_Socket ), UDPBuffer );
			FreeEthernet();
			UDP_CloseSocket( UDP_Socket );
			UDP_Socket = 0xffff;
		}
	}
	return;
}
