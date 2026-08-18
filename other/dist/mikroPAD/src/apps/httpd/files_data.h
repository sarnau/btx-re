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
 
#ifndef _FILES_DATA_H
	#define FILES_DATA_H

	#define MAX_FILES_ENTRYS 		6

	typedef struct {
		const prog_char	*filesname;
		const prog_char	*files;
		const prog_char	filestype;
		const int	len;
	} const FILES ;

	#define	TEXT	0
	#define	JPEG	1
	#define PNG		2

const char files1[] PROGMEM = "index.html";
const char data1[] PROGMEM = {
	"<HTML>\r\n"
	"<HEAD>\r\n"
	"<TITLE>OpenMCP</TITLE>\r\n"
	"</HEAD>\r\n"
	"<frameset rows=\"60,35,*,40\" scrolling=\"no\" frameborder=\"2\" border=\"2\" framespacing=\"2\" bordercolor=\"#000000\">\r\n"
  	" <frame src=\"headline.html\" name=\"Navigation1\" scrolling=\"no\">\r\n"
  	" <frame src=\"mainmenu.html\" name=\"Navigation2\" scrolling=\"no\">\r\n"
  	" <frame src=\"stream.cgi\" name=\"main\" scrolling=\"no\">\r\n"
  	" <frame src=\"stats.cgi\" name=\"update\" scrolling=\"no\">\r\n"
  	" <noframes>\r\n"
    "  <body>\r\n"
    "   <p><a href=\"verweise.htm\">Navigation</a> <a href=\"startseite.htm\">Daten</a></p>\r\n"
    "  </body>\r\n"
  	" </noframes>\r\n"
	"</frameset>\r\n"
	"</HTML>\r\n"
	"\r\n"	};

const char files2[] PROGMEM = "headline.html";
const char data2[] PROGMEM = {
	"<HTML>\r\n"
	" <HEAD>\r\n"
	"  <TITLE>OpenMCP</TITLE>\r\n"
	" </HEAD>\r\n"
	" <BODY bgcolor=\"#6666FF\" text=\"#FFFFFF\">\r\n"
	"  <h1>OpenMCP</h1>\r\n"
	" </BODY>\r\n"
	"</HTML>\r\n"
	"\r\n"	};

const char files3[] PROGMEM = "stream.html";
const char data3[] PROGMEM = {
	"<HTML>\r\n"
	" <HEAD>\r\n"
	"  <TITLE>OpenMCP</TITLE>\r\n"
	"  <style type=\"text/css\">\r\n"
	"   a:link { text-decoration:none; font-weight:bold; color:#FFFFFF; }\r\n"
	"   a:visited { text-decoration:none; font-weight:bold; color:#FFFFFF; }\r\n"
	"   a:hover { text-decoration:none; font-weight:bold; background-color:#DDDDDD; }\r\n"
	"   a:active { text-decoration:none; font-weight:bold; background-color:#FFFFFF; }\r\n"
	"   a:focus { text-decoration:none; font-weight:bold; background-color:#DDDDDD; }\r\n"
	"  </style>\r\n"
	" </HEAD>\r\n"
	" <BODY bgcolor=\"#8888FF\" text=\"#FFFFFF\">\r\n"
	"  <a href=\"mainmenu.html\">zurueck</a> / <a href=\"stream.cgi\" target=\"main\">Stream</a> / <a href=\"stream.cgi?info\" target=\"main\">Infos</a> / <a href=\"stream.cgi?config\" target=\"main\">Konfiguration</a>\r\n"
	" </BODY>\r\n"
	" </HTML>\r\n"
	"\r\n"	};

