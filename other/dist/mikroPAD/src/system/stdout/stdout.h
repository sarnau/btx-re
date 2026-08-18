/*! \file stdout.h \brief STDOUT-Funktion um die Ausgaben umzulenken */
//***************************************************************************
//*            stdout.h
//*
//*  Sat July  13 21:07:42 2008
//*  Copyright  2008  Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup system
///	\defgroup stdout Stdout Funktionen (stdout.h)
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
#ifndef _STDOUT_H
	#define STDOUT_H

void STDOUT_INIT( void );
void STDOUT_Send_Byte ( unsigned char Byte );
void STDOUT_Set_RS232 ( void );
void STDOUT_Set_TCP_Socket ( unsigned int SOCKET );
void STDOUT_Set_LEDTAFEL ( void );
void STDOUT_Set_NULL ( void );
void STDOUT_Flush();

	struct STDOUT {
		char			TYPE;
		char			TCP_SOCKET;
		unsigned char *	BUFFER;
		int				BUFFER_POS;
		int				XPOS;
		int				YPOS;
	};

	#define UNKNOWN		-1
	#define NULL		0
	#define	RS232		1
	#define TCP			2
	#define TAFEL		3

#endif /* STDOUT_H */

 //@}
