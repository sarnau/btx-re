/*!\file spi_2.h \brief Stellt die SPI2-Schnittstelle bereit*/
//***************************************************************************
//*            spi_2.h
//*
//*  Mon Jul 31 21:46:47 2006
//*  Copyright  2006  Dirk Broßwick
//*  Email: sharandac@snafu
///	\ingroup hardware
///	\defgroup SPI2 Die SPI-Schnittstelle (spi_2.h)
///	\code #include "spi_2.h" \endcode
///	\par Uebersicht
///		Die SPI2-Schnittstelle fuer den AVR-Controller
//****************************************************************************/
//@{
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
 
#ifndef _SPI_2_H
	#define SPI_2_H

	#include <avr/io.h>

	void SPI2_init( void );
	unsigned char SPI2_ReadWrite( unsigned char Data );
	void SPI2_FastMem2Write( unsigned char * buffer, unsigned int Datalenght );
	void SPI2_FastRead2Mem( unsigned char * buffer, unsigned int Datalenght );

	#define SPI2_PORT		PORTB
	#define SPI2_DDR		DDRB

	#define MISO2			PB2
	#define MOSI2			PB3
	#define SCK2			PB1

#endif /* SPI_2_H */
//@}
