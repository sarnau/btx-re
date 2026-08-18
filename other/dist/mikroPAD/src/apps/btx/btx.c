 /*
   ####################################################################################
   #                                                                                  #
   #                        Bildschirmtricks MikroPAD V2.0.0                          #
   #                                btx service control                               #
   #                                                                                  #
   #    Copyright (C) 2008 Philipp Fabian Benedikt Maier (aka. Dexter)                #
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
#include <avr/pgmspace.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "system/net/ip.h"		/* Include IP protocol layer */
#include "system/net/tcp.h"		/* Include TCP protocol layer */
#include "system/net/dns.h"		/* Include DNS resolver utility */
#include "system/clock/clock.h"		/* Include Timing utilities */

#include "btx.h"			/* Include own header file */
#include "config.h"			/* Include btx configuration */
#include "cept.h"			/* Include cept parser */

static char pageIdHistory[PAGE_HISTORY_SIZE][BTX_ULM_PAGE_ID_MAXLENGTH+1];		/* Storage space for history */
static int pageIdHistoryPosition = 0;							/* Courrent position in the ringbuffer */
static int pageIdHistoryFillstate = 0;							/* Fillstate of the history */
static int pageIdHistoryBlocked = HISTORY_UNBLOCKED ;					/* History blocking */

/* #################################################################################### */


/* ## NETWORK CONTROL ################################################################# */
/* Resolve Ulm IP-Adress */
int applicationBtxResolveUlm(unsigned long *ulmIp)
{
	int retrys = 0;
	unsigned char retryTimerHandle;

	retryTimerHandle = CLOCK_RegisterCoundowntimer();				/* Register timer */

	while(retrys < BTX_ULM_IP_RESOLVE_RETRYS)
	{
	    *ulmIp = DNS_ResolveName(BTX_ULM_HOST);

		if(*ulmIp != DNS_NO_ANSWER)
		{	
			return 0;
			CLOCK_ReleaseCountdownTimer(retryTimerHandle);		/* Free timer */
		}
		retrys++;

		CLOCK_SetCountdownTimer (retryTimerHandle, 100, MSECOUND );	/* Wait some time before trying again */
		while(CLOCK_GetCountdownTimer(retryTimerHandle) > 0);
		
		/* Note: The function SetCountdownTimer() has a cosmetic bug. Normaly you would expect that 
			 CLOCK_SetCountdownTimer (retryTimerHandle, 1000, MSECOUND ); would delay 1 second.
                         this is not correct here. You must cancel one of the zeros in the parameter to match
                         everything to reality. CLOCK_SetCountdownTimer (retryTimerHandle, 100, MSECOUND ); will
                         delay one second. Don't wonder about this, it is a cosmetic bug in the operating system */
	}

	CLOCK_ReleaseCountdownTimer(retryTimerHandle);		/* Free timer */
	return -1;		/* There is a problem resolving the ip-address */
}
/* #################################################################################### */


/* ## SUPPLEMENTARY FUNCTIONS ######################################################### */
/* Check if the page id is valid */
int applicationBtxCheckPageId(char *btxPageId)
{
	char *btxPageIdPointer = btxPageId;
	int lengthCount = 0;

	if(*btxPageIdPointer == '*')
	{
		btxPageIdPointer++;

		while(*btxPageIdPointer != '#')	/* Valid page ids are terminated with a '#' */
		{
			if((*btxPageIdPointer == '\0')||(*btxPageIdPointer == ' ')||(lengthCount >= BTX_ULM_PAGE_ID_MAXLENGTH))
				return -1;	/* Null terminator met or page id too long, page-id is invalid */
			lengthCount++;
			btxPageIdPointer++;
		}

		return 0;
	}
	else
		return -1;			/* # is missing, page-id is invalid */
}

/* Check if the hyperlink id is valid */
int applicationBtxCheckHyperlinkId(char *btxHyperlinkId)
{
	if(strlen(btxHyperlinkId) == 2)
	{
		if((btxHyperlinkId[0] >= 0x30)&&(btxHyperlinkId[0] <= 0x39))
			if((btxHyperlinkId[1] >= 0x30)&&(btxHyperlinkId[1] <= 0x39))
				return 0;
	}
	else
		return -1;			/* The hyperlink is not 2 bytes large, so it can't be valid */

	/* FIXME, not all cases covered, missing returncode here! */
}

