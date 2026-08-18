/*
   ####################################################################################
   #                                                                                  #
   #                        Bildschirmtricks MikroPAD V2.0.0                          #
   #                                cept parser layer                                 #
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
#ifndef CEPT_H
#define CEPT_H

/* ==Control codes==
 The the CEPT protocol includes a wide range of special control codes, here is a 
 hopefully complete list of all available control codes. A detailed explaination
 of nearly all control codes can be found in ETS_300_072 (downloadable from etsi.org) */

/* C0 Control characters (printf-friendly) */
#define F_CEPT_APB "\x08"			/* Active Position Back */
#define F_CEPT_APF "\x09"			/* Active Position Forward */
#define F_CEPT_APD "\x0A"			/* Active Position Down */
#define F_CEPT_APU "\x0B"			/* Active Position Up */
#define F_CEPT_CS  "\x0C"			/* Clear Screen */
#define F_CEPT_APR "\x0D"			/* Active Position Return */
#define F_CEPT_SI "\x0E"			/* Shift In */
#define F_CEPT_SO "\x0F"			/* Shift Out */
#define F_CEPT_CON "\x11"			/* Cursor ON */
#define F_CEPT_RPT "\x12"			/* Repeat Character */
#define F_CEPT_COF "\x14"			/* Cursor OFf */
#define F_CEPT_CAN "\x18"			/* CANcel (fills the rest off the line with spaces) */
#define F_CEPT_SS2 "\x19"			/* Single Shift 2 (G2 set, Some legents say that this is the magic "YES" character) */
#define F_CEPT_ESC "\x1B"			/* ESCape */
#define F_CEPT_SS3 "\x1D"			/* Single Shift 3 (G3 set) */
#define F_CEPT_APH "\x1E"			/* Active Position Home */
#define F_CEPT_US  "\x1F"			/* Unit Seperator (also known as APA) */

/* C0 Control characters (single byte constants) */
#define CEPT_APB 0x08				/* Active Position Back */
#define CEPT_APF 0x09				/* Active Position Forward */
#define CEPT_APD 0x0A				/* Active Position Down */
#define CEPT_APU 0x0B				/* Active Position Up */
#define CEPT_CS  0x0C				/* Clear Screen */
#define CEPT_APR 0x0D				/* Active Position Return */
#define CEPT_SI  0x0E 				/* Shift In */
#define CEPT_SO  0x0F 				/* Shift Out */
#define CEPT_CON 0x11				/* Cursor ON */
#define CEPT_RPT 0x12				/* Repeat Last Character */
#define CEPT_COF 0x14				/* Cursor OFf */
#define CEPT_CAN 0x18				/* CANcel (fills the rest off the line with spaces) */
#define CEPT_SS2 0x19				/* Single Shift 2 (G2 set, Some legents say that this is the magic "YES" character) */
#define CEPT_ESC 0x1B				/* ESCape */
#define CEPT_SS3 0x1D				/* Single Shift 3 (G3 set) */
#define CEPT_APH 0x1E				/* Active Position Home */
#define CEPT_US  0x1F				/* Unit Seperator (also known as APA) */


/* C0 Data link control tokens (printf-friendly) */
#define F_CEPT_NUL "\x00"			/* NULl */
#define F_CEPT_SOH "\x01"			/* Start Of Heading */
#define F_CEPT_STX "\x02"			/* Start TeXt */
#define F_CEPT_ETX "\x03"			/* End TeXt */
#define F_CEPT_EOT "\x04"			/* End Of Transmission */
#define F_CEPT_ENQ "\x05"			/* ENQuiry */
#define F_CEPT_ACK "\x06"			/* ACKnowledge */
#define F_CEPT_ITB "\x07"			/* end InTermediate Block */
#define F_CEPT_DLE "\x10"			/* Data Link Escape */
#define F_CEPT_NAK "\x15"			/* Negative AcKnowledge */
#define F_CEPT_SYN "\x16"			/* SYNcronize */
#define F_CEPT_ETB "\x17"			/* End TeXtblock */