const char files4[] PROGMEM = "mainmenu.html";
const char data4[] PROGMEM = {
	"<HTML>\r\n"
	" <HEAD>\r\n"
	"  <TITLE>OpenMCP</TITLE>\r\n"
	"  <style type=\"text/css\">\r\n"
	"   a:link { text-decoration:none; font-weight:bold; color:#FFFFFF; }\r\n"
	"   a:visited { text-decoration:none; font-weight:bold; color:#FFFFFF; }\r\n"
	"   a:hover { text-decoration:none; font-weight:bold; background-color:#DDDDDD; }\r\n"
	"   a:active { text-decoration:none; font-weight:bold; background-color:#FFFFFF; }\r\n"
	"   a:focus { text-decoration:none; font-weight:bold; background-color:#DDDDDD; }\r\n"
	"  </style>\r\n"
	" </HEAD>\r\n"
	" <BODY bgcolor=\"#8888FF\" text=\"#FFFFFF\">\r\n"
	"  <a href=\"info.html\"target=\"main\">Informationen</a> / <a href=\"stream.html\">Stream</a> / <a href=\"network.html\">Netzwerk</a> / <a href=\"system.html\">System</a>\r\n"
	" </BODY>\r\n"
	" </HTML>\r\n"
	"\r\n"	};


const char files5[] PROGMEM = "network.html";
const char data5[] PROGMEM = {
	"<HTML>\r\n"
	" <HEAD>\r\n"
	"  <TITLE>OpenMCP</TITLE>\r\n"
	"  <style type=\"text/css\">\r\n"
	"   a:link { text-decoration:none; font-weight:bold; color:#FFFFFF; }\r\n"
	"   a:visited { text-decoration:none; font-weight:bold; color:#FFFFFF; }\r\n"
	"   a:hover { text-decoration:none; font-weight:bold; background-color:#DDDDDD; }\r\n"
	"   a:active { text-decoration:none; font-weight:bold; background-color:#FFFFFF; }\r\n"
	"   a:focus { text-decoration:none; font-weight:bold; background-color:#DDDDDD; }\r\n"
	"  </style>\r\n"
	" </HEAD>\r\n"
	" <BODY bgcolor=\"#8888FF\" text=\"#FFFFFF\">\r\n"
	"  <a href=\"mainmenu.html\">zurueck</a> / <a href=\"network.cgi\" target=\"main\">Infos</a> / <a href=\"network.cgi?config\" target=\"main\">Konfiguration</a>\r\n"
	" </BODY>\r\n"
	" </HTML>\r\n"
	"\r\n"	};

const char files6[] PROGMEM = "system.html";
const char data6[] PROGMEM = {
	"<HTML>\r\n"
	" <HEAD>\r\n"
	"  <TITLE>OpenMCP</TITLE>\r\n"
	"  <style type=\"text/css\">\r\n"
	"   a:link { text-decoration:none; font-weight:bold; color:#FFFFFF; }\r\n"
	"   a:visited { text-decoration:none; font-weight:bold; color:#FFFFFF; }\r\n"
	"   a:hover { text-decoration:none; font-weight:bold; background-color:#DDDDDD; }\r\n"
	"   a:active { text-decoration:none; font-weight:bold; background-color:#FFFFFF; }\r\n"
	"   a:focus { text-decoration:none; font-weight:bold; background-color:#DDDDDD; }\r\n"
	"  </style>\r\n"
	" </HEAD>\r\n"
	" <BODY bgcolor=\"#8888FF\" text=\"#FFFFFF\">\r\n"
	"  <a href=\"mainmenu.html\">zurueck</a> / <a href=\"reset.cgi\" target=\"main\">Reset</a>\r\n"
	" </BODY>\r\n"
	" </HTML>\r\n"
	"\r\n"	};

FILES files[ MAX_FILES_ENTRYS ] = {
	{ files1, data1, TEXT, sizeof( data1 ) - 1 },
	{ files2, data2, TEXT, sizeof( data2 ) - 1 },
	{ files3, data3, TEXT, sizeof( data3 ) - 1 },
	{ files4, data4, TEXT, sizeof( data4 ) - 1 },
	{ files5, data5, TEXT, sizeof( data5 ) - 1 },
	{ files6, data6, TEXT, sizeof( data6 ) - 1 }
};

#endif /* FILES_DATA_H */
