/***************************************************************************
 *            cgi-bin.h
 *
 *  Tue Jun 24 17:36:32 2008
 *  Copyright  2008  sharan
 *  <sharan@bastard>
 ****************************************************************************/

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
 
#ifndef _CGI_BIN_H
	#define CGI_BIN_H
	
	int check_cgibin( void * pStruct );
	void cgi_stream( void * pStruct );
	void cgi_stats( void * pStruct );
	void cgi_reset( void * pStruct );
	void cgi_tafel( void * pStruct );

	#define MAX_CGI_ENTRYS 		4

	typedef void pCGIBIN ( void * pStruct );

	typedef struct PROGRMEM {
		pCGIBIN			*programname;
		const prog_char	*funktionname;
	} const CGIBIN ;

#endif /* CGI_BIN_H */

 
