/*! \file vs10xx.h \brief Stellt Funktionen für den VS10xx Decoder bereit */
//***************************************************************************
//*            vs10xx.h
//*
//*  Mon May 12 17:42:13 2008
//*  Copyright  2008 Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup hardware
///	\defgroup VS10xx Funktionen für den VS10xx (vs10xx.h)
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
#ifndef _VS1001K_H
	#define VS1001K_H

	typedef enum {
		SOFT_RESET,
		HARD_RESET
	} reset_e;

	#define VS10xx_XTAL	24000000UL
	#define VS10xx_CLOCKF	(VS10xx_XTAL/2000)

	char VS10xx_INIT( void );
	unsigned int VS10xx_read( unsigned char address);
	void VS10xx_write(unsigned char address, unsigned int Data);
	void VS10xx_send_32_from_FIFO( unsigned int FIFO );
	void VS10xx_send_32( unsigned char *pBuffer );
	void VS10xx_send_data(unsigned char Data );
	void VS10xx_nulls(unsigned int nNulls);
	char VS10xx_reset(reset_e r);
	void VS10xx_sine_on(unsigned char freq);
	void VS10xx_sine_off(void);
	unsigned char VS10xx_sine_sweep(void);
	void VS10xx_vol( unsigned char Lvol, unsigned char Rvol );
	void VS10xx_set_xtal( void );
	void VS10xx_flush_from_FIFO( unsigned int FIFO );
	unsigned int VS10xx_get_decodetime( void );

	#define SS2_PORT		PORTB
	#define SS2_DDR			DDRB
	#define SS2				PB0

	#define RESET_PORT		PORTB
	#define RESET_DDR		DDRB
	#define RESET			PB4

	#define	BSYNC_DDR		DDRF
	#define BSYNC_PORT		PORTF
	#define BSYNC			PF0

	#define DREQ_DDR		DDRE
	#define DREQ_PORT		PORTE
	#define DREQ_PIN		PINE
	#define DREQ			PE5

	#define POWER_DDR		DDRF
	#define POWER_PORT		PORTF
	#define POWER			PF1

	#define RESET_OK		0
	#define RESET_FAILED	-1

	//
	// VS1001 commands
	//
	#define VS10xx_READ		0x03
	#define VS10xx_WRITE		0x02

    //
    // VS1001 Controlregister
	//
	#define	VS10xx_Register_MODE			0
	#define	VS10xx_Register_STATUS			1
	#define	VS10xx_Register_CLOCKF			3
	#define	VS10xx_Register_DECODE_TIME		4
	#define	VS10xx_Register_AUDATA			5
	#define	VS10xx_Register_WRAM			6
	#define	VS10xx_Register_WRAM_ADDR		7
	#define	VS10xx_Register_HDAT0			8
	#define	VS10xx_Register_HDAT1			9
	#define	VS10xx_Register_AIADDR			10
	#define	VS10xx_Register_VOL			11
	#define	VS10xx_Register_AICTRL			12

#endif /* VS1001K_H */
//@}