/* C0 Data link control tokens (single byte constants) */
#define CEPT_NUL 0x00				/* NULl */
#define CEPT_SOH 0x01				/* Start Of Heading */
#define CEPT_STX 0x02				/* Start TeXt */
#define CEPT_ETX 0x03				/* End TeXt */
#define CEPT_EOT 0x04				/* End Of Transmission */
#define CEPT_ENQ 0x05				/* ENQuiry */
#define CEPT_ACK 0x06				/* ACKnowledge */
#define CEPT_ITB 0x07				/* end InTermediate Block */
#define CEPT_DLE 0x10				/* Data Link Escape */
#define CEPT_NAK 0x15				/* Negative AcKnowledge */
#define CEPT_SYN 0x16				/* SYNcronize */
#define CEPT_ETB 0x17				/* End TeXtblock */

/* NOTE: The Data link control tokens are not mentiond in ETS_300_072 */


/* C0 Propritay-BTX tokens (printf-friendly) */
#define F_CEPT_INI "\x13"			/* Initiator (*) */
#define F_CEPT_TER "\x1C"			/* Terminator (#) */
#define F_CEPT_DCT "\x1A"			/* propritary btx (makes the terminal talking) */

/* C0 Propritay-BTX tokens (single byte constants) */
#define CEPT_INI 0x13				/* Initiator (*) */
#define CEPT_TER 0x1C				/* Terminator (#) */
#define CEPT_DCT 0x1A				/* propritary btx (makes the terminal talking) */





/* C1S/C1P control functions set (printf-friendly) */
#define F_CEPT_FSH "\x88"			/* FlaSH */
#define F_CEPT_STD "\x89"			/* STeaDy */
#define F_CEPT_EBX "\x8A"			/* End BoX */
#define F_CEPT_SBX "\x8B"			/* Start BoX */
#define F_CEPT_NSZ "\x8C"			/* Normal-SiZe */
#define F_CEPT_DBH "\x8D"			/* DouBle-Height */
#define F_CEPT_DBW "\x8E"			/* DouBle-Width */
#define F_CEPT_DBS "\x8F"			/* DouBle-Size */
#define F_CEPT_CDY "\x98"			/* Conceal DisplaY */
#define F_CEPT_SPL "\x99"			/* StoP Lining */
#define F_CEPT_STL "\x9A"			/* StarT Lining */
#define F_CEPT_CSI "\x9B"			/* Control Sequence Introducer */

/* C1S/C1P control functions set (single byte constants) */
#define CEPT_FSH 0x88				/* FlaSH */
#define CEPT_STD 0x89				/* STeaDy */
#define CEPT_EBX 0x8A				/* End BoX */
#define CEPT_SBX 0x8B				/* Start BoX */
#define CEPT_NSZ 0x8C				/* Normal-SiZe */
#define CEPT_DBH 0x8D				/* DouBle-Height */
#define CEPT_DBW 0x8E				/* DouBle-Width */
#define CEPT_DBS 0x8F				/* DouBle-Size */
#define CEPT_CDY 0x98				/* Conceal DisplaY */
#define CEPT_SPL 0x99				/* StoP Lining */
#define CEPT_STL 0x9A				/* StarT Lining */
#define CEPT_CSI 0x9B				/* Control Sequence Introducer */


