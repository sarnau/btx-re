/*
   ####################################################################################
   #                                                                                  #
   #                        Bildschirmtricks MikroPAD V2.0.0                          #
   #                               http client layer                                  #
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
#include "hardware/uart/uart.h"		/* Include Uart hardware drivers */
#include "system/stdout/stdout.h"	/* Include STOUT control utility */
#include "system/clock/clock.h"		/* Include Timing utilities */

#include "config.h"			/* Include btx configuration */
#include "termctrl.h"			/* Include termcontrol */
#include "cept.h"			/* include own header file */

#define HEAD_PARSER_MODE 1		/* Parser control constants */
#define HYPERTEXT_PARSER_MODE 2
/* #################################################################################### */


/* ## CEPT PARSER ##################################################################### */
/* Trigger on a specified tag (Note: tag should be null terminated e.g. "<body>\0" ) */
static int applicationBtxCeptTriggerTag(char *data, char *tag)
{
	char *dataPointer = data;
	char *tagPointer = tag;

	while(*tagPointer != '\0')
	{
		if((*tagPointer != *dataPointer)||(*dataPointer == '\0'))
			return -1;			/* Not that tag what we are looking for */

		dataPointer++;
		tagPointer++;
	}

	return 0;					/* All chars match, this is the tag what we ar looking for */
}

