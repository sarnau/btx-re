/*! \file clock.h \brief Stellt die CLOCK Funkionalitaet bereit */
//***************************************************************************
//*            clock.h
//*
//*  Sat Jun  3 23:01:42 2006
//*  Copyright  2006  Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup system
///	\defgroup CLOCK Die Clockfunktion für den Microcontroller als Zeitbasis (clock.h)
///	\par Uebersicht
///		Stellt funktionen bereit um eine genaue Zeit zu realisieren und funktionen
/// um Zeitgesteuert eigene Funktionen die man hinterlegt aufzurufen
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
 
#ifndef _CLOCK_H
	#define _CLOCK_H
	
	volatile unsigned long uptime;
	volatile unsigned char ms;
	volatile unsigned char ss;
	volatile unsigned char mm;
	volatile unsigned char hh;
	
	typedef void ( * CLOCK_CALLBACK_FUNC ) ( void );
	
	#define MAX_CLOCK_COUNTDOWNTIMER	32
	#define MAX_CLOCK_CALLBACKS		16

//	#define TRUE				0
//	#define FALSE				(!TRUE)
//	#define NULL				0
	
	#define MSECOUND			1
	#define SECOUND				2
	#define MINUTE				3
	#define HOUR				4
	#define DAY					5
	#define NO_USE				6

	#define	NonExecutionbit		0
	#define	Executionbit		1

	/*! \struct TIME
	 *  \brief Definiert die Struktur in der die Zeit an eine Anwendung übergeben wird.
	 */
	struct	TIME {
		volatile unsigned char ms;			/*!< Zeit in 1/100 Sekunden */
		volatile unsigned char ss;			/*!<  Sekunden */
		volatile unsigned char mm;			/*!<  Minuten */
		volatile unsigned char hh;			/*!<  Stunden */
		volatile unsigned long uptime;		/*!<  Laufzeit seid dem letzten Reset */
	};

	/*! \struct CALLBACK
	 *  \brief Struktur für Callbackeinträge.
	 */
	struct CALLBACK {
		volatile CLOCK_CALLBACK_FUNC 	CallbackFunc;	/*!< Pointer auf die Funktion die aufgerufen wird. NULL wenn nicht benutzt */
		volatile unsigned char 			Resolution;		/*!< Die mögliche zeitliche auflösung */
		volatile unsigned char			Execution;		/*!< Der wert wird gesetzt wenn die Funktion nicht im aktuellen Tick passt, wird dann beim nächsten Tick ausgeführt */
	};
	
	/*! \struct COUNTER
	 *  \brief Hier werden die Counter verwaltet
	 */
	struct COUNTER {
		volatile unsigned long			Counter;	/*!< Der Counter der gezählt wird */
		volatile unsigned char			Resolution; /*!< Die Auflösung mit den der Counter verringert wird, siehe define MSECOUND, SECOUND, ... */
	};

	

	void CLOCK_init( void );
	void CLOCK_tick( void );
	unsigned char CLOCK_RegisterCallbackFunction( CLOCK_CALLBACK_FUNC pFunc, unsigned char Resolution);
	unsigned char CLOCK_RemoveCallbackFunction( CLOCK_CALLBACK_FUNC pFunc );
	unsigned char CLOCK_RegisterCoundowntimer( void );
	void CLOCK_SetCountdownTimer( unsigned char counter, unsigned int value, unsigned char Resolution );
	unsigned int CLOCK_GetCountdownTimer( unsigned char counter );
	void CLOCK_ReleaseCountdownTimer( unsigned char counter );
	void CLOCK_GetTime( unsigned char * Time );
	void CLOCK_delay(unsigned int us);

#endif /* _CLOCK_H */

