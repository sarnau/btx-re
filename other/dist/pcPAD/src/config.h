/*
   ####################################################################################
   #                                                                                  #
   #                          Bildschirmtricks pcPAD V1.0.0                           #
   #                               pcPAD configuration                                #
   #                                                                                  #
   #    Copyright (C) 2008-2014 Philipp Fabian Benedikt Maier (aka. Dexter)           #
   #                                                                                  #
   #    This program is free software; you can redistribute it and/or modify          #
   #    it under the terms of the GNU General Public License as published by          #
   #    the Free Software Foundation; either version 2 of the License, or             #
   #    (at your option) any later version.                                           #
   #                                                                                  #
   #    This program is distributed in the hope that it will be useful,               #
   #    but WITHOUT ANY WARRANTY; without even the implied warranty of                #
   #    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 #
   #    GNU General Public License for more details.                                  #
   #                                                                                  #
   #    You should have received a copy of the GNU General Public License             #
   #    along with this program; if not, write to the Free Software                   #
   #    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA    #
   #                                                                                  #
   #################################################################################### */


/* ## HEADER ########################################################################## */
#ifndef CONFIG_H
#define CONFIG_H

/* ATTENTION! You must do a "make clean" and then "make" to make the changes take effect! */

/* Terminal line characteristics */
#define TERMINAL_BAUDRATE 1200				/* Baudrate of the terminal (Minibtx has 1200 baud in both directions) */
#define TERMINAL_DATABITS 8				/* Databits width (default is 8) */

/* General configuration */
#define VERSIONSTRING "V 1.0.0"				/* Version string */
#define DISPLAYPAGE_RETRYS 3				/* Configures how often the system retrys to display a page */
#define PAGE_HISTORY_SIZE 23				/* Configures how many entered pageIds the integrated history can store */

/* Ulm configuration */
#define BTX_ULM_INITIAL_PAGE "*index#"			/* Set the initial page (Wellcomescreen) */
#define BTX_ULM_GENERIC_ERROR_PAGE "*error#"		/* Set the initial page (Wellcomescreen) */
#define BTX_ULM_STYLE 1					/* Configure ulm style: 1=Static, 2=Dynamic (see readme file) */
#define BTX_ULM_PAGE_FILE_SUFFIX "btx"			/* File type prefix that will be used for http access */
#define BTX_ULM_PAGE_ID_MAXLENGTH 32			/* The length of the page id (see readme file) */
#define BTX_ULM_HYPERLINK_ID_MAXLENGTH 2		/* The lenght of an hyperlink id (recomanded value is 2) */
#define BTX_ULM_PAGE_BASENAME "page"			/* When style 2 is selected, this is the name of the page that generates the contnet */
#define BTX_ULM_PAGE_REQUESTVAR "id"			/* When style 2 is selected, this is the name of the variable that contains the request */
#define BTX_ULM_IP_RESOLVE_RETRYS 3			/* Configures how often the system retrys to resolve the ulm ip */
#define BTX_ULM_WGET_TEMP "temp.btx"			/* Temporary btx file */

/* Internal messages */
#define BTX_MESSAGE_HTTPERROR "SEITE NICHT VERFUEGBAR"		/* Message that is displayed if an http-error anounces */
#define BTX_MESSAGE_CONNECTIONERROR "ZENTRALE NICHT ERREICHBAR"	/* Message that is displayed when there is a problem with the ulm connection */
#define BTX_MESSAGE_PARSEERROR "SEITE BESCHAEDIGT"		/* Message that is displayed when parse errors anounces */

/* Cept parser configuration: */
#define BTX_CEPT_HYPERTEXT_BUFFERSIZE 30000		/* Maximal size of a hypertext cept-page the PAD can process */
#define BTX_CEPT_BINARY_BUFFERSIZE 8000			/* Maximal size of a binary cept-page the PAD can process */
#define BTX_CEPT_TAG_MAXSIZE 32				/* Maximal size of a single tag */
#define BTX_CEPT_META_TAG_MAXCOUNT 8			/* Maximal number of meta tags that can be stored in one page structure */
#define BTX_CEPT_META_TAG_NAME_BUFFERSIZE 32+1		/* Maximal size of a meta tag name (+1 for \0) */
#define BTX_CEPT_META_TAG_CONTENT_BUFFERSIZE 256+1	/* Maximal size of a meta tag content (+1 for \0) */
#define BTX_CEPT_META_TAG_HYPERLINKLIST_BUFFER 256	/* Hyperlinklist buffer size */

#endif /*CONFIG_H*/
/* #################################################################################### */

