/*! \file httpd2-pharse.c \brief Ein sehr einfacher HTTP-Server */
//***************************************************************************
//*            httpd2-pharse.c
//*
//*  Mon Jun 23 14:19:16 2008
//*  Copyright  2006 Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup software
///	\defgroup httpd2 Ein sehr einfacher HTTP-Server (httpd2-pharse.c)
///	\code #include "httpd2-pharse.h" \endcode
///	\par Uebersicht
/// 	Es handelt sich hier um einen einfachen HTTP-Server der nur eine statische
/// Seite ausliefert. 
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "httpd2.h"
#include "httpd2_pharse.h"

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Die Initfunktion für den HTTP-Server. Hier wird der Port registriert auf dem er lauschen soll und 
 * ein Timer, um die Bearbeitungszeit zu messen.
 * \param 	pStruct	Pointer auf die Struktur die die HTTP zurück gibt
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void PharseGetFile( void * pStruct )
{
	struct HTTP_REQUEST * http_request;
	http_request = (struct HTTP_REQUEST *) pStruct;
	
	int i , x = 0;
	char * endp;
	
	for ( i = 0 ; i < REQUEST_BUFFERLEN ; i++ )
		if ( http_request->HTTP_LINEBUFFER[ i ] == '/' || http_request->HTTP_LINEBUFFER[ i ] == '\0' )
			break;
	
	if ( i == REQUEST_BUFFERLEN ) return;
	
	i++;
	
	for ( ; i < REQUEST_BUFFERLEN ; i++)
	{
		if ( http_request->HTTP_LINEBUFFER[ i ] == ' ' ||  http_request->HTTP_LINEBUFFER[ i ] == '?' || http_request->HTTP_LINEBUFFER[ i ] == '\0' )
		{
			http_request->GET_FILE[ x++ ] = '\0';
			break;
		}
		else if ( http_request->HTTP_LINEBUFFER[ i ] == '%' )
		{
			i++;
			http_request->GET_FILE[ x++ ] = ( char ) strtol( &http_request->HTTP_LINEBUFFER[ i ], endp , 16 );
			i++;
		}
		else
		{
			http_request->GET_FILE[ x++ ] = http_request->HTTP_LINEBUFFER[ i ];											
			http_request->GET_FILE[ x ] = '\0';
		}
	}
	
	if ( http_request->GET_FILE[ 0 ] == '\0' ) strcpy( http_request->GET_FILE, "index.html" );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Die Initfunktion für den HTTP-Server. Hier wird der Port registriert auf dem er lauschen soll und 
 * ein Timer, um die Bearbeitungszeit zu messen.
 * \param 	pStruct	Pointer auf die Struktur die die HTTP zurück gibt
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void PharseGetData( void * pStruct )
{
	struct HTTP_REQUEST * http_request;
	http_request = (struct HTTP_REQUEST *) pStruct;
	
	char buffer[3] = "00";
	
	int i , x = 0, size;
	char * endp;
	
	for ( i = 0 ; i < REQUEST_BUFFERLEN ; i++ )
		if ( http_request->HTTP_LINEBUFFER[ i ] == '?' || http_request->HTTP_LINEBUFFER[ i ] == '\0' )
			break;
	
	if ( i == REQUEST_BUFFERLEN || http_request->HTTP_LINEBUFFER[ i ] == '\0' ) return;
	
	i++;
	
	for ( ; i < REQUEST_BUFFERLEN ; i++)
	{
		if ( http_request->HTTP_LINEBUFFER[ i ] == ' ' || http_request->HTTP_LINEBUFFER[ i ] == '\0' )
		{
			http_request->GET_DATA[ x++ ] = '\0';
			break;
		}
		else if ( http_request->HTTP_LINEBUFFER[ i ] == '%' )
		{
			i++;
			buffer[0] = http_request->HTTP_LINEBUFFER[ i ];
			i++;
			buffer[1] = http_request->HTTP_LINEBUFFER[ i ];
			http_request->GET_DATA[ x++ ] = ( char ) strtol( buffer, endp , 16 );
		}
		else if ( http_request->HTTP_LINEBUFFER[ i ] == '+' )
		{
			http_request->GET_DATA[ x++ ] = ' ';
		}
		else
		{
			http_request->GET_DATA[ x++ ] = http_request->HTTP_LINEBUFFER[ i ];											
			http_request->GET_DATA[ x ] = '\0';
		}
	}
	
	http_request->argc = 0;
	
	if ( http_request->GET_DATA[ 0 ] != '\0' )
	{
		http_request->argc = 1;
		http_request->argname[ http_request->argc - 1 ] = &http_request->GET_DATA[ 0 ];
		http_request->argvalue[ http_request->argc - 1 ] = &http_request->GET_DATA[ 0 ];
		
		for ( i = 0 ; i < REQUEST_BUFFERLEN ; i++ )
		{
			if ( http_request->GET_DATA[ i ] == '&' )
			{
				http_request->GET_DATA[ i ] = '\0';
				if ( http_request->argc < MAX_HTTP_PARAMS )
				{
					http_request->argc++;
					i++;
					http_request->argname[ http_request->argc - 1 ] = &http_request->GET_DATA[ i ];
					http_request->argvalue[ http_request->argc - 1 ] = &http_request->GET_DATA[ i ];
				}
			}
		}
		
		for ( i = 0 ; i < http_request->argc ; i++ )
		{
			size = strlen( http_request->argname[ i ] );
			for ( x = 0 ; x < size ; x++ )
			{											 
				if ( http_request->argname[ i ][ x ] == '\0'  )
					break;	
				else if ( http_request->argname[ i ][ x ] == '=' )
				{
					http_request->argname[ i ][ x++ ] = '\0';
					http_request->argvalue[ i ] = &http_request->argname[ i ][ x ];
				}
			}	
		}
	}
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Die Initfunktion für den HTTP-Server. Hier wird der Port registriert auf dem er lauschen soll und 
 * ein Timer, um die Bearbeitungszeit zu messen.
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
char PharseGetValue( void * pStruct , char * ArgName )
{
	struct HTTP_REQUEST * http_request;
	http_request = (struct HTTP_REQUEST *) pStruct;
	
	char ReturnCode = -1, i ;
	
	for ( i = 0 ; i < http_request->argc ; i++ )
	{
		if ( !strcmp( http_request->argname[ i ] , ArgName ) ) ReturnCode = i ;
	}
	return( ReturnCode );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Die Initfunktion für den HTTP-Server. Hier wird der Port registriert auf dem er lauschen soll und 
 * ein Timer, um die Bearbeitungszeit zu messen.
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
char PharseGetValue_P( void * pStruct , char * ArgName )
{
	struct HTTP_REQUEST * http_request;
	http_request = (struct HTTP_REQUEST *) pStruct;
	
	char ReturnCode = -1, i ;
	
	for ( i = 0 ; i < http_request->argc ; i++ )
	{
		if ( !strcmp_P( http_request->argname[ i ] , ArgName ) ) ReturnCode = i ;
	}
	return( ReturnCode );	
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Die Initfunktion für den HTTP-Server. Hier wird der Port registriert auf dem er lauschen soll und 
 * ein Timer, um die Bearbeitungszeit zu messen.
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
char PharseCheckName( void * pStruct , char * ArgName )
{
	struct HTTP_REQUEST * http_request;
	http_request = (struct HTTP_REQUEST *) pStruct;
	
	char ReturnCode = 0, i ;
	
	for ( i = 0 ; i < http_request->argc ; i++ )
	{
		if ( !strcmp( http_request->argname[ i ] , ArgName ) ) ReturnCode = -1 ;
	}
	return( ReturnCode );
}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Die Initfunktion für den HTTP-Server. Hier wird der Port registriert auf dem er lauschen soll und 
 * ein Timer, um die Bearbeitungszeit zu messen.
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
char PharseCheckName_P( void * pStruct , char * ArgName )
{
	struct HTTP_REQUEST * http_request;
	http_request = (struct HTTP_REQUEST *) pStruct;
	
	char ReturnCode = 0, i ;
	
	for ( i = 0 ; i < http_request->argc ; i++ )
	{
		if ( !strcmp_P( http_request->argname[ i ] , ArgName ) ) ReturnCode = -1 ;
	}
	return( ReturnCode );	
}