/* Translate a cept tag to its binary pendant */
static int applicationBtxCeptTranslateTag(char *data, char *code, int *size)
{
	*size = 1;
	int sizecount = 0;
	char *dataPointer = data;
	char *tagPointer = data;
	char hexCode = 0;

	if(*dataPointer == '<')
	{
		dataPointer++;

		/* Calculate tag size */
		while(*dataPointer != '>')
		{
		
			if((sizecount >= BTX_CEPT_TAG_MAXSIZE)||(*dataPointer == '\0'))
			{
				*code = *data;
				*size = 1;
				return -1;			/* Invalid tag */
			}
			dataPointer++;
			sizecount++;
		}
		*size = sizecount + 2;				/* Tag + 2 chars for <> */

		/* Examine tag */
		if(applicationBtxCeptTriggerTag(tagPointer,"<0x\0") == 0)
		{
			tagPointer++;
			tagPointer++;
			tagPointer++;
			
			/* Note: I know this is not very witty but i was tired to code a 
                                 more sophisticated piece of code... */

			if(*tagPointer == '0')
				hexCode = 0;
			else if(*tagPointer == '1')
				hexCode = 16;
			else if(*tagPointer == '2')
				hexCode = 32;
			else if(*tagPointer == '3')
				hexCode = 48;
			else if(*tagPointer == '4')
				hexCode = 64;
			else if(*tagPointer == '5')
				hexCode = 80;
			else if(*tagPointer == '6')
				hexCode = 96;
			else if(*tagPointer == '7')
				hexCode = 112;
			else if(*tagPointer == '8')
				hexCode = 128;
			else if(*tagPointer == '9')
				hexCode = 144;
			else if(*tagPointer == 'a')
				hexCode = 160;
			else if(*tagPointer == 'b')
				hexCode = 176;
			else if(*tagPointer == 'c')
				hexCode = 192;
			else if(*tagPointer == 'd')
				hexCode = 208;
			else if(*tagPointer == 'e')
				hexCode = 224;
			else if(*tagPointer == 'f')
				hexCode = 240;

			tagPointer++;

			if(*tagPointer == '1')
				hexCode += 1;
			else if(*tagPointer == '2')
				hexCode += 2;
			else if(*tagPointer == '3')
				hexCode += 3;
			else if(*tagPointer == '4')
				hexCode += 4;
			else if(*tagPointer == '5')
				hexCode += 5;
			else if(*tagPointer == '6')
				hexCode += 6;
			else if(*tagPointer == '7')
				hexCode += 7;
			else if(*tagPointer == '8')
				hexCode += 8;
			else if(*tagPointer == '9')
				hexCode += 9;
			else if(*tagPointer == 'a')
				hexCode += 10;
			else if(*tagPointer == 'b')
				hexCode += 11;
			else if(*tagPointer == 'c')
				hexCode += 12;
			else if(*tagPointer == 'd')
				hexCode += 13;
			else if(*tagPointer == 'e')
				hexCode += 14;
			else if(*tagPointer == 'f')
				hexCode += 15;

			*code = hexCode;
		}
		else if(applicationBtxCeptTriggerTag(tagPointer,"<sp>\0") == 0)
			*code = ' ';
		else if(applicationBtxCeptTriggerTag(tagPointer,"<apb>\0") == 0)
			*code = CEPT_APB;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<apf>\0") == 0)
			*code = CEPT_APF;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<apd>\0") == 0)
			*code = CEPT_APD;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<apu>\0") == 0)
			*code = CEPT_APU;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<cs>\0") == 0)
			*code = CEPT_CS;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<apr>\0") == 0)
			*code = CEPT_APR;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<si>\0") == 0)
			*code = CEPT_SI;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<so>\0") == 0)
			*code = CEPT_SO;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<con>\0") == 0)
			*code = CEPT_CON;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<rpt>\0") == 0)
			*code = CEPT_RPT;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<cof>\0") == 0)
			*code = CEPT_COF;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<can>\0") == 0)
			*code = CEPT_CAN;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<ss2>\0") == 0)
			*code = CEPT_SS2;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<esc>\0") == 0)
			*code = CEPT_ESC;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<ss3>\0") == 0)
			*code = CEPT_SS3;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<aph>\0") == 0)
			*code = CEPT_APH;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<us>\0") == 0)
			*code = CEPT_US;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<nul>\0") == 0)
			*code = CEPT_NUL;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<soh>\0") == 0)
			*code = CEPT_SOH;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<stx>\0") == 0)
			*code = CEPT_STX;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<etx>\0") == 0)
			*code = CEPT_ETX;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<eot>\0") == 0)
			*code = CEPT_EOT;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<enq>\0") == 0)
			*code = CEPT_ENQ;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<ack>\0") == 0)
			*code = CEPT_ACK;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<itb>\0") == 0)
			*code = CEPT_ITB;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<dle>\0") == 0)
			*code = CEPT_DLE;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<nak>\0") == 0)
			*code = CEPT_NAK;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<syn>\0") == 0)
			*code = CEPT_SYN;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<etb>\0") == 0)
			*code = CEPT_ETB;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<ini>\0") == 0)
			*code = CEPT_INI;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<ter>\0") == 0)
			*code = CEPT_TER;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<dct>\0") == 0)
			*code = CEPT_DCT;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<fsh>\0") == 0)
			*code = CEPT_FSH;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<std>\0") == 0)
			*code = CEPT_STD;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<ebx>\0") == 0)
			*code = CEPT_EBX;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<sbx>\0") == 0)
			*code = CEPT_SBX;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<nsz>\0") == 0)
			*code = CEPT_NSZ;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<dbh>\0") == 0)
			*code = CEPT_DBH;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<dbw>\0") == 0)
			*code = CEPT_DBW;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<dbs>\0") == 0)
			*code = CEPT_DBS;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<cdy>\0") == 0)
			*code = CEPT_CDY;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<spl>\0") == 0)
			*code = CEPT_SPL;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<stl>\0") == 0)
			*code = CEPT_STL;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<csi>\0") == 0)
			*code = CEPT_CSI;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<abk>\0") == 0)
			*code = CEPT_ABK;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<anr>\0") == 0)
			*code = CEPT_ANR;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<ang>\0") == 0)
			*code = CEPT_ANG;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<any>\0") == 0)
			*code = CEPT_ANY;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<anb>\0") == 0)
			*code = CEPT_ANB;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<anm>\0") == 0)
			*code = CEPT_ANM;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<anc>\0") == 0)
			*code = CEPT_ANC;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<anw>\0") == 0)
			*code = CEPT_ANW;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<mbk>\0") == 0)
			*code = CEPT_MBK;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<msr>\0") == 0)
			*code = CEPT_MSR;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<msg>\0") == 0)
			*code = CEPT_MSG;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<msy>\0") == 0)
			*code = CEPT_MSY;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<msb>\0") == 0)
			*code = CEPT_MSB;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<msm>\0") == 0)
			*code = CEPT_MSM;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<msc>\0") == 0)
			*code = CEPT_MSC;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<msw>\0") == 0)
			*code = CEPT_MSW;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<bbd>\0") == 0)
			*code = CEPT_BBD;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<nbd>\0") == 0)
			*code = CEPT_NBD;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<hms>\0") == 0)
			*code = CEPT_HMS;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<rms>\0") == 0)
			*code = CEPT_RMS;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<bkf>\0") == 0)
			*code = CEPT_BKF;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<rdf>\0") == 0)
			*code = CEPT_RDF;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<grf>\0") == 0)
			*code = CEPT_GRF;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<ylf>\0") == 0)
			*code = CEPT_YLF;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<blf>\0") == 0)
			*code = CEPT_BLF;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<mgf>\0") == 0)
			*code = CEPT_MGF;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<cnf>\0") == 0)
			*code = CEPT_CNF;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<whf>\0") == 0)
			*code = CEPT_WHF;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<bkb>\0") == 0)
			*code = CEPT_BKB;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<rdb>\0") == 0)
			*code = CEPT_RDB;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<grb>\0") == 0)
			*code = CEPT_GRB;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<ylb>\0") == 0)
			*code = CEPT_YLB;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<blb>\0") == 0)
			*code = CEPT_BLB;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<mgb>\0") == 0)
			*code = CEPT_MGB;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<cnb>\0") == 0)
			*code = CEPT_CNB;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<whb>\0") == 0)
			*code = CEPT_WHB;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<npo>\0") == 0)
			*code = CEPT_NPO;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<ipo>\0") == 0)
			*code = CEPT_IPO;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<trb>\0") == 0)
			*code = CEPT_TRB;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<stc>\0") == 0)
			*code = CEPT_STC;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<fbkb>\0") == 0)
			*code = CEPT_FBKB;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<frdb>\0") == 0)
			*code = CEPT_FRDB;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<fgrb>\0") == 0)
			*code = CEPT_FGRB;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<fylb>\0") == 0)
			*code = CEPT_FYLB;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<fblb>\0") == 0)
			*code = CEPT_FBLB;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<fmgb>\0") == 0)
			*code = CEPT_FMGB;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<fcnb>\0") == 0)
			*code = CEPT_FCNB;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<fwhb>\0") == 0)
			*code = CEPT_FWHB;
		else if(applicationBtxCeptTriggerTag(tagPointer,"<ftrb>\0") == 0)
			*code = CEPT_FTRB;
		else
		{
			*size = 1;
			*code = *data;
			return -1;				/* Invalid tag! */
		}

		return 0;
	}
	else
	{
		*size = 1;
		*code = *data;
		return -1;					/* Invalid tag! */
	}
}

