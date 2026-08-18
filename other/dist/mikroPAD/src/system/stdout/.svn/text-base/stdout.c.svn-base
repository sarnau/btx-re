/*! \file stdout.c \brief STDOUT-Funktion um die Ausgaben umzulenken */
//***************************************************************************
//*            stdout.c
//*
//*  Sat July  13 21:07:42 2008
//*  Copyright  2008  Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup system
///	\defgroup stdout Stdout Funktionen (stdout.c)
///	\code #include "stdout.h" \endcode
///	\par Uebersicht
//****************************************************************************/
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
//@{
#include <stdio.h>
#include "stdout.h"
#include "hardware/uart/uart.h"
#include "system/net/tcp.h"


static unsigned char BUFFER[ MAX_TCP_Datalenght ];
 
struct STDOUT streamout;

// stdout auf UART_Send_Byte verbiegen
static FILE mystdout = FDEV_SETUP_STREAM( STDOUT_Send_Byte , NULL, _FDEV_SETUP_WRITE);


void STDOUT_INIT( void )
{
	// printf auf uart umbiegen
	stdout = &mystdout;
	streamout.TYPE = UNKNOWN;
	streamout.BUFFER = BUFFER ;
	streamout.BUFFER_POS = 0 ;
	streamout.XPOS = 1;
	streamout.YPOS = 1;
}	

void STDOUT_Send_Byte ( unsigned char Byte )
{
	switch ( streamout.TYPE )
	{
		
		case RS232:		UART_Send_Byte ( Byte );
						break;		
		case TCP:		if ( streamout.BUFFER_POS == MAX_TCP_Datalenght )
							STDOUT_Flush();
						else
							streamout.BUFFER[ streamout.BUFFER_POS++ ] = Byte;
						break;
		default:		break;
	}
}

void STDOUT_Set_RS232 ( void )
{
	streamout.TYPE = RS232;
}

void STDOUT_Set_NULL ( void )
{
	if ( streamout.BUFFER_POS != 0 ) STDOUT_Flush();
	
	streamout.BUFFER_POS = 0;
	streamout.TYPE = NULL;
}

void STDOUT_Set_TCP_Socket ( unsigned int SOCKET )
{
	if ( SOCKET >= MAX_TCP_CONNECTIONS ) return;
	
	if ( streamout.BUFFER_POS != 0 ) STDOUT_Flush();
	
	streamout.BUFFER_POS = 0;
	streamout.TCP_SOCKET = SOCKET;
	streamout.TYPE = TCP;
}

void STDOUT_Flush()
{
	switch( streamout.TYPE )
	{
		case	RS232:		break;
		case	TCP:		if ( streamout.BUFFER_POS != 0 )
							{
								PutSocketData ( streamout.TCP_SOCKET, streamout.BUFFER_POS, streamout.BUFFER );
								streamout.BUFFER_POS = 0;
							}
							break;
		default:			break;
	}
}
//@}
