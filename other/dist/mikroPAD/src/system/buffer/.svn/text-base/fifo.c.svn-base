/*! \file fifo.c \brief Stellt die FIFO Funkionalitaet bereit */
//***************************************************************************
//*            fifo.c
//*
//*  Thu Apr  3 23:01:42 2008
//*  Copyright  2008  Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup system
///	\defgroup FIFO Stellt die FIFO Funkionalitaet bereit (fifo.c)
///	\code #include "fifo.h" \endcode
///	\par Uebersicht
///			Stellt Funktionen bereit um einen FIFO-Puffer zu verwalten.
/// \todo Die Lib muss nocg ausgibig geprüft werden, benutzung noch nicht empfohlen<br>
/// \date 04-01-2008: FIFO ist getestet und funktioniert, erste Verwendung in UART
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
#include <string.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include "fifo.h"

#define _fastcopy_
 
struct FIFO FIFO_Table[ MAX_FIFO_BUFFERS ];

unsigned char FIFO_initstate = 0;

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Initialisiert die FIFO-Buffer Strukturen.
 * \param	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void INIT_FIFO( void )
{
	unsigned int i;
	
	if ( FIFO_initstate != 0 )
		return;
	
	FIFO_initstate = 1;
	// löschen aller FIFOs
	for ( i = 0 ; i < MAX_FIFO_BUFFERS ; i++ )
		FIFO_Table[ i ].buffer = NULL;
	
	return;
}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Reserviert eine FIFO-Buffer verwaltungsstruktur.
 * \param	buffer			Zeiger auf einen Puffer der verwaltet werden soll.
 * \param	bufferlenght	Größe des Puffer.
 * \return	FIFOnumber		Die Nummer des FIFO oder FIFO_ERROR.
 */
/*------------------------------------------------------------------------------------------------------------*/
unsigned int Get_FIFO( unsigned char * buffer, unsigned int bufferlenght )
{
	unsigned int i;
	
	for( i = 0; i < MAX_FIFO_BUFFERS ; i++)
	{
		if ( FIFO_Table[ i ].buffer == NULL )
		{
			FIFO_Table[ i ].buffer = buffer;
			FIFO_Table[ i ].bufferlenght = bufferlenght;
			FIFO_Table[ i ].readpointer = 0;
			FIFO_Table[ i ].writepointer = 0;
			FIFO_Table[ i ].byteinbuffer = 0;
			FIFO_Table[ i ].Bytecopyhit = 0;
			FIFO_Table[ i ].Blockcopyhit = 0;
			FIFO_Table[ i ].lock = UNLOCK;
			return( i );
		}
	}
	return( FIFO_ERROR );
}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Gibt eine reservierte FIFO-Struktur wieder zur Benutzung frei.
 * \param	FIFO		Nummer des FIFO.
 * \return	Returncode	FIFO_OK oder FIFO_ERROR.
 */
/*------------------------------------------------------------------------------------------------------------*/
unsigned char Free_FIFO ( unsigned int FIFO )
{
	if ( ( FIFO < MAX_FIFO_BUFFERS ) && ( FIFO_Table[ FIFO ].lock == UNLOCK ) )
	{
		FIFO_Table[ FIFO ].buffer = NULL;
		return( FIFO_OK );
	}
	return( FIFO_ERROR );
}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Holt die anzahl der Bytes die sich im Puffer befunden.
 * \param	FIFO		Nummer des FIFO.
 * \return	Returncode	Anzahl der Bytes oder FIFO_ERROR.
 */
/*------------------------------------------------------------------------------------------------------------*/
unsigned int Get_Bytes_in_FIFO( unsigned int FIFO )
{
	// FIFO auf gültigkeit testen
	if ( ( FIFO < MAX_FIFO_BUFFERS ) && ( FIFO_Table[ FIFO ].lock == UNLOCK ) )
	{
		return( FIFO_Table[ FIFO ].byteinbuffer );
	}
	return( FIFO_ERROR );
}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Schreibt ein Byte in den FIFO.
 * \param	FIFO		Nummer des FIFO.
 * \param	Byte		Das Byte welches in den FIFO soll.
 * \return	Returncode	FIFO_OK oder FIFO_ERROR.
 */