/* Strip everthing that is encapsulated by <!-- --> */
static int applicationBtxCeptStripComments(char *data)
{
	int ignoreData = 0;
	char *dataPointer = data;
	char *dataAheadPointer = data;

	while(*dataAheadPointer != '\0')
	{
		if(applicationBtxCeptTriggerTag(dataAheadPointer, "<!--\0") == 0)
			ignoreData = 1;

		if(applicationBtxCeptTriggerTag(dataAheadPointer, "-->\0") == 0)
		{
			ignoreData = 0;
			dataAheadPointer += 3;
		}

		*dataPointer = *dataAheadPointer;

		if(ignoreData == 0)
			dataPointer++;

		dataAheadPointer++;
	}

	*dataPointer = '\0';

	return 0;
}

/* Chech if the character is non pintable. Return 0 when printable and -1 when non printable */
static int applicationBtxCeptCheckPrintable(char *data)
{
	if(*data == ' ')				/* In our case a space is a non printable char (We have <sp> for that) */
		return -1;
	else if ((*data >= 0x20)&&(*data <= 0x7E))	/* Printable char */
		return 0;
	else if(*data == '\0')				/* Do not destroy \0 terminator */
		return 0;
	else						/* All other chars are threated as non printable */
		return -1;
}

/* Strip non printable characters */
static int applicationBtxCeptStripNonPrintable(char *data)
{
	char *dataPointer = data;
	char *dataAheadPointer = data;

	while(*dataAheadPointer != '\0')
	{
		if(applicationBtxCeptCheckPrintable(dataAheadPointer) == -1)
			dataAheadPointer++;
		else
		{
			*dataPointer = *dataAheadPointer;
			dataPointer++;
			dataAheadPointer++;
		}
	}

	*dataPointer = '\0';

	return 0;
}

