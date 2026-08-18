/*! \file cgi-bin.c \brief CGI-BIN Programme und Funktionen */
//***************************************************************************
//*            cgi-bin.c
//*
//*  Sat May  10 21:07:42 2008
//*  Copyright  2008  Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup software
///	\defgroup cgibin CGI-BIN Programme und Funktionen (cgi-bin.c)
///	\code #include "cgi-bin.h" \endcode
///	\par Uebersicht
///
///
///
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
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/io.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "system/clock/clock.h"
#include "system/net/ethernet.h"
#include "system/stdout/stdout.h"
 
#include "apps/mp3-streamingclient/mp3-clientserver.h"

#include "httpd2.h" 
#include "cgi-bin.h"
#include "system/net/tcp.h"
#include "system/softreset/softreset.h"

const char cmd1[] PROGMEM = "stream.cgi";
const char cmd2[] PROGMEM = "stats.cgi";
const char cmd3[] PROGMEM = "reset.cgi";

CGIBIN cgibin[ MAX_CGI_ENTRYS ] = {
	{ cgi_stream, cmd1 },
	{ cgi_stats, cmd2 },
	{ cgi_reset, cmd3 }
};

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Initialisiert die streamingengine.
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
int check_cgibin( void * pStruct )
{
	struct HTTP_REQUEST * http_request;
	http_request = (struct HTTP_REQUEST *) pStruct;
	
	int i, returnvalue = -1;
	
	for( i = 0 ; i < MAX_CGI_ENTRYS ; i++ )
	{
		if ( !strcmp_P( &http_request->GET_FILE, cgibin[ i ].funktionname ) )
		{
			cgibin[ i ].programname( http_request );
			returnvalue = 1 ;
			break;
		}
	}

	return( returnvalue );
}	
	
