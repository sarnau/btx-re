/*
   ####################################################################################
   #                                                                                  #
   #                          Bildschirmtricks pcPAD V1.0.0                           #
   #                                 program control                                  #
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "btx.h"			/* Include btx service control */
#include "config.h"			/* Include btx configuration */
#include "http.h"			/* Include http client */
#include "cept.h"			/* Include cept parser */
#include "termctrl.h"			/* Include termcontrol */
//#include "test.h"			/* Include test routines */
//#include "speaker.h"			/* Include Speaker drivers */

/* Errors return-codes for applicationBtxDisplayPage() */
#define DISPPAGE_IP_RESOLVEERROR -1	/* Error: The system could not resolve the ULM-IP */
#define DISPPAGE_HTTP_FATALERROR -2	/* Error: There was a fatal error while performing the htt-get request */
#define DISPPAGE_CEPT_PARSEERROR -3	/* Error: The page could not be parsed */
#define DISPPAGE_TERMINAL_NOT_READY -4  /* Error: The terminal was not ready */
/* HTTP-Errors are passed as negative values (e.g -404 for Not found) */


/* #################################################################################### */


/* #################################################################################### */
/* Show a quick message (for errors, status messages and so on) */
static void applicationBtxMessage(char *message, char *port)
{
	printf("== Status ==\n");
      	btxCeptPageData btxCeptPage;
	printf("* Displaying status message on terminal...\n");		/* Format message */
	sprintf(btxCeptPage.ceptPage,F_CEPT_CS F_CEPT_APD F_CEPT_APD F_CEPT_DBH "%s" F_CEPT_APH F_CEPT_NSZ F_CEPT_APD F_CEPT_APD,message);
	btxCeptPage.ceptPageLength = strlen(btxCeptPage.ceptPage);			/* Fill in a correct length */
	applicationBtxCeptTransmit(&btxCeptPage,port);
	return;
}

/* Fetch a page from server and display it */
static int applicationBtxDisplayPage(char *btxPageId, btxCeptPageData *btxPageMemory, char *ulmAddr, char *port)
{
	char ceptHypertextBuffer[BTX_CEPT_HYPERTEXT_BUFFERSIZE];
	char pageUrl[BTX_ULM_PAGE_ID_MAXLENGTH + sizeof(BTX_ULM_PAGE_BASENAME) + sizeof(BTX_ULM_PAGE_REQUESTVAR) + 32];
	int ulmHttpState;

	printf("== Page request ==\n");				/* Fetch the initial page */
	printf(" * Transforming btx page id (%s) to http url\n",btxPageId);	/* Transform btx page id to http url */

	applicationBtxGenUrl(btxPageId,pageUrl);

	printf(" * Downloading hypertext cept page from ulm\n");		/* Download page */

	if(applicationBtxHttpGet(pageUrl,ulmAddr,ceptHypertextBuffer,&ulmHttpState) == 0)
	{
		if(ulmHttpState != 200)
		{
			printf("   ==> HTTP status is %i -- aborting!\n",ulmHttpState);
			printf("\n");
			return -1*(ulmHttpState);
		}
		else
			printf("   ==> HTTP status is %i -- request successful!\n",ulmHttpState);
	}
	else
	{
		printf("   ==> Fatal error during get request -- aborting!\n");
		return DISPPAGE_HTTP_FATALERROR;
	}

	printf(" * Parsing cept-hypertext\n");						/* Parse page */
	if(applicationBtxCeptParse(ceptHypertextBuffer,btxPageMemory) != 0)
	{
		printf("   ==> Parse error -- aborting!\n");
		printf("\n");
		return DISPPAGE_CEPT_PARSEERROR;
	}

	else
		printf("   ==> Page successfully parsed!\n");

	printf(" * Checking terminal state...\n");				/* Check terminal state */
	if(applicationBtxTermctrlGetReadyState() != TERMCTRL_TERMINAL_READY)
	{
		printf("   ==> Terminal not ready -- aborting!\n");
		return DISPPAGE_TERMINAL_NOT_READY;
	}

	printf(" * Transmitting page...\n");					/* Transmit page to terminal */
	applicationBtxCeptTransmit(btxPageMemory,port);
	if(applicationBtxHistoryPush(btxPageId) == 0)
		printf(" * Storing page (%s) in history.\n",btxPageId);
	applicationBtxHistoryBlockCtrl(HISTORY_UNBLOCKED);			/* Make history writable again for future inputs */

	printf("   ==> Done!\n");
	printf("\n");

	return 0;
}