/* Extract content and name from a meta tag string */
static int applicationBtxParseMetaTag(char *data, char *name, char *content)
{
	char *dataPointer = data;
	char *namePointer = name;
	char *contentPointer = content;
	int copyControl = 0;
	int nameSizeCount = 0;			/* Size counter to prevent buffer overflow */
	int contentSizeCount = 0;

	while(*dataPointer != '>')
	{
		if(*dataPointer == '\0')
			return -1;		/* Something has gone completely wrong */

		if(*dataPointer == '\"')	/* Switch off copy control when delimeter character passes by */
			copyControl = 0;

		if(copyControl == 1)
		{
			if(nameSizeCount < (BTX_CEPT_META_TAG_NAME_BUFFERSIZE -1))
			{
				*namePointer = *dataPointer;
				namePointer++;
				nameSizeCount++;
			}
			else
				return -1;
		}
		else if(copyControl == 2)
		{
			if(contentSizeCount < (BTX_CEPT_META_TAG_CONTENT_BUFFERSIZE -1))
			{
				*contentPointer = *dataPointer;
				contentPointer++;
				contentSizeCount++;
			}
			else
				return -1;
		}
		else
		{
			if((applicationBtxCeptTriggerTag(dataPointer,"name=\"\0") == 0)) 
			{
				dataPointer += 5;
				copyControl = 1;
			}
			else if((applicationBtxCeptTriggerTag(dataPointer,"content=\"\0") == 0)) 
			{
				dataPointer += 8;
				copyControl = 2;
			}
		}

		dataPointer++;
	}

	*namePointer = '\0';
	*contentPointer = '\0';

	if(copyControl != 0)	/* If copy control flag is not zero something gone wrong ! */
		return -1;
	else
		return 0;
}