/*------------------------------------------------------------------------------------------------------------*/
unsigned int Put_Block_in_FIFO( unsigned int FIFO, unsigned int bufferlenght, unsigned char * buffer )
{
	unsigned int i;
	unsigned char * bufferpointer;
	// FIFO auf gültigkeit testen
	if ( ( FIFO < MAX_FIFO_BUFFERS ) && ( FIFO_Table[ FIFO ].lock == UNLOCK ) )
	{
		// passen noch byte in den Puffer
		if ( ( FIFO_Table[ FIFO ].bufferlenght - FIFO_Table[ FIFO ].byteinbuffer ) >= bufferlenght)
		{
			#ifdef _fastcopy_
			// schnelle kopie des puffer wenn bereich in der fifo zuhängend ist
			if ( ( FIFO_Table[ FIFO ].writepointer + bufferlenght ) < FIFO_Table[ FIFO ].bufferlenght )
			{
				// schnelle kopie des puffer
				bufferpointer = FIFO_Table[ FIFO ].buffer + FIFO_Table[ FIFO ].writepointer ;
				memcpy( bufferpointer, buffer, bufferlenght );
				FIFO_Table[ FIFO ].writepointer = FIFO_Table[ FIFO ].writepointer + bufferlenght;
				FIFO_Table[ FIFO ].Blockcopyhit++;
			}
			else
			{			
			#endif
				for ( i = 0 ; i < bufferlenght ; i++ )
				{
					// Byte in Puffer schreiben
					FIFO_Table[ FIFO ].buffer[ FIFO_Table[ FIFO ].writepointer ] = buffer[i]; 
					
					FIFO_Table[ FIFO ].writepointer++;
					// writepointer schon das ende erreicht ? und setzen
					if ( FIFO_Table[ FIFO ].writepointer == FIFO_Table[ FIFO ].bufferlenght )
						FIFO_Table[ FIFO ].writepointer = 0;
				}
				FIFO_Table[ FIFO ].Bytecopyhit++;
			#ifdef _fastcopy_
			}
			#endif
			
			FIFO_Table[ FIFO ].byteinbuffer = FIFO_Table[ FIFO ].byteinbuffer + bufferlenght;

			return( bufferlenght );
		}
	}
	return( FIFO_ERROR );
}
/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Schreibt ein Byte in den FIFO.
 * \param	FIFO		Nummer des FIFO.
 * \param	Byte		Das Byte welches in den FIFO soll.
 * \return	Returncode	FIFO_OK oder FIFO_ERROR.
 */
/*------------------------------------------------------------------------------------------------------------*/
unsigned char Put_Byte_in_FIFO( unsigned int FIFO, unsigned char Byte )
{
	// FIFO auf gültigkeit testen
	if ( ( FIFO < MAX_FIFO_BUFFERS ) && ( FIFO_Table[ FIFO ].lock == UNLOCK ) )
	{
		// passen noch byte in den Puffer
		if ( FIFO_Table[ FIFO ].byteinbuffer < FIFO_Table[ FIFO ].bufferlenght )
		{
			// Byte in Puffer schreiben
			FIFO_Table[ FIFO ].buffer[ FIFO_Table[ FIFO ].writepointer ] = Byte; 
			
			FIFO_Table[ FIFO ].writepointer++;
			// writepointer schon das ende erreicht ? und setzen
			if ( FIFO_Table[ FIFO ].writepointer == FIFO_Table[ FIFO ].bufferlenght )
				FIFO_Table[ FIFO ].writepointer = 0;
			
			FIFO_Table[ FIFO ].byteinbuffer++;
			return( FIFO_OK );
		}
	}
	return( FIFO_ERROR );
}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Holt ein Byte aus den FIFO.
 * \param	FIFO		Nummer des FIFO.
 * \return	byte		Byte aus dem FIFO.
 */
