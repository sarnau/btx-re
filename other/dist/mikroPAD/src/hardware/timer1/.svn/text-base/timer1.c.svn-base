/*!\file timer1.c \brief Stellt den Timer1 im CTC mode bereit */
//***************************************************************************
//*            timer1.c
//*
//*  Mon Jul 31 21:46:47 2006
//*  Copyright  2006 Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup hardware
///	\defgroup timer1 Stellt den Timer1 bereit (timer1.c)
///	\code #include "timer1.h" \endcode
///	\par Uebersicht
///		Stellt den Timer1 im CTC mode bereit der in festgelegten intervallen einen
///		Interrupt auslöst.
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


#include <avr/interrupt.h>
#include <avr/io.h>
#include "timer1.h"

TIMER1_CALLBACK_FUNC TIMER1_CallbackFunc[ MAX_TIMER1_CALLBACKS ];
 
/* -----------------------------------------------------------------------------------------------------------*/
/*! \brief Initialisiert den Timer1 im CTC-Mode.
 * \param 	Hz		Die anzahl der aufrufe in der Sekunde.
 */
/* -----------------------------------------------------------------------------------------------------------*/ 
void timer1_init( unsigned int Hz, unsigned int timedrift )
	{
		unsigned char i;
		
		// Alle Callbackeinträge löschen
		for ( i = 0 ; i < MAX_TIMER1_CALLBACKS ; i++ ) TIMER1_CallbackFunc[i] = NULL;
			
		// Timer0 einstellungen setzen
		TCCR1B |= ( 1<<WGM12 ); // CTC mode setzen
		OCR1A = ( 16000000 / ( 8 * Hz ) ) - timedrift ; // 100Hz bei 16MHz
		TCCR1B |= ( 0<<CS12 ) | ( 1<<CS11 ) | ( 0<<CS10 ); // Prescaler 1024

		TIMSK1 |= ( 1<<OCIE1A );  // Compare Match A Interupt freigegben

	}

/* -----------------------------------------------------------------------------------------------------------*/
/*! \brief Stoppt den Timer1.
 * \param	NONE
 */
/* -----------------------------------------------------------------------------------------------------------*/
void timer1_stop(void)
	{
		TIMSK1 &= ~( 1<<OCIE1A );  // Compare Match A Interupt sperren		
	}
			

/* -----------------------------------------------------------------------------------------------------------*/
/*! \brief Gibt den Timer1 frei.
 * \param	NONE
 */
/* -----------------------------------------------------------------------------------------------------------*/ 
void timer1_free(void)
	{
		TIMSK1 |= ( 1<<OCIE1A );  // Compare Match A Interupt freigeben
	}


ISR( TIMER1_COMPA_vect )
	{
		unsigned char i;
		for ( i = 0 ; i < MAX_TIMER1_CALLBACKS ; i++ ) if ( TIMER1_CallbackFunc[i] != NULL ) TIMER1_CallbackFunc[i]();
	}
	
/* -----------------------------------------------------------------------------------------------------------*/
/*! \brief Hinterlegt eine Callbackfunktion für den Timer1.
 * \param	pFunc		Zeiger auf die Aufzurufende Funktion.
 * \return	ErrorCode	TRUE oder FALSE.
 */
/* -----------------------------------------------------------------------------------------------------------*/ 
unsigned char timer1_RegisterCallbackFunction( TIMER1_CALLBACK_FUNC pFunc )
	{
		unsigned char i;
		
		for ( i = 0 ; i < MAX_TIMER1_CALLBACKS ; i++ ) 
		{
			if ( TIMER1_CallbackFunc[i] == pFunc )
				return TRUE;
		}
		
		for ( i = 0 ; i < MAX_TIMER1_CALLBACKS ; i++ )
		{
			if ( TIMER1_CallbackFunc[i] == NULL )
			{
				TIMER1_CallbackFunc[i] = pFunc;
				return TRUE;
			}
		}
		return FALSE;
	}

/* -----------------------------------------------------------------------------------------------------------*/
/*! \brief Hinterlegt eine Callbackfunktion für den Timer1.
 * \param	pFunc		Zeiger auf die Aufzurufende Funktion.
 * \return	ErrorCode	TRUE oder FALSE.
 */
/* -----------------------------------------------------------------------------------------------------------*/ 
unsigned char timer1_RemoveCallbackFunction( TIMER1_CALLBACK_FUNC pFunc )
	{
		unsigned char i;
		
		for ( i = 0 ; i < MAX_TIMER1_CALLBACKS ; i++ )
		{
			if ( TIMER1_CallbackFunc[i] == pFunc )
			{
				TIMER1_CallbackFunc[i] = NULL;
				return TRUE;
			}
		}
		return FALSE;
	}
//@}