/* Parse the raw data that was received via http to a btx-cept-page */
int applicationBtxCeptParse(char *dataIn, btxCeptPageData *page)
{
	char *dataInPointer = dataIn;
	char *dataOutPointer = page->ceptPage;

	int tagSize;
	char binaryCode;

	int parserMode = 0;				/* Parser control flags */ 
	int parserActive = 0;
	int tagHistory = 0;

	page->ceptPageLength = 0;			/* Initalize data in the page structure */
	page->metaTagCount = 0;

	applicationBtxCeptStripComments(dataInPointer);		/* strip comments */
	applicationBtxCeptStripNonPrintable(dataInPointer);	/* strip non printable chars (and whitespaces) */

	while(*dataInPointer != '\0')
	{
		/* Handle parser control */
		if((applicationBtxCeptTriggerTag(dataInPointer,"<cept>\0") == 0)) 
		{
			if(tagHistory == 0)
			{
				tagHistory = 1;
				parserActive = 1;
			}
			else
				return -1;
		}
		if((applicationBtxCeptTriggerTag(dataInPointer,"<head>\0") == 0)) 
		{
			if(tagHistory == 1)
			{
				tagHistory = 2;
				parserMode = HEAD_PARSER_MODE;
			}
			else
				return -1;
		}
		if((applicationBtxCeptTriggerTag(dataInPointer,"</head>\0") == 0)) 
		{
			if(tagHistory == 2)
			{
				tagHistory = 3;
				parserMode = 0;
			}
			else
				return -1;
		}
		if((applicationBtxCeptTriggerTag(dataInPointer,"<body>\0") == 0)) 
		{
			if(tagHistory == 3)
			{
				tagHistory = 4;
				parserMode = HYPERTEXT_PARSER_MODE;
				dataInPointer += 6;	/* Override the body tag */
			}
			else
				return -1;
		}
		if((applicationBtxCeptTriggerTag(dataInPointer,"</body>\0") == 0)) 
		{
			if(tagHistory == 4)
			{
				tagHistory = 5;
				parserMode = 0;
			}
			else
				return -1;
		}
		if((applicationBtxCeptTriggerTag(dataInPointer,"</cept>\0") == 0)) 
		{
			if(tagHistory == 5)
			{
				tagHistory = 6;
				parserActive = 0;
			}
			else
				return -1;
		}


		/* Head parser mode */
		if((parserMode == HEAD_PARSER_MODE)&&(parserActive == 1))
		{
			if((applicationBtxCeptTriggerTag(dataInPointer,"<meta\0") == 0)) 
			{
				if(page->metaTagCount<BTX_CEPT_META_TAG_MAXCOUNT)
				{
					if(applicationBtxParseMetaTag(dataInPointer,page->metaName[page->metaTagCount],page->metaContent[page->metaTagCount]) == -1)
						return -1;
					page->metaTagCount++;
				}
				else
					return -1;
			}

			dataInPointer++;
		}
		/* Hypertect parser mode */
		else if((parserMode == HYPERTEXT_PARSER_MODE)&&(parserActive == 1))
		{
			if(page->ceptPageLength < BTX_CEPT_BINARY_BUFFERSIZE)
			{
				applicationBtxCeptTranslateTag(dataInPointer,&binaryCode,&tagSize);	/* Translate hypertext to binary */
				dataInPointer += tagSize;
				*dataOutPointer = binaryCode;
				dataOutPointer++;
				page->ceptPageLength++;
			}
			else
				return 0;
		}
		/* Parser off */
		else
			dataInPointer++;
	}

	*dataOutPointer = '\0';

	/* Do final check, every flack must be set! */
	if((tagHistory == 6)&&(parserActive == 0)&&(parserMode == 0))
		return 0;
	else 
		return -1;

}
/* #################################################################################### */


/* ## CEPT TRANSMISSION HANDLER ####################################################### */
/* Transmits a byte and asures that the byte is sent befor return */
static void uartTransmitByteBlocking(unsigned char byte)
{
	UART_Send_Byte(byte);						/* Transmit byte */
	while(UART_Get_Bytes_in_Tx_Buffer() != 0);			/* Hold on until the transmission is complete */
	return;
}


/* Transmit cept page to the terminal */
int applicationBtxCeptTransmit(btxCeptPageData *page)
{
	char *dataPointer = page->ceptPage;
	int i;

#if (DEBUGPORT == 1)
	applicationBtxTermctrlPortSelect(TERMCTRL_BTX_TERMINAL);	/* Select BTX-Terminal for input/output */
#endif /*DEBUGPORT*/

	applicationBtxTermctrlInhibit(TERMCTRL_INHIBIT_ON);		/* Ignore all incoming data from terminal */

	for(i=0;i<page->ceptPageLength;i++)
	{
		UART_Send_Byte((unsigned char) *dataPointer);
		dataPointer++;
	}

	applicationBtxTermctrlInhibit(TERMCTRL_INHIBIT_OFF);		/* Turn off ignoring the terminal data */

#if (DEBUGPORT == 1)
	applicationBtxTermctrlPortSelect(TERMCTRL_V24_TERMINAL);	/* Select V24-Terminal for input/output */
#endif /*DEBUGPORT*/

	return 0;
}