/*------------------------------------------------------------------------------------------------------------*/
unsigned int Get_FIFO_to_FIFO( unsigned int Src_FIFO, unsigned int bufferlenght, unsigned int Dest_FIFO )
{
	unsigned char byte, * bufferwritepointer, * bufferreadpointer;
	unsigned int i=0;
	
	// FIFO auf gültigkeit testen
	if ( Src_FIFO < MAX_FIFO_BUFFERS && Dest_FIFO < MAX_FIFO_BUFFERS )
	{
		// sind noch bytes in den Puffer
		if ( FIFO_Table[ Src_FIFO ].byteinbuffer >= bufferlenght && ( FIFO_Table[ Dest_FIFO ].bufferlenght - FIFO_Table[ Dest_FIFO ].byteinbuffer ) >= bufferlenght)
		{
			#ifdef _fastcopy_
			// schnelle kopie des puffer wenn bereich in der fifo zuhängend ist
			if ( ( FIFO_Table[ Src_FIFO ].readpointer + bufferlenght ) < FIFO_Table[ Src_FIFO ].bufferlenght && ( FIFO_Table[ Dest_FIFO ].writepointer + bufferlenght ) < FIFO_Table[ Dest_FIFO ].bufferlenght && FIFO_Table[ Src_FIFO ].byteinbuffer > bufferlenght )
			{
				// schnelle kopie des puffer
				bufferreadpointer = FIFO_Table[ Src_FIFO ].buffer + FIFO_Table[ Src_FIFO ].readpointer ;
				bufferwritepointer = FIFO_Table[ Dest_FIFO ].buffer + FIFO_Table[ Dest_FIFO ].writepointer ;
				memcpy( bufferwritepointer, bufferreadpointer, bufferlenght );
				FIFO_Table[ Src_FIFO ].readpointer = FIFO_Table[ Src_FIFO ].readpointer + bufferlenght;
				FIFO_Table[ Dest_FIFO ].writepointer = FIFO_Table[ Dest_FIFO ].writepointer + bufferlenght;
				FIFO_Table[ Src_FIFO ].Blockcopyhit++;
				FIFO_Table[ Dest_FIFO ].Blockcopyhit++;
			}
			else
			{
			#endif
				for( i = 0; i < bufferlenght ; i++ )
				{
					byte = FIFO_Table[ Src_FIFO ].buffer[ FIFO_Table[ Src_FIFO ].readpointer ]; 
							
					FIFO_Table[ Src_FIFO ].readpointer++;
					// readpointer schon das ende erreicht ? und setzen
					if ( FIFO_Table[ Src_FIFO ].readpointer == FIFO_Table[ Src_FIFO ].bufferlenght )
						FIFO_Table[ Src_FIFO ].readpointer = 0;
									
					// Byte in Puffer schreiben
					FIFO_Table[ Dest_FIFO ].buffer[ FIFO_Table[ Dest_FIFO ].writepointer ] = byte; 
				
					FIFO_Table[ Dest_FIFO ].writepointer++;
					// writepointer schon das ende erreicht ? und setzen
					if ( FIFO_Table[ Dest_FIFO ].writepointer == FIFO_Table[ Dest_FIFO ].bufferlenght )
						FIFO_Table[ Dest_FIFO ].writepointer = 0;
				}
				FIFO_Table[ Src_FIFO ].Bytecopyhit++;
				FIFO_Table[ Dest_FIFO ].Bytecopyhit++;
			#ifdef _fastcopy_
			}
			#endif
			
			FIFO_Table[ Src_FIFO ].byteinbuffer = FIFO_Table[ Src_FIFO ].byteinbuffer - bufferlenght;
			FIFO_Table[ Dest_FIFO ].byteinbuffer = FIFO_Table[ Dest_FIFO ].byteinbuffer + bufferlenght;
		}
		else
		{
			return(0);
		}
	}
	return( bufferlenght );
}


/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Holt ein Byte aus den FIFO.
 * \param	FIFO		Nummer des FIFO.
 * \return	byte		Byte aus dem FIFO.
 */
