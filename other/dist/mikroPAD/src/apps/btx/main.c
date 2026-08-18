/*
   ####################################################################################
   #                                                                                  #
   #                        Bildschirmtricks MikroPAD V2.0.0                          #
   #                                 program control                                  #
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
#include "system/stdout/stdout.h"	/* Include STOUT control utility */
#include "system/clock/clock.h"		/* Include Timing utilities */

#include "btx.h"			/* Include btx service control */
#include "config.h"			/* Include btx configuration */
#include "http.h"			/* Include http client */
#include "cept.h"			/* Include cept parser */
#include "termctrl.h"			/* Include termcontrol */
#include "test.h"			/* Include test routines */
#include "speaker.h"			/* Include Speaker drivers */

/* Errors return-codes for applicationBtxDisplayPage() */
#define DISPPAGE_IP_RESOLVEERROR -1	/* Error: The system could not resolve the ULM-IP */
#define DISPPAGE_HTTP_FATALERROR -2	/* Error: There was a fatal error while performing the htt-get request */
#define DISPPAGE_CEPT_PARSEERROR -3	/* Error: The page could not be parsed */
#define DISPPAGE_TERMINAL_NOT_READY -4  /* Error: The terminal was not ready */
/* HTTP-Errors are passed as negative values (e.g -404 for Not found) */


/* #################################################################################### */


/* #################################################################################### */
/* Show a quick message (for errors, status messages and so on) */
static void applicationBtxMessage(char *message)
{
      	btxCeptPageData btxCeptPage;

#if (DEBUGPORT == 1)
	printf_P(PSTR("* Displaying status message on terminal...\r\n"));		/* Format message */
#endif /*DEBUGPORT*/
	sprintf_P(btxCeptPage.ceptPage,PSTR(F_CEPT_CS F_CEPT_APD F_CEPT_APD F_CEPT_DBH "%s" F_CEPT_APH F_CEPT_NSZ F_CEPT_APD F_CEPT_APD),message);

	btxCeptPage.ceptPageLength = strlen(btxCeptPage.ceptPage);			/* Fill in a correct length */
	applicationBtxCeptTransmit(&btxCeptPage);

	return;
}

