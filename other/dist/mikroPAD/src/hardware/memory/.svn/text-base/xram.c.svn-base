/*! \file xram.c \brief Aktiviert das externe RAM-Interface */
//***************************************************************************
//*            xram.c
//*
//*  Sat Jun  3 23:01:42 2006
//*  Copyright  2006  User
//*  Email
//****************************************************************************/
///	\ingroup hardware
///	\defgroup xram Aktiviert das externe RAM-Interface (xram.c)
///	\code #include "xram.h" \endcode
///	\par Uebersicht
/// Aktiviert das externe RAM-Interface. Wenn die xram.h eingebunden wird, wird
/// automatisch die Aktivierung in .init eingetragen und steht somit sofort zur
/// Verfügung.
//****************************************************************************/
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
//@{

#include <avr/io.h>
#include "xram.h"

#include "hardware/led/led_1.h"
#include "hardware/led/led_2.h"

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Aktiviert das externe RAM-Interface und testet den externen RAM
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void init_xram (void)
{
	// externes RAM-Interface freigeben
//	XMCRA |= ( 1<<SRE ) | (1<<SRW11) | (1<<SRW10) ;
	XMCRA |= ( 1<<SRE ) ;
	// A16 freigeben, damit der RAM funktioniert, wenn dies nicht gemacht wird, ist A16 tristate und der RAM macht komische sachen :-)
	DDRD |= ( 1<<PD7 );
	PORTD |= ( 1<<PD7 );
	
//	DDRA |= 0xff;
//	PORTA |= 0xff;
	
	LED_2_init();
	LED_1_init();
	
	// RAM-Test durchführen
	unsigned char *p = ( unsigned char * ) 0x2200;
	
//	p = 0xffff;
//	*p = 0xff;
	
//	while(1);
	
	for( p = 0x0200 ; p <= 0x21ff ; p++ )
		*p = 0x55;

	for( p = 0x0200 ; p <= 0x21ff ; p++ )
	{
		if( !(*p == 0x55) )
		{ 
			LED_1_ON();
			while(1);
		}
	}

	for( p = 0x2200 ; p <= 0xfffe ; p++ )
		*p = 0x55;

	for( p = 0x2200 ; p <= 0xfffe ; p++ )
	{
		if( !(*p == 0x55) )
		{ 
			LED_2_ON();
			while(1);
		}
	}
}
//@}
