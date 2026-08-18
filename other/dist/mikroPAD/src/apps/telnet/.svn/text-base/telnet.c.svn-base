/*! \file telnet.c \brief Ein sehr einfacher Telnetclient */
//***************************************************************************
//*            telnet.c
//*
//*  Sat Jun  3 23:01:42 2006
//*  Copyright  2006 Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup software
///	\defgroup telnet Ein sehr einfacher Telnetclient (telnet.c)
///	\code #include "telnet.h" \endcode
///	\par Uebersicht
/// 	Ein sehr einfacher Telnet-client der auf den Cointroller läuft. Ermöglich
/// Das abfragen vom Status des Controller und dieverse andere dinge.
/// \todo	Bei einer Verbindung mit Windows geht der Client noch nicht, da Windows die Daten für jeden Tastendruck
/// einzeln schickt. Muss noch gefixt werden. Möglicher fix ist, die Zeichen einzeln zu holen und selber noch mal zwischen
/// zu speichern, bis eine Eingabe mit 0x0a (ASCII 10) oder 0x0d (ASCII 13) abgeschlossen wird. Kann aber erst passieren, wenn TCP auf FIFO umgestellt
/// ist, da sonst die Performace leidet.
/// \date	04-18-2008: Der Blödsinn mit Windows ist beseitigt, geht jetzt. Es wird jetzt pro durchlauf versucht den puffer auszulesen und
///			erst wenn eine Eingabe mit 0x0a,0x0d abgeschlossen wird gehts ab an den Verarbeitung des Strings. Was für ein Akt :-) und alles wegen
///			Windows.
//****************************************************************************/
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
//@{

#include <avr/pgmspace.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <avr/version.h>

#include "system/clock/clock.h"

#include "hardware/network/enc28j60.h"
#include "hardware/memory/mem-check.h"
#include "hardware/uart/uart.h"

#include "system/net/tcp.h"
#include "system/net/ip.h"
#include "system/net/ethernet.h"
#include "system/net/arp.h"
#include "system/net/dns.h"
#include "system/stdout/stdout.h"
#include "system/config/config.h"
#include "system/softreset/softreset.h"

#include "telnet.h"
#include "apps/mp3-streamingclient/mp3-clientserver.h"

unsigned int TELNET_SOCKET = NO_SOCKET_USED;

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Initialisiert den Telnet-clinet und registriert den Port auf welchen dieser lauschen soll.
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void telnet_init()
	{
		RegisterTCPPort( TELNET_PORT );
		printf_P( PSTR("Telnet-Server gestartet auf Port %d.\r\n") , TELNET_PORT );
	}

