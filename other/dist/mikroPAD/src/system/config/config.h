/*! \file config.h \brief Stellt Config-tools bereit */
//***************************************************************************
//*            config.h
//*
//*  Son Aug 10 16:25:49 2008
//*  Copyright  2008  Dirk Broßwick
//*  Email: sharandac@snafu.de
///	\ingroup hardware
///	\defgroup config Die Config (config.h)
///	\par Uebersicht
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
 
#ifndef _CONFIG_H
	#define CONFIG_H
	
	void eetest( void );

	char readConfig_P( char * ConfigName, char * ConfigValue );
	char writeConfig_P( char * ConfigName, char * ConfigValue );
	char changeConfig_P( char * ConfigName, char * ConfigValue );
	char deleteConfig_P( char * ConfigName);
	char readConfig( char * ConfigName, char * ConfigValue );
	char writeConfig( char * ConfigName, char * ConfigValue );
	char changeConfig( char * ConfigName, char * ConfigValue );
	char deleteConfig( char * ConfigName);

	#define cfglen 	4093

    typedef struct {
		char	TAG[3];
		char	CFG[ cfglen ];
	} Config ;

#endif /* CONFIG_H */

//@}
