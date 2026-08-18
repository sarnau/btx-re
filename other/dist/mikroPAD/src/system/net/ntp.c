/***************************************************************************
 *            ntp.c
 *
 *  Mon Aug 28 11:36:49 2006
 *  Copyright  2006  Dirk Broßwick
 *  Email: sharandac@snafu.de
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
#include <stdlib.h>
#include <avr/interrupt.h>

#include "system/clock/clock.h"
#include "hardware/uart/uart.h"
#include "hardware/led/led_core.h"

#include "ip.h"
#include "tcp.h"
#include "ntp.h"
#include "dns.h"

unsigned int NTP_GetTime( unsigned long IP, unsigned char * dnsbuffer )
	{
		unsigned char * buffer;
		buffer = (unsigned char*) __builtin_alloca (( size_t ) 32 );

		unsigned int i=0,SOCKET;
		unsigned long Zeit,Std,Min,Sek ;
	
		union DATE ZeitInSek;
	
		if ( IP == 0 )
		{
			if ( dnsbuffer != 0 )
			{
				// Host nach IP auflösen
				IP = DNS_ResolveName( dnsbuffer );
				// könnte er aufgelöst werden ?
				if ( IP == DNS_NO_ANSWER ) return( NTP_ERROR );
			}
			else
			{
				return( NTP_ERROR );
			}
		}
		
		while( 1 )
		{
			// mit IP auf Port 37 verbinden
			SOCKET = Connect2IP( IP, 37);
			// Verbindung okay ?
			if ( SOCKET != NO_SOCKET_USED ) break;				
			if ( i > MAX_NTP_FAILED ) return( NTP_ERROR );
			i++;
		}
		
		while ( 1 )
		{
			while( 1 )
			{
				if( GetBytesInSocketData( SOCKET ) == 4 ) break;
			}
			i = GetSocketData( SOCKET, 4, buffer );
			// Wenn Daten empfangen, das weiter
			if ( i != 0 )
			{
				for ( i = 0 ; i < 4 ; i++ ) ZeitInSek.DateByte[ i ] = buffer[ 3 - i ];
				ZeitInSek.Date = ZeitInSek.Date + TimeZone;
				ZeitInSek.Date = ZeitInSek.Date % SecondsPerDay;
				Std = ZeitInSek.Date / SecondsPerHour;
				ZeitInSek.Date = ZeitInSek.Date - ( Std * 3600 );
				Min = ZeitInSek.Date / SecondsPerMin;
				Sek = ZeitInSek.Date % SecondsPerMin;

				cli();
				hh = Std;
				mm = Min;
				ss = Sek;
				sei();
				
				CloseTCPSocket( SOCKET );
				return( NTP_OK );
			}
			else
			{
				return( NTP_ERROR );
			}
				
		}
	}
