/*! \file config.c \brief Stellt Config-tools bereit */
//***************************************************************************
//*            config.c
//*
//*  Son Aug 10 16:25:49 2008
//*  Copyright  2008  Dirk Broßwick
//*  Email: sharandac@snafu.de
///	\ingroup hardware
///	\defgroup config Die Config (config.c)
///	\code #include "config.h" \endcode
///	\par Uebersicht
//****************************************************************************/
//@{
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
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <stdio.h>
#include "config.h"

void eetest( void )
{
	char test[] = ("test foobar");
	char test2[ sizeof( test ) ];
	
	eeprom_write_block( test , 0, sizeof ( test ) );
	eeprom_read_block( test2 , 0, sizeof ( test ) );
	
	printf_P( PSTR("String: %s\r\n"), test2 );
}

char readConfig_P( char * ConfigName, char * ConfigValue )
{
	
}

char writeConfig_P( char * ConfigName, char * ConfigValue )
{
	
}

char changeConfig_P( char * ConfigName, char * ConfigValue )
{
	
}

char deleteConfig_P( char * ConfigName)
{
	
}

char readConfig( char * ConfigName, char * ConfigValue )
{
	
}

char writeConfig( char * ConfigName, char * ConfigValue )
{
	
}

char changeConfig( char * ConfigName, char * ConfigValue )
{
	
}

char deleteConfig( char * ConfigName)
{
	
}
//@}