/*------------------------------------------------------------------------------------------------------------*/
unsigned int Get_Block_from_FIFO( unsigned int FIFO, unsigned int bufferlenght, unsigned char * buffer )
{
	unsigned char byte, *bufferpointer;
	unsigned int i=0;
	
	// FIFO auf gültigkeit testen
	if ( ( FIFO < MAX_FIFO_BUFFERS ) && ( FIFO_Table[ FIFO ].lock == UNLOCK ) )
	{
		// sind noch bytes in den Puffer
		if ( FIFO_Table[ FIFO ].byteinbuffer >= bufferlenght )
		{
			#ifdef _fastcopy_
			// schnelle kopie des puffer wenn bereich in der fifo zuhängend ist
			if ( ( FIFO_Table[ FIFO ].readpointer + bufferlenght ) < FIFO_Table[ FIFO ].bufferlenght )
			{
				// schnelle kopie des puffer
				bufferpointer = FIFO_Table[ FIFO ].buffer + FIFO_Table[ FIFO ].readpointer ;
				memcpy( buffer, bufferpointer, bufferlenght );
				FIFO_Table[ FIFO ].readpointer = FIFO_Table[ FIFO ].readpointer + bufferlenght;
				FIFO_Table[ FIFO ].Blockcopyhit++;
			}
			else
			{
			#endif
				for( i = 0; i < bufferlenght ; i++ )
				{
					buffer[ i ] = FIFO_Table[ FIFO ].buffer[ FIFO_Table[ FIFO ].readpointer ]; 
							
					FIFO_Table[ FIFO ].readpointer++;
					// readpointer schon das ende erreicht ? und setzen
					if ( FIFO_Table[ FIFO ].readpointer == FIFO_Table[ FIFO ].bufferlenght )
						FIFO_Table[ FIFO ].readpointer = 0;
				}
				FIFO_Table[ FIFO ].Bytecopyhit++;
			#ifdef _fastcopy_
			}
			#endif
			
			FIFO_Table[ FIFO ].byteinbuffer = FIFO_Table[ FIFO ].byteinbuffer - bufferlenght;
		}
	}
	return( bufferlenght );
}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Holt ein Byte aus den FIFO.
 * \param	FIFO		Nummer des FIFO.
 * \return	byte		Byte aus dem FIFO.
 */
/*------------------------------------------------------------------------------------------------------------*/
unsigned char Get_Byte_from_FIFO( unsigned int FIFO )
{
	unsigned char byte;
	
	// FIFO auf gültigkeit testen
	if ( ( FIFO < MAX_FIFO_BUFFERS ) && ( FIFO_Table[ FIFO ].lock == UNLOCK ) )
	{
		// sind noch bytes in den Puffer
		if ( FIFO_Table[ FIFO ].byteinbuffer != 0 )
		{
			// Byte aus Puffer lesen
			byte = FIFO_Table[ FIFO ].buffer[ FIFO_Table[ FIFO ].readpointer ]; 
						
			FIFO_Table[ FIFO ].readpointer++;
			// readpointer schon das ende erreicht ? und setzen
			if ( FIFO_Table[ FIFO ].readpointer == FIFO_Table[ FIFO ].bufferlenght )
				FIFO_Table[ FIFO ].readpointer = 0;

			FIFO_Table[ FIFO ].byteinbuffer--;
			
			return( byte );
		}
	}
	return( 0 );
}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Leert den FIFO.
 * \param	FIFO		Nummer des FIFO.
 * \return	Returncode	FIFO_OK oder FIFO_ERROR.
 */
/*------------------------------------------------------------------------------------------------------------*/
unsigned int Flush_FIFO( unsigned int FIFO )
{
	if ( ( FIFO < MAX_FIFO_BUFFERS ) && ( FIFO_Table[ FIFO ].lock == UNLOCK ) )
	{
		FIFO_Table[ FIFO ].readpointer = 0;
		FIFO_Table[ FIFO ].writepointer = 0;
		FIFO_Table[ FIFO ].byteinbuffer = 0;
		return( FIFO_OK );
	}
	return( FIFO_ERROR );

}

unsigned int Get_FIFOsize( unsigned int FIFO )
{
	if ( FIFO < MAX_FIFO_BUFFERS ) 	
		return( FIFO_Table[ FIFO ].bufferlenght );
	return( FIFO_ERROR );
}

unsigned int Get_FIFOrestsize( unsigned int FIFO )
{
	if ( FIFO < MAX_FIFO_BUFFERS ) 	
		return( FIFO_Table[ FIFO ].bufferlenght - FIFO_Table[ FIFO ].byteinbuffer );
	return( FIFO_ERROR );
}
//@}
