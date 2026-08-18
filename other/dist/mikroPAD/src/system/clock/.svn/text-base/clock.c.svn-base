/*! \file clock.c \brief Stellt die CLOCK Funkionalitaet bereit */
//***************************************************************************
//*            clock.c
//*
//*  Sat Jun  3 23:01:42 2006
//*  Copyright  2006  Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup system
///	\defgroup CLOCK Die Clockfunktion für den Microcontroller als Zeitbasis (clock.c)
///	\code #include "clock.h" \endcode
///	\par Uebersicht
///		Stellt funktionen bereit um eine genaue Zeit zu realisieren und funktionen
/// um Zeitgesteuert eigene Funktionen die man hinterlegt aufzurufen
/// \todo	Mach mal schneller die Routine. 
///			Wenn zu viele Callback registriert sind bleibt der Controller hängen, sollte man
///         mal ändern. Vorschlag wäre die Callback zu verteilen auf die einzelnen Ticks.
/// \date	03-06-2008: Neuen Code hinzugefügt, geht jetzt schneller, die Callbacks und 
///			Counter werden nur einmal durchsucht pro Tick.
/// \date	04-06-2008: Lastverteilung in den Callbacks eingebaut, geht jetzt wunderbar, es wird pro Tick
///			nur eine Callback aufgerufen für Resolutions größer gleich Sekunde, die anderen werden
///			als Executionbit markiert und in den folgenden Tick ausgeführt.
/// \date	05-01-2008: Die Uhrzeit kann jetzt mit Hilfe eines Struct geholt werden, sollte in zukunft benutzt
///			werden da nicht mehr auf die Globalen Variablen zugegriffen werden muss.
/// \date	05-14-2008: Delayfunktion hinzugefügt.
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

#include <avr/interrupt.h>
#include <avr/io.h>
#include "hardware/timer1/timer1.h"
#include "clock.h"
	
struct CALLBACK CallBack_Table [ MAX_CLOCK_CALLBACKS ]; 

struct COUNTER Counter_Table [ MAX_CLOCK_COUNTDOWNTIMER ];