/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Initialisiert die streamingengine.
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void cgi_stream( void * pStruct )
{
	struct HTTP_REQUEST * http_request;
	http_request = (struct HTTP_REQUEST *) pStruct;
	
	printf_P( PSTR(		"HTTP/1.0 200\r\n"
						"Content-Type: text/html\r\n"
						"Keep-Alive: close\r\n"
						"\r\n"
						"<HTML>\r\n"
						"<HEAD>\r\n"));
	if( http_request->argc != 0 && PharseCheckName_P ( http_request, PSTR("info") ) )
	{
		printf_P( PSTR(	"<meta http-equiv=\"refresh\" content=\"60; \">\r\n"));
	}
	
	printf_P( PSTR(	"<TITLE>Streaming</TITLE>\r\n"
					"</HEAD>\r\n"
					"<BODY>\r\n"));

	if ( http_request->argc == 0  )
	{
		printf_P( PSTR( "<form action=\"stream.cgi\" method=\"get\" accept-charset=\"ISO-8859-1\">"
					    "<p>Stream-URL:<input name=\"streamurl\" size=\"50\"></p>"
					    "<p><input type=\"submit\" value=\"stream starten\"></p></form>"
					   	"<form action=\"stream.cgi?stop\" method=\"get\" accept-charset=\"ISO-8859-1\">"
					    "<p><input type=\"submit\" name=\"stop\" value=\"stop\"></p></form>"
					   	"<form action=\"stream.cgi?replay\" method=\"get\" accept-charset=\"ISO-8859-1\">"
					    "<p><input type=\"submit\" name=\"replay\" value=\"replay\"></p></form>" ));
	}
	else if( PharseCheckName_P ( http_request, PSTR("streamurl") ) )
	{	
		printf_P( PSTR("<pre><p>"));
		PlayURL( http_request->argvalue[ PharseGetValue_P ( http_request, PSTR("streamurl") ) ], http_request->HTTP_SOCKET );
		printf_P( PSTR("</pre></p>"));
	}
	else if( PharseCheckName_P ( http_request, PSTR("config") ) )
	{	
		printf_P( PSTR("kommt noch!"));
	}
	else if( PharseCheckName_P ( http_request, PSTR("replay") ) )
	{	
		printf_P( PSTR("<pre><p>"));
		RePlayURL( http_request->HTTP_SOCKET );
		printf_P( PSTR("</pre></p>"));
	}
	else if( PharseCheckName_P ( http_request, PSTR("stop") ) )
	{	
		printf_P( PSTR("<pre><p>"));
		StopPlay();
		printf_P( PSTR("</pre></p>"));
	}
	else if( PharseCheckName_P ( http_request, PSTR("info") ) )
	{
		printf_P( PSTR("<pre><p>"));
		mp3clientPrintInfo ( http_request->HTTP_SOCKET , HTML );
		printf_P( PSTR("</pre></p>"));
	}
	printf_P( PSTR(		"</BODY>\r\n"
						"</HTML>\r\n"
						"\r\n"));
	
	STDOUT_Flush();
}
	

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Initialisiert die streamingengine.
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void cgi_stats( void * pStruct )
{	
	
	struct HTTP_REQUEST * http_request;
	http_request = (struct HTTP_REQUEST *) pStruct;
	
	static int VisitCounter=0;
	
	struct TIME Time;
	// Zeit holen
	CLOCK_GetTime ( &Time );
	
	VisitCounter++;
			
	printf_P( PSTR(	"HTTP/1.0 200\r\n"
					"Content-Type: text/html\r\n"
					"Keep-Alive: close\r\n"
					"\r\n"
					"<HTML>\r\n"
					"<HEAD>\r\n"
					"<meta http-equiv=\"refresh\" content=\"120; \">\r\n"
					"<TITLE>Winkewinke</TITLE>\r\n"
					"</HEAD>\r\n"
					"<BODY bgcolor=\"#6666FF\" text=\"#FFFFFF\">"));
			
	printf_P( PSTR( "Zeit: %02d:%02d:%02d.%02d"), Time.hh, Time.mm, Time.ss, Time.ms );
		
	printf_P( PSTR( ", Uptime: %ld sek ; "), Time.uptime );

	printf_P( PSTR( "Ethernet: %ld Bytes "), ByteCounter);

	printf_P( PSTR( "in %ld Packets ; "), PacketCounter );

	printf_P( PSTR( "Du bist der %d. Besucher auf Socket %d.\r\n"), VisitCounter, http_request->HTTP_SOCKET );

	printf_P( PSTR(	"</BODY>\r\n"
					"</HTML>\r\n"
					"\r\n"));

}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Initialisiert die streamingengine.
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void cgi_reset( void * pStruct )
{
	struct HTTP_REQUEST * http_request;
	http_request = (struct HTTP_REQUEST *) pStruct;
	
	printf_P( PSTR(		"HTTP/1.0 200\r\n"
						"Content-Type: text/html\r\n"
						"Keep-Alive: close\r\n"
						"\r\n"
						"<HTML>\r\n"
						"<HEAD>\r\n"
						"<TITLE>Reset</TITLE>\r\n"
						"</HEAD>\r\n"
						"<BODY>\r\n"));

	if ( http_request->argc == 0 )
	{
		printf_P( PSTR(	"<form action=\"reset.cgi\" method=\"get\" accept-charset=\"ISO-8859-1\">"
						"<p>Stream-URL:<input name=\"text\" size=\"56\"></p>"
						"<p><input type=\"submit\" name=\"reset\" value=\"reset\"></p></form>"
						"</BODY>\r\n"
						"</HTML>\r\n"
						"\r\n"));
	}
	else if( PharseCheckName_P( http_request, PSTR("reset") ) )
	{	
		if ( !strcmp_P( http_request->argvalue[ PharseGetValue_P( http_request, PSTR("reset") ) ] , PSTR("reset") ) )
		{
			printf_P( PSTR(	"Reset</BODY>\r\n"
							"</HTML>\r\n"
							"\r\n"));
			STDOUT_Flush();
			CloseTCPSocket( http_request->HTTP_SOCKET );
			softreset();
			while(1);
		}
	}
	STDOUT_Flush();	
}
//@}