/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Der Telnet-client an sich. Er wird zyklisch aufgerufen und schaut nach ob eine Verbindung auf den
 * registrierten Port eingegangen ist. Wenn ja holt er sich die Socketnummer der Verbindung und speichert diese.
 * Wenn eine Verbindung zustande gekommen ist wird diese wiederrum zyklisch nach neuen Daten abgefragt und entsprechend
 * reagiert.
 * \param 	NONE
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/	
void telnet_thread()
	{
		static unsigned char TELNET_BUFFER [ TELNET_BUFFER_LEN ];
		static unsigned int TELNET_POS;
		static unsigned int TELNET_STATE;

		unsigned long IP;
		unsigned char MACbuffer[6];

		unsigned char Data,ttl,timer,i;
		
		union IP_ADDRESS DNSIP;

		// keine alte Verbindung offen?
		if ( TELNET_SOCKET == NO_SOCKET_USED )
		{ 	
			// auf neue Verbindung testen
			TELNET_SOCKET = CheckPortRequest( TELNET_PORT );
			if ( TELNET_SOCKET != NO_SOCKET_USED )
			{	
				// Wenn ja, Startmeldung ausgeben und startzustand herstellen für telnet
				static const char MSG[] PROGMEM = {
					"Welcome on Tiny-Telnetserver 0.1a!\r\n"
					"Build on AVR-libc version: " __AVR_LIBC_VERSION_STRING__ "/" __AVR_LIBC_DATE_STRING__ "\r\n"
					"Und, Weiter Weiter!!!\r\n> " }	;
				PutSocketData_P ( TELNET_SOCKET, strlen( MSG ), MSG );

				// TELNET_BUFFER leeren und auf Ausgangszustand setzen
				TELNET_STATE = 0;
				TELNET_POS = 0;
				TELNET_BUFFER[0] = '\0';
				TELNET_STATE = 0;
				
				FlushSocketData( TELNET_SOCKET );
			}
		}
		// Wenn alte Verbindung offen hier weiter
		else
		{
			// checken ob noch offen ist
			if( CheckSocketState( TELNET_SOCKET ) == SOCKET_NOT_USE )
			{
				CloseTCPSocket( TELNET_SOCKET );
				TELNET_SOCKET = NO_SOCKET_USED;
				return;
			}

			// Auf neue Daten zum zusammenbauen testen
			// hier wird der TELNET_BUFFER aufgefüllt bis 0x0a oder 0x0d eintreffen. der Puffer ist statisch
			// Wenn ein 0x0a oder 0x0d empfangen wurde, wird der TELNET_STATE auf 1 gesetzt, damit er verarbeitet werden kann
			if ( TELNET_STATE == 0 )
			{	
				STDOUT_Set_TCP_Socket ( TELNET_SOCKET );
				
				while( GetBytesInSocketData( TELNET_SOCKET ) >= 1 )
				{
					Data = ( GetByteFromSocketData ( TELNET_SOCKET ) );
					if ( Data != 0x0a )
					{
						if ( TELNET_POS < TELNET_BUFFER_LEN )
						{
							TELNET_BUFFER[ TELNET_POS++ ] = Data;
							TELNET_BUFFER[ TELNET_POS ] = '\0';
						}
						if ( Data == 0x0d )
						{
							TELNET_STATE = 1;
							break;
						}
					}
				}
			}	

			// Wenn TELNET_BUFFER eine Zeile vollständig hat gehts hier weiter
			if ( TELNET_STATE == 1 )
			{
				// auf STATS checken
				if ( !memcmp( &TELNET_BUFFER[0] , "stats" , 5 ) ) 
				{
					unsigned int REVID,_PHSTAT1,_PHSTAT2;

					// buffer mit ausgabe vorbereiten & ausgeben
					printf_P( PSTR("Ethernet: %ld Bytes in %ld Packeten\r\n") , ByteCounter, PacketCounter );
					printf_P( PSTR("TCP-TX Errors: %d TCP-RX Errors: %d (unsorted %d, oldseq %d)\r\n\r\n"), TXErrorCounter , RXErrorCounter, RXErrorUnsort, RXErrorOldSeq );

					LockEthernet();
					
					REVID = enc28j60Read( EREVID );
					_PHSTAT1 = enc28j60PhyRead( PHSTAT1 );
					_PHSTAT2 = enc28j60PhyRead( PHSTAT2 );
					
					FreeEthernet();
					
					printf_P( PSTR("ENC28J60:\r\n"));
					printf_P( PSTR("Reg.    | Wert\r\n"));
					printf_P( PSTR("--------+--------\r\n"));
					printf_P( PSTR("Rev.ID  |   0x%02X\r\n") , REVID );
					printf_P( PSTR("PHSTAT1 | 0x%04X\r\n") , _PHSTAT1 );
					printf_P( PSTR("PHSTAT2 | 0x%04X\r\n") , _PHSTAT2 );

				}
				// auf QUIT checken
				else if ( !memcmp( &TELNET_BUFFER[0] , "quit" , 4 ) ) 
				{
					// Socket schließen
					printf_P( PSTR("Verbindung wird geschlossen\r\n") );
					STDOUT_Flush();
					STDOUT_Set_RS232 ();
					CloseTCPSocket( TELNET_SOCKET );
					TELNET_SOCKET = NO_SOCKET_USED;
					return;
				}

				// auf TIME checken
				else if ( !memcmp( &TELNET_BUFFER[0] , "time" , 4 ) )
				{
					if ( TELNET_BUFFER[4] != 0x0d )
					{
						// Zeit einlesen & setzen wenn parameter angebenen wurden
						if ( atoi( &TELNET_BUFFER[5] ) >= 0 || atoi( &TELNET_BUFFER[5] ) <= 23 ) {
							hh = atoi( &TELNET_BUFFER[5] );
							mm = atoi( &TELNET_BUFFER[8] );
							ss = atoi( &TELNET_BUFFER[11] );
						}
					}
					// ausgaben vorbereiten und senden
					printf_P( PSTR("Time: %02d:%02d:%02d\r\n") , hh , mm , ss);
				}

				// der arp Befehl 
				else if ( !memcmp( &TELNET_BUFFER[0] , "arp" , 3 ) )
				{
					if ( memcmp( &TELNET_BUFFER[4] , "-n" , 2 ) )
					{
						// ausgaben vorbereiten & senden
						if ( GetIP2MAC ( 0x0202a8c0, MACbuffer ) == ARP_ANSWER )
						{
							printf_P( PSTR("MAC Adresse %02X:%02X:%02X%:%02X:%02X:%02X\r\n"), MACbuffer[0], MACbuffer[1], MACbuffer[2], MACbuffer[3], MACbuffer[4], MACbuffer[5]);
						}
					}
					else
					{						
						printf_P( PSTR("IP            MAC                 TTL\r\n") );
						
						for ( i = 0; i < MAX_ARPTABLE_ENTRYS ; i++ )
						{
							if( GetARPtableEntry ( i, &DNSIP.IP, &MACbuffer, &ttl ) == 1 )
							{
								if ( ttl != 0 )
								{
									printf_P( PSTR("%d.%d.%d.%d    %02X:%02X:%02X:%02X:%02X:%02X   %d\r\n"),DNSIP.IPbyte[0],DNSIP.IPbyte[1],DNSIP.IPbyte[2],DNSIP.IPbyte[3],MACbuffer[0],MACbuffer[1],MACbuffer[2],MACbuffer[3],MACbuffer[4],MACbuffer[5], ttl);
								}
							}
						}
					}
				}
				
				// auf MEM checken
				else if ( !memcmp( &TELNET_BUFFER[0] , "ntp" , 3 ) )
				{
					for ( i = 4 ; i < TELNET_BUFFER_LEN ; i++ )
						if ( TELNET_BUFFER[i] == '\r' )
						{
							TELNET_BUFFER[i] = '\0';
							break;
						}
					// sprintf_P( buffer , PSTR("www.berlin.de") );
					DNSIP.IP = DNS_ResolveName( &TELNET_BUFFER[4] );
					// ausgaben vorbereiten & senden
					printf_P( PSTR("Socket: %d\r\n") , NTP_GetTime( DNSIP.IP, 0 ) );
					printf_P( PSTR("Time: %02d:%02d:%02d\r\n") , hh , mm , ss);
				}
				
				else if ( !memcmp( &TELNET_BUFFER[0] , "reset" , 5 ) )
				{
					// Socket schließen
					printf_P( PSTR("Verbindung wird geschlossen\r\n") );
					STDOUT_Flush();
					STDOUT_Set_RS232 ();
					CloseTCPSocket( TELNET_SOCKET );
					TELNET_SOCKET = NO_SOCKET_USED;
					softreset();
				}
				
				else if ( !memcmp( &TELNET_BUFFER[0] , "eetest" , 6 ) )
				{
					eetest();
				}

				else if ( !memcmp( &TELNET_BUFFER[0] , "stream" , 6 ) )
				{
					mp3clientcommand( &TELNET_BUFFER[7] , TELNET_SOCKET );
				}
										
				else if ( !memcmp( &TELNET_BUFFER[0] , "flood" , 4 ) )
				{
					unsigned long kbytes=0,loop;
					
					timer = CLOCK_RegisterCoundowntimer ();
					CLOCK_SetCountdownTimer ( timer, 30000, MSECOUND );
					
					kbytes = atol( &TELNET_BUFFER[6] ) * 1024;

					for ( loop = 0 ; loop < kbytes ; loop = loop + 32 )
					{
						printf_P( PSTR("12345678901234567890123456789012") );					
					}
					printf_P( PSTR("s") );
					
					printf_P( PSTR("\n%ld Bytes gesendet ") , loop );
					printf_P( PSTR("in %d.%02d Sekunden.\r\n"), ( 30000 - CLOCK_GetCountdownTimer ( timer ) ) / 100 , ( 30000 - CLOCK_GetCountdownTimer ( timer ) ) % 100 );
					
					CLOCK_ReleaseCountdownTimer ( timer );
				
					
				}
				
				// auf DNS checken
				else if ( !memcmp( &TELNET_BUFFER[0] , "dns" , 3 ) )
				{
					// ausgaben vorbereiten & senden
					for ( i = 4 ; i < TELNET_BUFFER_LEN ; i++ )
						if ( TELNET_BUFFER[i] == '\r' )
						{
							TELNET_BUFFER[i] = '\0';
							break;
						}
					// sprintf_P( buffer , PSTR("www.berlin.de") );
					DNSIP.IP = DNS_ResolveName( &TELNET_BUFFER[4] );
					if ( DNSIP.IP == DNS_NO_ANSWER ) 
					{
						printf_P( PSTR("Kein DNS-Eintrag vorhanden\r\n"));
					}
					else
					{
						printf_P( PSTR("%s = %d.%d.%d.%d\r\n"), &TELNET_BUFFER[4], DNSIP.IPbyte[0],DNSIP.IPbyte[1],DNSIP.IPbyte[2],DNSIP.IPbyte[3] );
					}
				}
					
				// auf HELP checken
				else if ( !memcmp( &TELNET_BUFFER[0] , "help" , 4 ) )
				{
					// Hilfetext senden aus dem Flash
					static const char helptxt[] PROGMEM = { 
							"\r\n Help\r\n======\r\n\r\n"
							"flood		sendet Daten mit n Kbyte und zeigt die Zeit\r\n"
							"arp		holt zu einer IP die MAC Adresse\r\n"
							"ntp		holt die aktuelle Zeit von einen NTP-Server\r\n"
							"stream		spielt einen mp3-Stream, weitere Infos unter 'stream help'\r\n"
							"dns		stellt eine DNS-Anfrage\r\n"
							"mem		zeigt den maximalen freien Speicher\r\n"
							"quit		wat wohl ?\r\n"
							"stats		Statistiken\r\n"
							"time		zeigt/setzt die Uhr (time hh:mm:ss)\r\n\r\n" } ;
					PutSocketData_RPE( TELNET_SOCKET , strlen( helptxt ) , helptxt, FLASH );
				}
				
				// Wenn befehle abgearbeitet TELNET_STATE wieder auf 0 und TELNET_BUFFER zurücksetzen, damit die nächste zeile eingelesen werden kann
				STDOUT_Flush();
				STDOUT_Set_RS232 ();

				TELNET_STATE = 0;
				TELNET_POS = 0;
				TELNET_BUFFER[0] = '\0';
				TELNET_STATE = 0;
				PutSocketData_P( TELNET_SOCKET , 2 , PSTR("> ") );
			}
			
		}
}

//@}