/* C1S control functions set (printf-friendly) */
#define F_CEPT_ABK "\x80"			/* Alpha BlacK */
#define F_CEPT_ANR "\x81"			/* Alpha Red */
#define F_CEPT_ANG "\x82"			/* Alpha Green */
#define F_CEPT_ANY "\x83"			/* Alpha Yellow */
#define F_CEPT_ANB "\x84"			/* Alpha Blue */
#define F_CEPT_ANM "\x85"			/* Alpha Mageta */
#define F_CEPT_ANC "\x86"			/* Alpha Cyan */
#define F_CEPT_ANW "\x87"			/* Alpha White */
#define F_CEPT_MBK "\x90"			/* Mosaic BlacK */
#define F_CEPT_MSR "\x91"			/* MoSaic Red */
#define F_CEPT_MSG "\x92"			/* MoSaic Green */
#define F_CEPT_MSY "\x93"			/* MoSaic Yellow */
#define F_CEPT_MSB "\x94"			/* MoSaic Blue */
#define F_CEPT_MSM "\x95"			/* MoSaic Magenta */
#define F_CEPT_MSC "\x96"			/* MoSaic Cyan */
#define F_CEPT_MSW "\x97"			/* MoSaic White */
#define F_CEPT_BBD "\x9c"			/* Black BackgrounD */
#define F_CEPT_NBD "\x9d"			/* New BackgrounD */
#define F_CEPT_HMS "\x9e"			/* Hold MoSaic */
#define F_CEPT_RMS "\x9f"			/* Release MoSaic */

/* C1S control functions set (single byte constants) */
#define CEPT_ABK 0x80				/* Alpha BlacK */
#define CEPT_ANR 0x81				/* Alpha Red */
#define CEPT_ANG 0x82				/* Alpha Green */
#define CEPT_ANY 0x83				/* Alpha Yellow */
#define CEPT_ANB 0x84				/* Alpha Blue */
#define CEPT_ANM 0x85				/* Alpha Mageta */
#define CEPT_ANC 0x86				/* Alpha Cyan */
#define CEPT_ANW 0x87				/* Alpha White */
#define CEPT_MBK 0x90				/* Mosaic BlacK */
#define CEPT_MSR 0x91				/* MoSaic Red */
#define CEPT_MSG 0x92				/* MoSaic Green */
#define CEPT_MSY 0x93				/* MoSaic Yellow */
#define CEPT_MSB 0x94				/* MoSaic Blue */
#define CEPT_MSM 0x95				/* MoSaic Magenta */
#define CEPT_MSC 0x96				/* MoSaic Cyan */
#define CEPT_MSW 0x97				/* MoSaic White */
#define CEPT_BBD 0x9c				/* Black BackgrounD */
#define CEPT_NBD 0x9d				/* New BackgrounD */
#define CEPT_HMS 0x9e				/* Hold MoSaic */
#define CEPT_RMS 0x9f				/* Release MoSaic */


/* C1P control functions set (printf-friendly) */
#define F_CEPT_BKF "\x80"			/* BlacK Foreground */
#define F_CEPT_RDF "\x81"			/* ReD Foreground */
#define F_CEPT_GRF "\x82"			/* GReen Foreground */
#define F_CEPT_YLF "\x83"			/* YeLlow Foreground */
#define F_CEPT_BLF "\x84"			/* BLue Foreground */
#define F_CEPT_MGF "\x85"			/* MaGenta Foreground */
#define F_CEPT_CNF "\x86"			/* CyaN Foreground */
#define F_CEPT_WHF "\x87"			/* WHite Foreground */
#define F_CEPT_BKB "\x90"			/* BlacK Background */
#define F_CEPT_RDB "\x91"			/* ReD Background */
#define F_CEPT_GRB "\x92"			/* GReen Background */
#define F_CEPT_YLB "\x93"			/* YeLlow Background */
#define F_CEPT_BLB "\x94"			/* BLue Background */
#define F_CEPT_MGB "\x95"			/* MaGenta Background */
#define F_CEPT_CNB "\x96"			/* CyaN Background */
#define F_CEPT_WHB "\x97"			/* WHite Background */
#define F_CEPT_NPO "\x9c"			/* Normal POlarity */
#define F_CEPT_IPO "\x9d"			/* Inverted POlarity */
#define F_CEPT_TRB "\x9e"			/* TranspaRent Background */
#define F_CEPT_STC "\x9f"			/* STop Conceal */