/* Run BTX application */
void applicationBtx(char *ulmAddr, char *port)
{

//	union IP_ADDRESS ip2string;
	btxCeptPageData btxCeptPage;						/* Contains the page information of the last displayed page */
	int retrycounter = 0;
//	unsigned char retryTimerHandle;
	int userInputLen = 0;
	char generalProposeBuffer[1024];

	char pageIdBuffer[BTX_ULM_PAGE_ID_MAXLENGTH+1] = BTX_ULM_INITIAL_PAGE;
	char pageMetaContentBuffer[BTX_CEPT_META_TAG_CONTENT_BUFFERSIZE];
	int pageDisplayStatus;
	int i;

	printf("\n");
	printf("== Startup ==\n");
	printf(" * Parameters:\n");
	printf(" * Initial page: " BTX_ULM_INITIAL_PAGE "\n");
	printf("   Terminal is connected to port: %s\n",port);
	printf("   ULM is located in: %s\n",ulmAddr);


	printf(" * Starting BTX service...\n");

	/* Initalize terminal (serial port) */
	applicationBtxTermctrlInit(port);

	/* Wait for terminal */
	printf(" * Waiting for terminal...\n");
	while(applicationBtxTermctrlGetReadyState() != TERMCTRL_TERMINAL_READY);
	printf(" * Terminal has become ready...\n");
	printf("\n");


	/* Launch Browser */
	while(1)								
	{
		/* Run down all redirections */
		do {								
			/* Fetch page, try multiple times if necessary */
			retrycounter = 0;
			do
			{
				/* Fetch page */
				pageDisplayStatus = applicationBtxDisplayPage(pageIdBuffer,&btxCeptPage,ulmAddr,port);
				if(pageDisplayStatus == 0)
				{
					if(applicationBtxCeptGetMetaTag(&btxCeptPage,"load_timeout",pageMetaContentBuffer) == 0)
						sleep(atoi(pageMetaContentBuffer));
				}

				/* Error handling */
				else
				{
					retrycounter++;
					sleep(1);

					printf("== Error ==\n");

					if(pageDisplayStatus == -404)
					{
						sprintf(pageIdBuffer,"*%i#", abs(pageDisplayStatus));
						printf(" * Error: HTTP-Error %i occoured, retrying with %s...\n",abs(pageDisplayStatus),pageIdBuffer);
					}
					else if(pageDisplayStatus == DISPPAGE_CEPT_PARSEERROR)
					{
						strcpy(pageIdBuffer,BTX_ULM_GENERIC_ERROR_PAGE);
						printf(" * Error: Parsing the page failed -- retrying with %s...\n",pageIdBuffer);
					}
					else					
						printf(" * Error: fetching the page failed -- retrying...\n");

					printf("\n");
				}

			}while((retrycounter < DISPLAYPAGE_RETRYS)&&(pageDisplayStatus != 0));

			/* Show an internal error message if it was not possible to display the page */
			if(pageDisplayStatus == DISPPAGE_IP_RESOLVEERROR)
				applicationBtxMessage(BTX_MESSAGE_CONNECTIONERROR,port);
			else if(pageDisplayStatus == DISPPAGE_HTTP_FATALERROR)
				applicationBtxMessage(BTX_MESSAGE_CONNECTIONERROR,port);
			else if(pageDisplayStatus == DISPPAGE_CEPT_PARSEERROR)
				applicationBtxMessage(BTX_MESSAGE_PARSEERROR,port);
			else if(pageDisplayStatus < -100)
				applicationBtxMessage(BTX_MESSAGE_HTTPERROR,port);

			/* Check if there was a remote disconnect submitted with the page */
			if(applicationBtxCeptGetMetaTag(&btxCeptPage,"disconnect",generalProposeBuffer) == 0)
			{
				printf(" * Ulm has sent the remote disconnect signal -- disconnecting in %s seconds\n",generalProposeBuffer);
				sleep(atoi(generalProposeBuffer));
				applicationBtxTermctrlTerminateConnection();
			}

		} while ((applicationBtxCeptGetMetaTag(&btxCeptPage,"load_page",pageIdBuffer) == 0)&&(pageDisplayStatus == 0));

		/* Handle user input */
		printf("== User input ==\n");
		do 
		{
			printf(" * waiting for keyboard input...\n");
			strcpy(generalProposeBuffer,pageIdBuffer);					/* Backup page id (warning, it's a cludge!) */
			userInputLen = applicationBtxCeptGetPageRequest(pageIdBuffer,port);		/* Read user input */

			for(i=0;i<userInputLen;i++)							/* Convert input to lower case */
			{
				if((pageIdBuffer[i] >= 'A')&&(pageIdBuffer[i] <= 'Z'))
					pageIdBuffer[i] += 32;
			}
			
			/* Check if the user wants to reload the page */
			if((strcmp(pageIdBuffer,"*00#") == 0)||(strcmp(pageIdBuffer,"*09#") == 0))
			{
				strcpy(pageIdBuffer,generalProposeBuffer);	
				printf(" * (*00#) or (*09#) pressed, reloading page (%s)...\n",pageIdBuffer);
			}

			/* Check if the user wants to see the next page */
			if(strcmp(pageIdBuffer,"#") == 0)
			{
				printf(" * nextpage selected, trying to resloving nextpage...\n");

				if(applicationBtxCeptGetMetaTag(&btxCeptPage,"next_page",pageIdBuffer) == 0)
				{
					printf("* nextpage (%s) successfully resolved\n",pageIdBuffer);
				}
				else
					printf("* no nextpage defiend.\n");
			}

			/* Check if the user wants to follow a hyperlink */
			if(applicationBtxCheckHyperlinkId(pageIdBuffer) == 0)					/* ...and check if the input matches an hyperlink request */
			{
				printf("* Hyperlink request (%s) caught, proceeding...\n",pageIdBuffer);
				applicationBtxResolveHyperlink(&btxCeptPage, pageIdBuffer, pageIdBuffer);	/* Try to resolve the hyperlink, a failure will be caught later */
			}

			applicationBtxHistoryBlockCtrl(HISTORY_UNBLOCKED);				/* Make history writable again for future inputs */

			/* Check if the user wants to load the previous page from history */
			if(strcmp(pageIdBuffer,"*#") == 0)
			{
				if(applicationBtxHistoryPop(pageIdBuffer) == 0)
				{
					printf(" * (*#) entered -- Restoring previous page (%s) from history.\n",pageIdBuffer);
					applicationBtxHistoryBlockCtrl(HISTORY_BLOCKED);			/* Do not store history calls in history! */
				}

				else
					printf(" * End of history reached!\n");
			}

			/* Check if the resulting page id is a correct one */
			if(applicationBtxCheckPageId(pageIdBuffer) != 0)					/* Check if a valid pageId come out */
			{
				printf(" * The page id (%s) is invalid -- retrying...\n",pageIdBuffer);
				applicationBtxCeptRubOut(userInputLen,port);
			}

			else
				printf(" * The page id (%s) is valid.\n",pageIdBuffer);

			printf("\n");

		} while (applicationBtxCheckPageId(pageIdBuffer) != 0);						/* We are done if we got a valid pageId */
	}

	return;
}

/* #################################################################################### */