/* Fetch a page from server and display it */
static int applicationBtxDisplayPage(char *btxPageId, btxCeptPageData *btxPageMemory)
{
	unsigned long ulmIp;								/* ip-adress of ulm */
	union IP_ADDRESS ip2string;
	char ceptHypertextBuffer[BTX_CEPT_HYPERTEXT_BUFFERSIZE];
	char pageUrl[BTX_ULM_PAGE_ID_MAXLENGTH + sizeof(BTX_ULM_PAGE_BASENAME) + sizeof(BTX_ULM_PAGE_REQUESTVAR) + 32];
	int ulmHttpState;

#if (DEBUGPORT == 1)
	printf_P(PSTR("Processing page request...\r\n"));				/* Fetch the initial page */
	printf_P(PSTR("* Transforming btx page id (%s) to http url\r\n"),btxPageId);	/* Transform btx page id to http url */
#endif /*DEBUGPORT*/

	applicationBtxGenUrl(btxPageId,pageUrl);

#if (DEBUGPORT == 1)
	printf_P(PSTR("  ==> %s\r\n"),pageUrl);
	printf_P(PSTR("* Resolving Ulm IP...\r\n"));					/* Resolve Ulm IP */
#endif /*DEBUGPORT*/

	if(applicationBtxResolveUlm(&ulmIp) != 0)
	{
#if (DEBUGPORT == 1)
		printf_P(PSTR("  ==> Fatal Error: Could not resolve Ulm-IP -- aborting!\r\n"));
#endif /*DEBUGPORT*/
		return DISPPAGE_IP_RESOLVEERROR;
	}
	else
	{
		ip2string.IP = ulmIp;
#if (DEBUGPORT == 1)
		printf_P(PSTR("  ==> Ulm is at: %i.%i.%i.%i\r\n"),ip2string.IPbyte[0],ip2string.IPbyte[1],ip2string.IPbyte[2],ip2string.IPbyte[3]);
#endif /*DEBUGPORT*/
	}

#if (DEBUGPORT == 1)
	printf_P(PSTR("* Downloading hypertext cept page from ulm\r\n"));		/* Download page */
#endif /*DEBUGPORT*/

	if(applicationBtxHttpGet(pageUrl,ulmIp,ceptHypertextBuffer,&ulmHttpState) == 0)
	{
		if(ulmHttpState != 200)
		{
#if (DEBUGPORT == 1)
			printf_P(PSTR("  ==> HTTP status is %i -- aborting!\r\n"),ulmHttpState);
#endif /*DEBUGPORT*/
			return -1*(ulmHttpState);
		}
#if (DEBUGPORT == 1)
		else
			printf_P(PSTR("  ==> HTTP status is %i -- request successful!\r\n"),ulmHttpState);
#endif /*DEBUGPORT*/
	}
	else
	{
#if (DEBUGPORT == 1)
		printf_P(PSTR("  ==> Fatal error during get request -- aborting!\r\n"));
#endif /*DEBUGPORT*/
		return DISPPAGE_HTTP_FATALERROR;
	}
#if (DEBUGPORT == 1)
	printf_P(PSTR("* Parsing cept-hypertext\r\n"));					/* Parse page */
#endif /*DEBUGPORT*/

	if(applicationBtxCeptParse(ceptHypertextBuffer,btxPageMemory) != 0)
	{
#if (DEBUGPORT == 1)
		printf_P(PSTR("  ==> Parse error -- aborting!\r\n"));
#endif /*DEBUGPORT*/
		return DISPPAGE_CEPT_PARSEERROR;
	}
#if (DEBUGPORT == 1)
	else
		printf_P(PSTR("  ==> Page successfully parsed!\r\n"));
#endif /*DEBUGPORT*/

#if (DEBUGPORT == 1)
	printf_P(PSTR("* Checking terminal state...\r\n"));				/* Check terminal state */
#endif /*DEBUGPORT*/

	if(applicationBtxTermctrlGetReadyState() != TERMCTRL_TERMINAL_READY)
	{
#if (DEBUGPORT == 1)
		printf_P(PSTR("  ==> Terminal not ready -- aborting!\r\n"));
#endif /*DEBUGPORT*/
		return DISPPAGE_TERMINAL_NOT_READY;
	}

#if (DEBUGPORT == 1)
	printf_P(PSTR("* Transmitting page...\r\n"));					/* Transmit page to terminal */
#endif /*DEBUGPORT*/
	applicationBtxCeptTransmit(btxPageMemory);

	if(applicationBtxHistoryPush(btxPageId) == 0)
	{
#if (DEBUGPORT == 1)
	printf_P(PSTR("* Storing page (%s) in history.\r\n"),btxPageId);
#endif /*DEBUGPORT*/
	}	
	applicationBtxHistoryBlockCtrl(HISTORY_UNBLOCKED);				/* Make history writable again for future inputs */

#if (DEBUGPORT == 1)
	printf_P(PSTR("  ==> Done!\r\n"));
#endif /*DEBUGPORT*/

	return 0;
}