/* C1P control functions set (single byte constants) */
#define CEPT_BKF 0x80				/* BlacK Foreground */
#define CEPT_RDF 0x81				/* ReD Foreground */
#define CEPT_GRF 0x82				/* GReen Foreground */
#define CEPT_YLF 0x83				/* YeLlow Foreground */
#define CEPT_BLF 0x84				/* BLue Foreground */
#define CEPT_MGF 0x85				/* MaGenta Foreground */
#define CEPT_CNF 0x86				/* CyaN Foreground */
#define CEPT_WHF 0x87				/* WHite Foreground */
#define CEPT_BKB 0x90				/* BlacK Background */
#define CEPT_RDB 0x91				/* ReD Background */
#define CEPT_GRB 0x92				/* GReen Background */
#define CEPT_YLB 0x93				/* YeLlow Background */
#define CEPT_BLB 0x94				/* BLue Background */
#define CEPT_MGB 0x95				/* MaGenta Background */
#define CEPT_CNB 0x96				/* CyaN Background */
#define CEPT_WHB 0x97				/* WHite Background */
#define CEPT_NPO 0x9c				/* Normal POlarity */
#define CEPT_IPO 0x9d				/* Inverted POlarity */
#define CEPT_TRB 0x9e				/* TranspaRent Background */
#define CEPT_STC 0x9f				/* STop Conceal */


/* FE Full Screen atributes (single byte constants) */
#define CEPT_FBKB 0x50				/* BlacK Background */
#define CEPT_FRDB 0x51				/* ReD Background */
#define CEPT_FGRB 0x52				/* GReen Background */
#define CEPT_FYLB 0x53				/* YeLlow Background */
#define CEPT_FBLB 0x54				/* BLue Background */
#define CEPT_FMGB 0x55				/* MaGenta Background */
#define CEPT_FCNB 0x56				/* CyaN Background */
#define CEPT_FWHB 0x57				/* WHite Background */
#define CEPT_FTRB 0x5E				/* TRansparent Background */

/* FE Full Screen atributes (printf-friendly) */
#define F_CEPT_FBKB "\x50"			/* BlacK Background */
#define F_CEPT_FRDB "\x51"			/* ReD Background */
#define F_CEPT_FGRB "\x52"			/* GReen Background */
#define F_CEPT_FYLB "\x53"			/* YeLlow Background */
#define F_CEPT_FBLB "\x54"			/* BLue Background */
#define F_CEPT_FMGB "\x55"			/* MaGenta Background */
#define F_CEPT_FCNB "\x56"			/* CyaN Background */
#define F_CEPT_FWHB "\x57"			/* WHite Background */
#define F_CEPT_FTRB "\x5E"			/* TRansparent Background */

#define CEPT_ECHOPLEX_GUARDTIME 200		/* Echoplex guardtime, time between receiving and echoing */

/* Data structure of a btx-cept-page */
typedef struct
{
	/* Contains the cept page in its binary form */
	char ceptPage[BTX_CEPT_BINARY_BUFFERSIZE];
	/* Contains the length of the cept page */
	int ceptPageLength;
	/* Contains the meta tag name */
	char metaName[BTX_CEPT_META_TAG_MAXCOUNT][BTX_CEPT_META_TAG_NAME_BUFFERSIZE];
	/* Contains the corospondenting meta content */
	char metaContent[BTX_CEPT_META_TAG_MAXCOUNT][BTX_CEPT_META_TAG_CONTENT_BUFFERSIZE];
	/* Contains the number of meta tags that are issued to the page */
	int metaTagCount;

}btxCeptPageData;

/* Parse the raw data that was received via http to a btx-cept-page */
int applicationBtxCeptParse(char *dataIn, btxCeptPageData *page);

#endif /*CEPT_H*/
/* #################################################################################### */
 
