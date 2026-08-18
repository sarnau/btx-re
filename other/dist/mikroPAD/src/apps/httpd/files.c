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
 
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/io.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include "httpd2.h" 
#include "cgi-bin.h"
#include "system/net/tcp.h"
#include "system/stdout/stdout.h"

#include "files.h"
#include "files_data.h"

int check_files( void * pStruct )
{
	struct HTTP_REQUEST * http_request;
	http_request = (struct HTTP_REQUEST *) pStruct;
		
	int i, returnvalue = -1;
	
	for( i = 0 ; i < MAX_FILES_ENTRYS ; i++ )
	{
		if ( !strcmp_P( &http_request->GET_FILE, files[ i ].filesname ) )
		{
			if ( files[ i ].filestype == PNG )
			{
				printf_P( PSTR("HTTP/1.0 200\r\n"
							   "Content-Type: png\r\n"
							   "Content-Lenght: %d\r\n"
							   "Keep-Alive: close\r\n\r\n" ), files[ i ].len );
			}
			else if ( files[ i ].filestype == JPEG )
			{
				printf_P( PSTR("HTTP/1.0 200\r\n"
							   "Content-Type: jpeg\r\n"
							   "Content-Lenght: %d\r\n"
							   "Keep-Alive: close\r\n\r\n" ), files[ i ].len );
			}
			else if ( files[ i ].filestype == TEXT )
			{
				printf_P( PSTR("HTTP/1.0 200\r\n"
							   "Content-Type: text/html\r\n"
							   "Keep-Alive: close\r\n\r\n" ) );
			}
			STDOUT_Flush();
			PutSocketData_P( http_request->HTTP_SOCKET, files[ i ].len, files[ i ].files );
			returnvalue = 1 ;
			break;
		}
	}
	return( returnvalue );
} 