/*-----------------------------------------------------------------------------------------------------------*/
/*! \brief Initialisiert die System-Clock und registriert ihn auf Timer1 mit 1/100s als Callbackfunktion.
 *  \param	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void CLOCK_init(void)
	{
		unsigned int i;
		
		// Alle Callbackeinträge löschen
		for ( i = 0 ; i < MAX_CLOCK_CALLBACKS ; i++ ) 
			{
				CallBack_Table[i].CallbackFunc = NULL;
				CallBack_Table[i].Resolution = NO_USE;
			}
			
		for ( i = 0 ; i < MAX_CLOCK_COUNTDOWNTIMER ; i++ ) 
			{
				Counter_Table[i].Counter = NULL;
				Counter_Table[i].Resolution = NO_USE;
			}
			
		ms=0;
		ss=0;
		mm=0;
		hh=0;
		uptime=0;
		// Clocksource init
		timer1_init( 100 , 0);
		// clock_tick registrieren als Callbackfunktion
		timer1_RegisterCallbackFunction( CLOCK_tick );
	}
	
/*-----------------------------------------------------------------------------------------------------------*/
/*! \brief Die ISR der Clock, hier werden die Uhrzeit und Callbackeinträge abgearbeitet
 * \param	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void CLOCK_tick( void )
	{

/*		unsigned char i;
		ms++;
		for ( i = 0 ; i < MAX_CLOCK_COUNTDOWNTIMER ; i++ )
			{
				if ( Counter_Table[i].Resolution == MSECOUND && Counter_Table[i].Counter != NULL )
				{
					Counter_Table[i].Counter--;
				}
			}
			// Callback für millisekunden suchen
		for ( i = 0 ; i < MAX_CLOCK_CALLBACKS ; i++ )
			{
				if ( CallBack_Table[i].Resolution == MSECOUND ) CallBack_Table[i].CallbackFunc();
			}

		if ( ms == 100 )
		{
			ms = 0;
			ss++;
			uptime++;
			// Callback für sekunden suchen
			for ( i = 0 ; i < MAX_CLOCK_CALLBACKS ; i++ )
				{
					if ( CallBack_Table[i].Resolution == SECOUND ) CallBack_Table[i].CallbackFunc();
				}

			if ( ss == 60 )
			{
				ss = 0;
				mm++;
		
				// Callback für minuten suchen
				for ( i = 0 ; i < MAX_CLOCK_CALLBACKS ; i++ )
					{
						if ( CallBack_Table[i].Resolution == MINUTE ) CallBack_Table[i].CallbackFunc();
					}

				if ( mm == 60 )
				{
					mm = 0;
					hh++;

					// Callback für stunden suchen
					for ( i = 0 ; i < MAX_CLOCK_CALLBACKS ; i++ )
						{
							if ( CallBack_Table[i].Resolution == HOUR ) CallBack_Table[i].CallbackFunc();
						}

					if ( hh == 24 )
					{
						hh = 0;
						
						// Callback für tag suchen
						for ( i = 0 ; i < MAX_CLOCK_CALLBACKS ; i++ )
							{
								if ( CallBack_Table[i].Resolution == DAY ) CallBack_Table[i].CallbackFunc();
							}
					}
				}
			}
		}
		
*/
		
		unsigned char state;
		unsigned int i;
		ms++;
		state = MSECOUND;
		if ( ms == 100 )
		{
			ms = 0;
			ss++;
			uptime++;
			state = SECOUND;
			if ( ss == 60 )
			{
				ss = 0;
				mm++;
				state = MINUTE;
				if ( mm == 60 )
				{
					mm = 0;
					hh++;
					state = HOUR;
					if ( hh == 24 )
					{
						hh = 0;
						state = DAY;
					}
				}
			}
		}
				
		// alle Callbacks durchgehen und alles was größer Resolution MSECOUND als Execution markieren für spätere ausführung
		for ( i = 0; i < MAX_CLOCK_CALLBACKS ; i++ )
		{
			if ( CallBack_Table[i].Resolution <= state )
			{
				if ( state >= SECOUND )
					CallBack_Table[i].Execution = Executionbit;
				else
				{
					CallBack_Table[i].Execution = NonExecutionbit;
					CallBack_Table[i].CallbackFunc();
				}
			}
		}

		// Hier werden die als Execution markierten Callbacks ausgeführt, je durchlauf einer
		for ( i = 0; i < MAX_CLOCK_CALLBACKS ; i++ )
		{
			if ( CallBack_Table[i].Execution == Executionbit )
			{
				CallBack_Table[i].Execution = NonExecutionbit;
				CallBack_Table[i].CallbackFunc();
				break;
			}
		}
		
		for ( i = 0 ; i < MAX_CLOCK_COUNTDOWNTIMER ; i++ )
		{
			if ( Counter_Table[i].Resolution <= state && Counter_Table[i].Counter != NULL )
			{
				Counter_Table[i].Counter--;
			}
		}
	}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Registriert einen CountdownTimer.
 * \param	pFunc		Zeiger auf die Funktion die aufgerufen werden soll
 * \param	Resolution	Gibt die Auflösung an mit der die Callbackfunktion aufgerufen werden soll
 *						mögliche Paramter: MSECOUND,SECOUND,MINUTE,HOUR und DAY
 * \return	TRUE oder FALSE
 */
/*------------------------------------------------------------------------------------------------------------*/
unsigned char CLOCK_RegisterCallbackFunction( CLOCK_CALLBACK_FUNC pFunc, unsigned char Resolution )
	{
		unsigned char i;
		
		timer1_stop();
		for ( i = 0 ; i < MAX_CLOCK_CALLBACKS ; i++ ) 
		{
			if ( CallBack_Table[i].CallbackFunc == pFunc )
			{
				timer1_free();
				return TRUE;
			}
		}
		
		for ( i = 0 ; i < MAX_CLOCK_CALLBACKS ; i++ )
		{
			if ( CallBack_Table[i].CallbackFunc == NULL )
			{
				// zuerst Pointer zu Funktion setzen und Resolution setzen zum scharf machen
				// dies ist nötig da der zugrif auf Resolution atomar ist
				CallBack_Table[i].CallbackFunc = pFunc;
				CallBack_Table[i].Resolution = Resolution;
				timer1_free();
				return TRUE;
			}
		}
		timer1_free();
		return FALSE;
	}
	
/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Löscht einen registrierte Callbackfunktion.
 * \param	pFunc		Zeiger auf die Funktion die aufgerufen werden soll
 * \return	TRUE oder FALSE
 */
/*------------------------------------------------------------------------------------------------------------*/
unsigned char CLOCK_RemoveCallbackFunction( CLOCK_CALLBACK_FUNC pFunc )
	{
		unsigned char counter;
		
		timer1_stop();
		for ( counter = 0 ; counter < MAX_CLOCK_CALLBACKS ; counter++ )
		{
			if ( CallBack_Table[ counter ].CallbackFunc == pFunc )
			{
				// zuerst Resolution löschen zum sperren und dann Pointer löschen
				// dies ist nötig da der zugrif auf Resolution atomar ist
				CallBack_Table[ counter ].Resolution = NO_USE;
				CallBack_Table[ counter ].CallbackFunc = NULL;
				timer1_free();
				return TRUE;
			}
		}
		timer1_free();
		return FALSE;
	}
	
