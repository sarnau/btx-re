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
 
#ifndef _HTTPD2_H
	#define HTTPD2_H

	void httpd_init( void );
	void httpd_thread( void );

	#define	HTTP_PORT			80

	#define REQUEST_BUFFERLEN	256

	#define MAX_HTTP_PARAMS		8

	struct HTTP_REQUEST {
		char			GET_FILE[ REQUEST_BUFFERLEN ];
		char			GET_DATA[ REQUEST_BUFFERLEN ];
		char			HTTP_LINEBUFFER[ REQUEST_BUFFERLEN ];
		char			STATE;
		char			REQUEST_TYPE;
		int				REQUEST_LEN;
		char			argc;
		char *			argname[ MAX_HTTP_PARAMS ];
		char *			argvalue[ MAX_HTTP_PARAMS ];
		int				HTTP_POS;
		unsigned long	CLIENT_IP;
		unsigned int	HTTP_SOCKET;
	};

	#define GET_REQUEST			0
	#define POST_REQUEST		1

	#define DISCONNECT			0
	#define	CONNECTED			1
	#define GETREQUEST_OK		2
	#define REQUEST_END			3
	#define ANSWER_SEND			4

#endif /* HTTPD2_H */

 
