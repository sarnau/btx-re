/*!\file spi_0.h \brief Stellt die SPI0-Schnittstelle bereit*/
//***************************************************************************
//*            spi_0.h
//*
//*  Mon Jul 31 21:46:47 2006
//*  Copyright  2006  Dirk Broßwick
//*  Email: sharandac@snafu
///	\ingroup hardware
///	\defgroup SPI0 Die SPI0-Schnittstelle (spi_0.h)
///	\code #include "spi_0.h" \endcode
///	\par Uebersicht
///		Die SPI0-Schnittstelle fuer den AVR-Controller
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
 
#ifndef _SPI_0_H
	#define _SPI_0_H
	
	#include <avr/io.h>

	unsigned int SPI0_init( unsigned int Options );
	unsigned char SPI0_ReadWrite( unsigned char Data );
	unsigned char SPI0_GetInitState( void );
	void SPI0_FastRead2Mem( unsigned char * buffer, unsigned int Datalenght );
	void SPI0_FastMem2Write( unsigned char * buffer, unsigned int Datalenght );
	
	#define SPI_NOT_INIT	0x00
	#define SPI_HALF_SPEED	0x01
	#define SPI_FULL_SPEED	0x02
	
	#define SPI0_PORT		PORTB
	#define SPI0_DDR		DDRB
	#define MISO			PB3
	#define MOSI			PB2
	#define SCK				PB1

#endif /* _SPI_0_H */
//@}