/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Registriert einen Counterdown-zähler.
 * \code
 * unsigned char timer;
 *
 * #define TIMEOUT 1000
 *		
 * timer = CLOCK_RegisterCoundowntimer();
 * CLOCK_SetCountdownTimer( timer , TIMEOUT, MSEC );
 *
 * while ( 1 )
 * {
 *		if ( Get_Key() && ( CLOCK_GetCountdownTimer( timer ) != 0 ) )
 *		{
 *			CLOCK_ReleaseCountdownTimer( timer );
 *			return( Okay );
 *		}
 *		if ( CLOCK_GetCountdownTimer( timer ) == 0 ) 
 *		{
 *			CLOCK_ReleaseCountdownTimer( timer );
 *			return( Error );
 *		}
 * }
 * \endcode
 * \return	Die Nummer des Countdowntimer oder FALSE
 */
/*------------------------------------------------------------------------------------------------------------*/
unsigned char CLOCK_RegisterCoundowntimer( void )
	{
		unsigned char counter ;
		
		timer1_stop();
		
		for ( counter = 0 ; counter < MAX_CLOCK_COUNTDOWNTIMER ; counter++ )
		{
			if ( Counter_Table[ counter ].Resolution == NO_USE )
			{
				Counter_Table[ counter ].Counter = 0;
				timer1_free();
				return ( counter ) ;
			}
		}
		timer1_free();
		return( FALSE );		
	}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Set einen startwert für einen Counterdown-zähler.
 * \param	counter		Die Counternummer der benutzt werden soll, dieser sollte vorher mit CLOCK_RegisterCoundowntimer
 *						ermittelt worden sein.
 * \param	value		Der Wert ab den gegen 0 gezählt werden soll.
 * \param	Resolution	Gibt die Auflösung an mit der die Callbackfunktion aufgerufen werden soll
 *						mögliche Paramter: MSECOUND,SECOUND,MINUTE,HOUR und DAY
 * \return	Die Nummer des Countdowntimer oder FALSE
 */
/*------------------------------------------------------------------------------------------------------------*/
void CLOCK_SetCountdownTimer( unsigned char counter, unsigned int value, unsigned char Resolution )
	{
		timer1_stop();
		Counter_Table[ counter ].Counter = value;
		Counter_Table[ counter ].Resolution = Resolution;
		timer1_free();
	}
	
/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Holt den aktuellen Zählerstand für einen Counterdown-zähler.
 * \param	counter		Die Counternummer der benutzt werden soll, dieser sollte vorher mit CLOCK_RegisterCoundowntimer
 *						ermittelt worden sein.
 * \return	Value		Der Zählerstand
 */
/*------------------------------------------------------------------------------------------------------------*/
unsigned int CLOCK_GetCountdownTimer( unsigned char counter )
	{
		unsigned int value;

		timer1_stop();
		value = Counter_Table[ counter ].Counter;
		timer1_free();
		
		return ( value );
	}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Gibt einen Counter wieder zur Benutzung frei.
 * \param	counter		Die Counternummer die benutzt werden soll, dieser sollte vorher mit CLOCK_RegisterCoundowntimer
 *						ermittelt worden sein.
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/	
void CLOCK_ReleaseCountdownTimer( unsigned char counter )
	{
		Counter_Table[ counter ].Resolution = NO_USE ;
	}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Holt die Aktuelle Uhrzeit ist speichert sich in der übergebenen Struktur.
 * \param	Time		Pointer auf die Struct in der die Uhrzeit abgelegt werden soll.
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void CLOCK_GetTime( unsigned char * Time )
	{
		struct TIME * Timestruct; 		//ETH_struc anlegen
		Timestruct = (struct TIME *) Time;
		
		timer1_stop();
		
		Timestruct->ms = ms;
		Timestruct->ss = ss;
		Timestruct->mm = mm;
		Timestruct->hh = hh;
		Timestruct->uptime = uptime;
		
		timer1_free();
		
		return( 0 );
	}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Delay was sonst.
 * \param	us			Warte einfach eine Zeit in us.
 * \return	Value		Der Zählerstand
 */
/*------------------------------------------------------------------------------------------------------------*/
void CLOCK_delay(unsigned int us) 
{
	unsigned char counter;
		
	counter = CLOCK_RegisterCoundowntimer();
	CLOCK_SetCountdownTimer( counter , us/10 , MSECOUND );

	while ( 1 )
		{
			if ( CLOCK_GetCountdownTimer( counter ) == 0 ) 
			{
				CLOCK_ReleaseCountdownTimer( counter );
				return;
			}
		}	
} 
//@}