/* Get keyboard a page request from keyboard (is just a more sphisticated input function with automated hyperlink/pagerequest detection */
int applicationBtxCeptGetPageRequest(char *string)
{
	unsigned char inputBuffer;
	char *stringPointer = string;
	int lengthCount = 0;
	int dynamicMaxlength = BTX_ULM_PAGE_ID_MAXLENGTH;		/* Here the maximum length must be dynamic */

#if (DEBUGPORT == 1)
	applicationBtxTermctrlPortSelect(TERMCTRL_BTX_TERMINAL);	/* Select BTX-Terminal for input/output */
#endif /*DEBUGPORT*/

	uartTransmitByteBlocking(CEPT_CON);				/* Cursor on */

	while((inputBuffer != '#')&&(lengthCount < dynamicMaxlength))
	{
		if(string[0] == '*')					/* Here we switch the maxlength */
			dynamicMaxlength = BTX_ULM_PAGE_ID_MAXLENGTH;
		else
			dynamicMaxlength = BTX_ULM_HYPERLINK_ID_MAXLENGTH;

		inputBuffer = UART_Get_Byte();
		CLOCK_delay(CEPT_ECHOPLEX_GUARDTIME);

		if(inputBuffer == CEPT_INI)				/* Replace Initiator with a '*' */
			inputBuffer = '*';
		else if(inputBuffer == CEPT_TER)			/* Replace Terminator with a '#' */
			inputBuffer = '#';

		if((inputBuffer >= 0x20)&&(inputBuffer <= 0x7E))			/* Echoplex and store the character when it is printable */
		{
			uartTransmitByteBlocking(inputBuffer);				/* Echoplex */
			*stringPointer = (char)inputBuffer;

			if((*stringPointer == '*')&&(*(stringPointer -1) == '*'))	/* Leave when "**" (corrector sign) was entered */
				inputBuffer = '#';					/* Note: This is a cludge! */

			stringPointer++;
			lengthCount++;
		}
		else if((inputBuffer == CEPT_APB)&&(lengthCount != 0))			/* Back key was pressed, user wants to correct previous input */
		{
			uartTransmitByteBlocking(CEPT_APB);				/* Clear character from screen by overwriting it with a whitespace */
			uartTransmitByteBlocking(' ');
			uartTransmitByteBlocking(CEPT_APB);				/* Clear character from screen by overwriting it with a whitespace */
			stringPointer--;
			lengthCount--;
		}
	}

	*stringPointer = '\0';

	uartTransmitByteBlocking(CEPT_COF);				/* Cursor off */

#if (DEBUGPORT == 1)
	applicationBtxTermctrlPortSelect(TERMCTRL_V24_TERMINAL);	/* Select V24-Terminal for input/output */
#endif /*DEBUGPORT*/

	return lengthCount;
}

/* Get keyboard input from the terminal (With echoplexing and the possibility of correction) */
int applicationBtxCeptGetString(char *string, int maxlength)
{
	unsigned char inputBuffer;
	char *stringPointer = string;
	int lengthCount = 0;

#if (DEBUGPORT == 1)
	applicationBtxTermctrlPortSelect(TERMCTRL_BTX_TERMINAL);	/* Select BTX-Terminal for input/output */
#endif /*DEBUGPORT*/

	uartTransmitByteBlocking(CEPT_CON);				/* Cursor on */

	while((inputBuffer != '#')&&(lengthCount < maxlength))
	{
		inputBuffer = UART_Get_Byte();
		CLOCK_delay(CEPT_ECHOPLEX_GUARDTIME);

		if(inputBuffer == CEPT_INI)				/* Replace Initiator with a '*' */
			inputBuffer = '*';
		else if(inputBuffer == CEPT_TER)			/* Replace Terminator with a '#' */
			inputBuffer = '#';

		if((inputBuffer >= 0x20)&&(inputBuffer <= 0x7E))	/* Echoplex and store the character when it is printable */
		{
			uartTransmitByteBlocking(inputBuffer);		/* Echoplex */
			*stringPointer = (char)inputBuffer;
			stringPointer++;
			lengthCount++;
		}
		else if((inputBuffer == CEPT_APB)&&(lengthCount != 0))	/* Back key was pressed, user wants to correct previous input */
		{
			uartTransmitByteBlocking(CEPT_APB);		/* Clear character from screen by overwriting it with a whitespace */
			uartTransmitByteBlocking(' ');
			uartTransmitByteBlocking(CEPT_APB);		/* Clear character from screen by overwriting it with a whitespace */
			stringPointer--;
			lengthCount--;
		}
	}

	*stringPointer = '\0';

	uartTransmitByteBlocking(CEPT_COF);				/* Cursor off */

#if (DEBUGPORT == 1)
	applicationBtxTermctrlPortSelect(TERMCTRL_V24_TERMINAL);	/* Select V24-Terminal for input/output */
#endif /*DEBUGPORT*/

	return lengthCount;
}