/* Convert a btx page identifier (e.g *123456#) to an http-style url */
int applicationBtxGenUrl(char *btxPageId, char *url)
{
	char *urlPonter = url;
	char *btxPageIdPointer = btxPageId;
	int i;

#if (BTX_ULM_STYLE == 1)
	char fileSuffix[sizeof(BTX_ULM_PAGE_FILE_SUFFIX)] = BTX_ULM_PAGE_FILE_SUFFIX;

	*urlPonter = '/';				/* Add a the leading slash */
	urlPonter++;

	btxPageIdPointer++;				/* Append page id (without * and #) */
	while(*btxPageIdPointer != '#')
	{
		if(*btxPageIdPointer == '\0')		/* Null detected, something is wrong here ! */
			return -1;

		*urlPonter = *btxPageIdPointer;
		btxPageIdPointer++;
		urlPonter++;
	}

	*urlPonter = '.';				/* Add a the dot between name and ending */
	urlPonter++;


	for(i=0;i<sizeof(BTX_ULM_PAGE_FILE_SUFFIX);i++)	/* Append the file suffix */
	{
		*urlPonter = fileSuffix[i];
		urlPonter++;
	}

	*urlPonter = '\0';

#elif (BTX_ULM_STYLE == 2)
	char fileSuffix[sizeof(BTX_ULM_PAGE_FILE_SUFFIX)] = BTX_ULM_PAGE_FILE_SUFFIX;
	char fileName[sizeof(BTX_ULM_PAGE_BASENAME)] = BTX_ULM_PAGE_BASENAME;
	char requestVar[sizeof(BTX_ULM_PAGE_REQUESTVAR)] = BTX_ULM_PAGE_REQUESTVAR;

	*urlPonter = '/';				/* Add a the leading slash */

	for(i=0;i<sizeof(BTX_ULM_PAGE_BASENAME);i++)	/* Append the file suffix */
	{
		urlPonter++;
		*urlPonter = fileName[i];
	}

	*urlPonter = '.';				/* Add a the dot between name and ending */

	for(i=0;i<sizeof(BTX_ULM_PAGE_FILE_SUFFIX);i++)	/* Append the file suffix */
	{
		urlPonter++;
		*urlPonter = fileSuffix[i];
	}

	*urlPonter = '?';				/* Add a the question mark for the request */

	for(i=0;i<sizeof(BTX_ULM_PAGE_REQUESTVAR);i++)	/* Append the name of the request variable */
	{
		urlPonter++;
		*urlPonter = requestVar[i];
	}

	*urlPonter = '=';				/* Add a the equal mark for the request */
	urlPonter++;

	btxPageIdPointer++;				/* Append page id (without * and #) */
	while(*btxPageIdPointer != '#')
	{
		if(*btxPageIdPointer == '\0')		/* Null detected, something is wrong here ! */
			return -1;

		*urlPonter = *btxPageIdPointer;
		btxPageIdPointer++;
		urlPonter++;
	}

	*urlPonter = '\0';
#endif

	return 0;
}
/* Store a btx page in history. */
int applicationBtxHistoryPush(char *btxPageId)
{
	static char previousPageId[BTX_ULM_PAGE_ID_MAXLENGTH + 1];

	if((pageIdHistoryBlocked == HISTORY_UNBLOCKED)&&(strlen(btxPageId) <= BTX_ULM_PAGE_ID_MAXLENGTH))
	{
		strcpy(pageIdHistory[pageIdHistoryPosition],previousPageId);	/* A cludge, but it works */
		strcpy(previousPageId,btxPageId);

		if(pageIdHistoryPosition < PAGE_HISTORY_SIZE)
			pageIdHistoryPosition++;
		else
			pageIdHistoryPosition = 0;

		if(pageIdHistoryFillstate < PAGE_HISTORY_SIZE)
			pageIdHistoryFillstate++;

		return 0;
	}
	else
		return -1;
}

/* Restore a btx page from history. */
int applicationBtxHistoryPop(char *btxPageId)
{
	if(pageIdHistoryFillstate > 0)
	{
		if(pageIdHistoryPosition > 0)
			pageIdHistoryPosition--;
		else
			pageIdHistoryPosition = PAGE_HISTORY_SIZE;	

		strcpy(btxPageId,pageIdHistory[pageIdHistoryPosition]);

		pageIdHistoryFillstate--;
	
		return 0;
	}
	else
		return -1;
}

/* Block/Unblock history */
int applicationBtxHistoryBlockCtrl(int mode)
{
	/* FIXME, no returncode! */
	pageIdHistoryBlocked = mode;
}



/* #################################################################################### */

