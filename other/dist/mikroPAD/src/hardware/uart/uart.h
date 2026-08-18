/*!\file uart.h \brief Stellt die UART-Schnittstelle bereit */
//***************************************************************************
//*            uart.h
//*
//*  Mon Jul 31 21:46:47 2007
//*  Copyright  2007 Dirk Broßwick
//*  Email
///	\ingroup hardware
///	\defgroup UART Die UART-Schnittstelle (uart.h)
///	\par Uebersicht
///		Die UART-Schnittstelle fuer den AVR-Controller
//****************************************************************************/
//@{
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

#ifndef _UART_H
	#define _UART_H

	#define F_CPU 16000000L

	#define BAUD 1200L
//	#define BAUD 57600L
//	#define BAUD 9600L
	#define UBRR_VAL (F_CPU/(BAUD*16)-1)
	
	#define TX_Bufferlen  	256
	#define RX_Bufferlen  	256
	
	#define TX_complete		0
	#define TX_sending		1

	void UART_init( void );
	void UART_Send_Byte( unsigned char Byte );
	unsigned char UART_Get_Byte( void );
	unsigned int UART_Get_Bytes_in_Buffer( void );
	unsigned int UART_Get_Bytes_in_Tx_Buffer( void );

#endif /* _UART_H */
//@}