/* Initalize Bildschirmtrix daemon */
void applicationBtxInit(void)
{
	union IP_ADDRESS ip2string;
	btxCeptPageData btxCeptPage;						/* Contains the page information of the last displayed page */
	int retrycounter = 0;
	unsigned char retryTimerHandle;
	int userInputLen = 0;
	char generalProposeBuffer[1024];

	char pageIdBuffer[BTX_ULM_PAGE_ID_MAXLENGTH+1] = BTX_ULM_INITIAL_PAGE;
	char pageMetaContentBuffer[BTX_CEPT_META_TAG_CONTENT_BUFFERSIZE];
	int pageDisplayStatus;
	int i;

	/* Initialize PAD */
	applicationBtxTermctrlInit();
	applicationBtxTermctrlPortSelect(TERMCTRL_V24_TERMINAL);
	applicationBtxSpeakerInit();						/* Initialize Speaker (for tests and so on) */

	applicationBtxSpeakerPlayTone(8,200);					/* Arcustic boot success message */

	STDOUT_Set_RS232();

#if (DEBUGPORT == 0)
	printf_P(PSTR("\r\n\r\n\r\nmikroPAD: Debug port disabled!\r\n"));
	applicationBtxTermctrlPortSelect(TERMCTRL_BTX_TERMINAL);		/* Select BTX-Terminal for input/output (one for all times!) */
#endif /*DEBUGPORT*/

#if (DEBUGPORT == 1)
	printf_P(PSTR("\r\n\r\n\r\n\r\n\a"));
	printf_P(PSTR("\t\t\t      PPPPP     AAAAA    DDDDD\r\n"));
	printf_P(PSTR("\t\t\t      PP  PP   AA   AA   DD  DDD\r\n"));
	printf_P(PSTR("  mmm mmm  ii  k k   rr  ooo  PPPPP    AAAAAAA   DD   DD\r\n"));
	printf_P(PSTR("  m  m  m  ii  kk  rr   o  o  PP       AA   AA   DD   DD\r\n"));
	printf_P(PSTR("  m     m  ii  k k rr   oooo  PP       AA   AA   DDDDDDD\r\n"));
	printf_P(PSTR("  ******************************************************\r\n"));
	printf_P(PSTR("  *\t\t\t\t\t\t       *\r\n"));
	printf_P(PSTR("  *\t      ******************************\t       *\r\n"));
	printf_P(PSTR("  *\t      *         **********         *\t       *\r\n"));
	printf_P(PSTR("  *\t      *       *\t\t   *       *\t       *\r\n"));
	printf_P(PSTR("  *\t      *     *\t\t     *     *\t       *\r\n"));
	printf_P(PSTR("  *         *   *** *\t\t     * ***   *         *\r\n"));
	printf_P(PSTR("  *       *   *   * *\t\t     * *   *   *       *\r\n"));
	printf_P(PSTR("    *   *   *     * *\t\t     * *     *   *   *\r\n"));
	printf_P(PSTR("      *   *       * *\t\t     * *       *   *\r\n"));
	printf_P(PSTR("\t*\t  *   *\t\t   *   *         *\r\n"));
	printf_P(PSTR("\t*\t    *   **********   *           *\r\n"));
	printf_P(PSTR("\t*\t      **************             *\r\n"));
	printf_P(PSTR("\t*\t\t\t\t\t *\r\n"));
	printf_P(PSTR("\t*\t\t\t\t\t *\r\n"));
	printf_P(PSTR("\t*\t\t\t\t\t *\r\n"));
	printf_P(PSTR("\t*\t\t\t\t\t *\r\n"));
	printf_P(PSTR("\t******************************************\r\n"));
	printf_P(PSTR("\r\n"));
	printf_P(PSTR("__________________________________________________________________________\r\n"));
	printf_P(PSTR("Bildschirmtrix Videotex / Page Assembler Device " VERSIONSTRING "\r\n"));
	printf_P(PSTR("Copyright (c)2008 Philipp Fabian Benedikt Maier\r\n"));	
	printf_P(PSTR("\r\n"));
#endif /*DEBUGPORT*/

	applicationBtxTestRun();						/* Run integrated test routines (if test switch is pressed) */

#if (DEBUGPORT == 1)
	printf_P(PSTR("Configuration:\r\n"));
	printf_P(PSTR("* Ulm: " BTX_ULM_HOST "\r\n"));
	printf_P(PSTR("* Port: %i\r\n"),BTX_ULM_PORT);
	printf_P(PSTR("* Initial page: " BTX_ULM_INITIAL_PAGE "\r\n"));
	ip2string.IP = myIP;
	printf_P(PSTR("* Terminal: %i.%i.%i.%i\r\n"),ip2string.IPbyte[0],ip2string.IPbyte[1],ip2string.IPbyte[2],ip2string.IPbyte[3]);
	printf_P(PSTR("\r\n"));
	printf_P(PSTR("* Waiting for terminal...\r\n"));			/* Wait for terminal */
#endif /*DEBUGPORT*/

	while(applicationBtxTermctrlGetReadyState() != TERMCTRL_TERMINAL_READY);

	applicationBtxSpeakerPlayTone(8,800);					/* Arcustic ready state notification */
	applicationBtxSpeakerPlayTone(12,800);

	/* Launch Browser */
	while(1)								
	{
		/* Run down all redirections */
		do {								
			retryTimerHandle = CLOCK_RegisterCoundowntimer();				/* Register timer */
			/* Fetch page */
			retrycounter = 0;
			do
			{
				pageDisplayStatus = applicationBtxDisplayPage(pageIdBuffer,&btxCeptPage);
	
				if(pageDisplayStatus == 0)
				{
					if(applicationBtxCeptGetMetaTag(&btxCeptPage,"load_timeout",pageMetaContentBuffer) == 0)
						CLOCK_delay(atoi(pageMetaContentBuffer) * 1000);
				}
				else
				{
					retrycounter++;

					CLOCK_SetCountdownTimer (retryTimerHandle, 100, MSECOUND );	/* Wait some time before trying again */
					while(CLOCK_GetCountdownTimer(retryTimerHandle) > 0);

					/* Note: The function SetCountdownTimer() has a cosmetic bug. Normaly you would expect that 
						 CLOCK_SetCountdownTimer (retryTimerHandle, 1000, MSECOUND ); would delay 1 second.
						 this is not correct here. You must cancel one of the zeros in the parameter to match
						 everything to reality. CLOCK_SetCountdownTimer (retryTimerHandle, 100, MSECOUND ); will
						 delay one second. Don't wonder about this, it is a cosmetic bug in the operating system */


					if(pageDisplayStatus == -404)
					{

						sprintf(pageIdBuffer,"*%i#", abs(pageDisplayStatus));
#if (DEBUGPORT == 1)
						printf_P(PSTR("HTTP-Error %i anounced, retrying with %s...\r\n"),abs(pageDisplayStatus),pageIdBuffer);
#endif /*DEBUGPORT*/
					}
					else if(pageDisplayStatus == DISPPAGE_CEPT_PARSEERROR)
					{
						strcpy(pageIdBuffer,"*error#");
#if (DEBUGPORT == 1)				
						printf_P(PSTR("Parse error anounced, retrying with %s...\r\n"),pageIdBuffer);
#endif /*DEBUGPORT*/
					}
#if (DEBUGPORT == 1)
					else					
						printf_P(PSTR("Errors while fetching the page, retrying...\r\n"));
#endif /*DEBUGPORT*/

				}

			}while((retrycounter < DISPLAYPAGE_RETRYS)&&(pageDisplayStatus != 0));
			CLOCK_ReleaseCountdownTimer(retryTimerHandle);		/* Free timer */

			/* Generate error message on screen */
			if(pageDisplayStatus == DISPPAGE_IP_RESOLVEERROR)
				applicationBtxMessage(BTX_MESSAGE_CONNECTIONERROR);
			else if(pageDisplayStatus == DISPPAGE_HTTP_FATALERROR)
				applicationBtxMessage(BTX_MESSAGE_CONNECTIONERROR);
			else if(pageDisplayStatus == DISPPAGE_CEPT_PARSEERROR)
				applicationBtxMessage(BTX_MESSAGE_PARSEERROR);
			else if(pageDisplayStatus < -100)
				applicationBtxMessage(BTX_MESSAGE_HTTPERROR);

			/* Check if there was a remote disconnect submitted with the page */
			if(applicationBtxCeptGetMetaTag(&btxCeptPage,"disconnect",generalProposeBuffer) == 0)
			{
#if (DEBUGPORT == 1)
				printf_P(PSTR("* Ulm has sent the remote disconnect signal -- disconnecting in %s seconds\r\n"),generalProposeBuffer);
#endif /*DEBUGPORT*/
				CLOCK_delay(1000*atoi(generalProposeBuffer));
				applicationBtxTermctrlTerminateConnection();
			}

		} while ((applicationBtxCeptGetMetaTag(&btxCeptPage,"load_page",pageIdBuffer) == 0)&&(pageDisplayStatus == 0));


		/* Handle user input */
#if (DEBUGPORT == 1)
		printf_P(PSTR("Processing user input...\r\n"));
#endif /*DEBUGPORT*/
		do 
		{
#if (DEBUGPORT == 1)
			printf_P(PSTR("* waiting for keyboard input...\r\n"));
#endif /*DEBUGPORT*/


			strcpy(generalProposeBuffer,pageIdBuffer);					/* Backup page id (warning, it's a cludge!) */
			userInputLen = applicationBtxCeptGetPageRequest(pageIdBuffer);			/* Read user input */

			for(i=0;i<userInputLen;i++)							/* Convert input to lower case */
			{
				if((pageIdBuffer[i] >= 'A')&&(pageIdBuffer[i] <= 'Z'))
					pageIdBuffer[i] += 32;
			}
			
			/* Check if the user wants to reload the page */
			if((strcmp(pageIdBuffer,"*00#") == 0)||(strcmp(pageIdBuffer,"*09#") == 0))
			{
				strcpy(pageIdBuffer,generalProposeBuffer);	
#if (DEBUGPORT == 1)
				printf_P(PSTR("* (*00#) or (*09#) pressed, reloading page (%s)...\r\n"),pageIdBuffer);
#endif /*DEBUGPORT*/
			}

			/* Check if the user wants to see the next page */
			if(strcmp(pageIdBuffer,"#") == 0)
			{
#if (DEBUGPORT == 1)
				printf_P(PSTR("* nextpage selected, trying to resloving nextpage...\r\n"));
#endif /*DEBUGPORT*/
				if(applicationBtxCeptGetMetaTag(&btxCeptPage,"next_page",pageIdBuffer) == 0)
				{
#if (DEBUGPORT == 1)
					printf_P(PSTR("* nextpage (%s) successfully resolved\r\n"),pageIdBuffer);
#endif /*DEBUGPORT*/
				}
#if (DEBUGPORT == 1)
				else
					printf_P(PSTR("* no nextpage defiend.\r\n"));
#endif /*DEBUGPORT*/
			}

			/* Check if the user wants to follow a hyperlink */
			if(applicationBtxCheckHyperlinkId(pageIdBuffer) == 0)					/* ...and check if the input matches an hyperlink request */
			{
#if (DEBUGPORT == 1)
				printf_P(PSTR("* Hyperlink request (%s) caught, proceeding...\r\n"),pageIdBuffer);
#endif /*DEBUGPORT*/
				applicationBtxResolveHyperlink(&btxCeptPage, pageIdBuffer, pageIdBuffer);	/* Try to resolve the hyperlink, a failure will be caught later */
			}

			applicationBtxHistoryBlockCtrl(HISTORY_UNBLOCKED);				/* Make history writable again for future inputs */

			/* Check if the user wants to load the previous page from history */
			if(strcmp(pageIdBuffer,"*#") == 0)
			{
				if(applicationBtxHistoryPop(pageIdBuffer) == 0)
				{
#if (DEBUGPORT == 1)
					printf_P(PSTR("* (*#) entered -- Restoring previous page (%s) from history.\r\n"),pageIdBuffer);
#endif /*DEBUGPORT*/
					applicationBtxHistoryBlockCtrl(HISTORY_BLOCKED);			/* Do not store history calls in history! */
				}
#if (DEBUGPORT == 1)
				else
					printf_P(PSTR("* End of history reached!\r\n"));
#endif /*DEBUGPORT*/
			}

			/* Check if the resulting page id is a correct one */
			if(applicationBtxCheckPageId(pageIdBuffer) != 0)					/* Check if a valid pageId come out */
			{
#if (DEBUGPORT == 1)
				printf_P(PSTR("* The page id (%s) is invalid -- retrying...\r\n"),pageIdBuffer);
#endif /*DEBUGPORT*/
				applicationBtxCeptRubOut(userInputLen);
			}
#if (DEBUGPORT == 1)
			else
				printf_P(PSTR("* The page id (%s) is valid.\r\n"),pageIdBuffer);
#endif /*DEBUGPORT*/
		} while (applicationBtxCheckPageId(pageIdBuffer) != 0);						/* We are done if we got a valid pageId */

	}


	return;
}

/* #################################################################################### */