/* Rub out content from screen (e.g an invalid entered page id) */
void applicationBtxCeptRubOut(int length)
{
	int i;

#if (DEBUGPORT == 1)
	applicationBtxTermctrlPortSelect(TERMCTRL_BTX_TERMINAL);	/* Select BTX-Terminal for input/output */
#endif /*DEBUGPORT*/

	applicationBtxTermctrlInhibit(TERMCTRL_INHIBIT_ON);

	for(i=0; i<length; i++)						/* The first we do is going back to the beginning of the input that has to be rubbed out */
		uartTransmitByteBlocking(CEPT_APB);

	for(i=0; i<length; i++)						/* Now we overwrite it with whitspaces */
		uartTransmitByteBlocking(' ');

	for(i=0; i<length; i++)						/* At last we go back to the start of the input. Now a new input can be performed! */
		uartTransmitByteBlocking(CEPT_APB);

	applicationBtxTermctrlInhibit(TERMCTRL_INHIBIT_OFF);

#if (DEBUGPORT == 1)
	applicationBtxTermctrlPortSelect(TERMCTRL_V24_TERMINAL);	/* Select V24-Terminal for input/output */
#endif /*DEBUGPORT*/
}


/* Automaticly find the content data for a given meta name and offset */
int applicationBtxCeptGetMetaTagWithOffset(btxCeptPageData *page, char *name, char *content, int offset)
{
	int i;
	int offsetCount = 0;

	for(i=0; i<page->metaTagCount; i++)
	{
		if(strcmp(name,page->metaName[i]) == 0)
		{
			/* Exit if we got the tag with the right offset */
			if(offsetCount == offset)
			{
				strcpy(content, page->metaContent[i]);
				return 0;
			}
			else
				offsetCount++;
		}
	}

	return -1;
}

/* Automaticly find the content data for a given meta name */
int applicationBtxCeptGetMetaTag(btxCeptPageData *page, char *name, char *content)
{
	return applicationBtxCeptGetMetaTagWithOffset(page,name,content,0);
}

/* Resolve a btx hyperlink */
int applicationBtxResolveHyperlink(btxCeptPageData *page, char *number, char *btxPageId)
{
	char *btxPageIdPointer;
	char list[BTX_CEPT_META_TAG_HYPERLINKLIST_BUFFER];
	char *listPointer;
	int offset = 0;

	while(1)
	{
		btxPageIdPointer = btxPageId;
		listPointer = list;

		if(applicationBtxCeptGetMetaTagWithOffset(page,"hyperlinks",list,offset) == 0)
		{
			while(*listPointer != '\0')
			{
				/* Process item */
				if((number[0]==listPointer[0])&&(number[1]==listPointer[1]))
				{
					listPointer++;
					listPointer++;

					while((*listPointer != ',')&&(*listPointer != '\0'))
					{
						*btxPageIdPointer = *listPointer;
						btxPageIdPointer++;
						listPointer++;
					}
					*btxPageIdPointer = '\0';
					return 0;			/* We have found what we want, so we return right here */
				}

				/* Jump to the next item in the list */
				do					/* Go to the next delimiter */
				{
					listPointer++;
				} while((*listPointer != ',')&&(*listPointer != '\0'));

				if(*listPointer == ',')			/* Skip delimiters between the list items */
					listPointer++;
			}
		}
		else
			return -1;	/* No, or no further hyperlink sections were detected */

		/* Fetch the next hyperlinks tag in the next run */
		offset++;
	}
}
/* #################################################################################### */
