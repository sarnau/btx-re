/*! \file tcp.c \brief Stellt die TCP/IP Funkionalitaet bereit */
//***************************************************************************
//*            tcp.c
//*
//*  Sat Jun  3 23:01:42 2006
//*  Copyright  2006  Dirk Broßwick
//*  Email
//****************************************************************************/
///	\ingroup network
///	\defgroup TCP Der TCP Stack fuer Mikrocontroller (tcp.c)
///	\code #include "tcp.h" \endcode
///	\par Uebersicht
///		Der TCP-Stack fuer Mikrocontroller. Behandelt komplett den TCP-Stack
/// mit Verbindungsaufbau, Abbau und halten. Es werden Ereignisse wie bei Timeouts
/// oder Retransmisions behandelt.	
/// \todo 	Umstellung auf FIFO-Buffer. Die Verarbeitung ist wesentlich schneller und effizenter denke ick mal.
/// \date	04-15-2008: Umstellung auf FIFO-Puffer erfolgt.
/// \date	05-14-2008: Man findet ja immer noch was zum basteln. Die CopyTCPdata2socketbuffer() verbessert,
///			kopieren des Buffer hat im Interrupt zu lange gedauert, so das die Uhr nach ging. Sollte eigentlich
///			nicht vorkommen, aber wenn man jedes Byte einzeln in den FIFO kopiert dauerts halt. Jetzt wird die optimierte
///			BlockToFIFO benutzt und für den Vorgang der Interrupt von Ethernet gesperrt und im
///			Interrupt alle anderen Interrupts Freigegeben damit nicht alles kurz hängen bleibt.
/// \date	05-15-2008: TCP so erweitert das jetzt auch TCP-Packete in der Flaschen Reihnfolge
///			eintreffen können, dazu wird ein Puffer (TCP_UNSORT) verwendet der das Packet zwischenspeichert
///			mit Socketnummer und Seq.nummer und beim nächsten empfang eines TCP-Packetes
///			auf gültigkeit überprüft wird ob es dem gerade empfangendem folgen sollte.
/// \date	05-25-2008: Irgend wie kommt es ab und zu zum deathlock des TCP-Stack, mal fehler finden.
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
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>

#include "system/math/math.h"
#include "system/math/checksum.h"
#include "system/clock/clock.h"
#include "system/buffer/fifo.h"
#include "hardware/led/led_core.h"

#include "ethernet.h"
#include "arp.h"
#include "ip.h"
#include "tcp.h"

// #define _DEBUG_

#ifdef _DEBUG_
	#include "hardware/uart/uart.h"
#endif

unsigned int TXErrorCounter = 0 ;
unsigned int RXErrorCounter = 0 ;
unsigned int RXErrorUnsort = 0;
unsigned int RXErrorOldSeq = 0;

struct TCP_SOCKET TCP_sockettable[MAX_TCP_CONNECTIONS];

struct TCP_PORT TCP_porttable[MAX_LISTEN_PORTS];

struct TCP_UNSORT TCP_Unsort;
	
/*------------------------------------------------------------------------------------------------------------*/
/*!\group TCP
 * \brief Hier wird der TCP Initialisiert.
 * Hier wird der TCP-Timeouthandler Registriert beim der Clock. Danach wird die Funktion alle 1000ms aufgerufen
 * und alle Offenen kontrolliert.
 * \param		NONE
 * \return		NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void tcp_init( void )
{
	unsigned int i;
	
	// CAllback für Timeout registrieren
	CLOCK_RegisterCallbackFunction( TCPTimeOutHandler, SECOUND );
	
	TCP_Unsort.lenght = 0 ;
	// FIFOs für den Recivebuffer anlegen
	for ( i = 0 ; i < MAX_TCP_CONNECTIONS ; i++ )
	{
		TCP_sockettable[ i ].fifo = Get_FIFO( TCP_sockettable[ i ].Recivebuffer, MAX_RECIVEBUFFER_LENGHT );
		TCP_sockettable[ i ].old_fifo = TCP_sockettable[ i ].fifo;
	}
}
	
/*------------------------------------------------------------------------------------------------------------*/
/*!\brief Die TCP-Funktion die aufgerufen wird wenn ein Packet eintrifft.
 * Hier findet die Bearbeitung der eintreffenden Packete statt. Das Packet wird einer Verbindung zugeordnet 
 * oder einen offenen Port wenn die in die TCP_PORT Liste eingetragen ist.
 * Danach wird es je nach Flag bearbeitet.
 * \param 		packet_lenght	Groesse des Packetes.
 * \param 		ethernetbuffer  Zeiger auf den Ethernetbuffer, dieser enthaelt noch die kompletten Header aller Layer.
 * \retval		NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void tcp( unsigned int packet_lenght, unsigned char * ethernetbuffer)
	{
		unsigned long i;
		unsigned int Socket;
		char sreg_tmp;
		
		struct ETH_header *ETH_packet; 		// ETH_struct anlegen
		ETH_packet = (struct ETH_header *) ethernetbuffer;
		struct IP_header *IP_packet;		// IP_struct anlegen
		IP_packet = ( struct IP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH];
		struct TCP_header *TCP_packet;		// TCP_struct anlegen
		TCP_packet = ( struct TCP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH + ((IP_packet->IP_Version_Headerlen & 0x0f) * 4 )];
		
		Socket = GetSocket ( ethernetbuffer );

		TCP_packet->TCP_ControllFlags &= ( TCP_SYN_FLAG | TCP_ACK_FLAG | TCP_FIN_FLAG );

		if ( Socket == 0xffff )
		{
			if ( ( CheckPortInList( ChangeEndian16bit ( TCP_packet->TCP_DestinationPort ) ) == 0xffff ) ) 
			{
				#ifdef _DEBUG_
					printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
					printf_P(PSTR("Verbindung zu unerlaubten Port\n\r"));
				#endif
				// Wieder auskommendtieren wenn ein Antwort gegeben werden soll
				Socket = RegisterSocket( ethernetbuffer );
				if ( Socket == 0xffff ) return; // Kein Socket Frei, nix machen
				MakeTCPheader( Socket, TCP_RST_FLAG , 0, 0, ethernetbuffer ); // Reset-Flag setzen, TCP bauen
				TCP_sockettable[ Socket ].ConnectionState = SOCKET_NOT_USE; // Connection State wieder auf frei setzen
				
				return;
			}
		}
		
		if ( TCP_packet->TCP_ControllFlags == TCP_SYN_FLAG ) // Packet mit SYN ?
			{
				Socket = RegisterSocket( ethernetbuffer ) ;
				if ( Socket != 0xffff ) // Verbinung einem neuem Socket zuordnen
				{
					#ifdef _DEBUG_
						printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
						printf_P( PSTR("Kommende Verbindung auf Port %d [SYN] auf Socket %d\r\n") , ChangeEndian16bit ( TCP_sockettable[ Socket ].DestinationPort ), Socket );
					#endif
					TCP_sockettable[ Socket ].ConnectionState = SOCKET_SYNINIT ; // SYNINIT setzen so lange wie das packet noch nicht beantwortet wurde, wird in MakeTCP gebraucht um de MSS zu senden
					TCP_sockettable[ Socket ].Windowsize = TCP_packet->TCP_Window; // Windowsize setzen, wird gebraucht um zu wissen wiviel gesendet werden kann ohne ACK
					TCP_sockettable[ Socket ].AcknowledgeNumber++; // SequenceNumber um 1 erhöhen, das gehört zur SYN-sequence dazu
					MakeTCPheader( Socket, TCP_SYN_FLAG | TCP_ACK_FLAG, 0 , MAX_RECIVEBUFFER_LENGHT , ethernetbuffer ); // Baue mal den TCP-Header mit Berechnung des Pseudoheader
					TCP_sockettable[ Socket ].SequenceNumber++; // SequenceNumber um 1 erhöhen, das gehört zur SYN-sequence dazu
					TCP_sockettable[ Socket ].ConnectionState = SOCKET_WAIT2SYNACK ; // State für den Socket auf WAIT2SYNACK und den den SYN abschließen zu können
					TCP_sockettable[ Socket ].SendState = SOCKET_READY2SEND ; // bereit zum senden
					TCP_sockettable[ Socket ].Timeoutcounter = 2;
					#ifdef _DEBUG_
						printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
						printf_P( PSTR("Verbindungsanforderung bestaetigt [SYN + ACK]\r\n") , ChangeEndian16bit ( TCP_sockettable[ Socket ].DestinationPort ) );
					#endif
				}
				return;
			}
		
		// Wenn immer noch keine Verbindung zugeordnet beenden
		if ( Socket == 0xffff ) return;
		// Timeout füer Socket zuruecksetzen
		#ifdef _DEBUG_
			printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
			printf_P(PSTR("Verbindungszuordnung auf Socket %d\r\n") , Socket );
		#endif
		// den Timeoutcounter wieder zuruecksetzen für die Verbindung auf den zugeordneten Socket
		TCP_sockettable[ Socket ].Timeoutcounter = TimeOutCounter;
				
		if ( TCP_packet->TCP_ControllFlags == ( TCP_SYN_FLAG + TCP_ACK_FLAG ) ) // Packet mit SYN + ACK ?
			{
				#ifdef _DEBUG_
					printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
					printf("Verbindungsbestaetigung [SYN + ACK]\r\n");
				#endif
				// SequenceNumber um 1 erhöhen, das gehört zur SYN-sequence dazu
				TCP_sockettable[ Socket ].SequenceNumber = ChangeEndian32bit( TCP_packet->TCP_AcknowledgeNumber );
				TCP_sockettable[ Socket ].AcknowledgeNumber = ChangeEndian32bit( TCP_packet->TCP_SequenceNumber );
				TCP_sockettable[ Socket ].AcknowledgeNumber++;
				// Windowsize setzen, wird gebraucht um zu wissen wiviel gesendet werden kann ohne ACK
				TCP_sockettable[ Socket ].Windowsize = TCP_packet->TCP_Window; 
				// Baue mal den TCP-Header mit Berechnung des Pseudoheader
				// MakeTCPheader( Socket, TCP_ACK_FLAG, 0 , MAX_RECIVEBUFFER_LENGHT , ethernetbuffer );
				TCP_sockettable[ Socket ].ConnectionState = SOCKET_READY ; // State für den Socket auf WAIT2SYNACK und den den SYN abschließen zu können
				TCP_sockettable[ Socket ].SendState = SOCKET_READY2SEND ; // bereit zum senden
				return;
			}

		// anzahl der Daten im TCP-Datagram berechnen
		i =	( ChangeEndian16bit ( IP_packet->IP_Totallenght )
					- ( ( IP_packet->IP_Version_Headerlen & 0x0f ) * 4 + ( ( TCP_packet->TCP_DataOffset & 0xf0 ) >> 2 ) ) ) ;
		
		if ( Get_FIFOrestsize( TCP_sockettable[ Socket ].fifo ) < i )
		{
			#ifdef _DEBUG_
				printf_P( PSTR("Buffer Error\r\n"));
			#endif
			unsigned char BUFFER[128]="\0";
			return;
		}
		
		// Wenn Daten vorhanden einffach mal kopieren in den Socket-Puffer und bestätigung senden
		if ( i != 0 )
		{
			// Richtige Reihnfolger der Daten ?
			// Hier kann erzwungen werden das die in der richtigen reihnfolge ankommen, 
			// wenn nicht wird ein ACK-Packet mit einer alten Seqnummer gesendet
			if ( ( TCP_sockettable[ Socket ].AcknowledgeNumber ) == ( ChangeEndian32bit ( TCP_packet->TCP_SequenceNumber ) ) )
			{
				#ifdef _DEBUG_
					printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
					printf_P( PSTR("Richtige reinfolge der Packete!\r\n") );
					printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
					printf_P( PSTR("%d Byte befinden sich im Recivebuffer [PSH + ACK] %d Byte empfangen\r\n"), Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) , i);
				#endif
				// Wenn kopieren in Puffer ok war, Acknummer richtig setzen, sonst nix machen, dann wird automatisch die alte Seqnummer gesendet
				// mit der richtigen Windowsize
				if ( CopyTCPdata2socketbuffer( Socket, i , ethernetbuffer ) != 0xffff )
					TCP_sockettable[ Socket ].AcknowledgeNumber = ChangeEndian32bit ( TCP_packet->TCP_SequenceNumber ) + i ;					

				// ACK senden
				MakeTCPheader( Socket, TCP_ACK_FLAG, 0 , ( MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) ) , ethernetbuffer );
				#ifdef _DEBUG_
					printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
					printf_P( PSTR("Bestaetigung gesendet [ACK]\r\n"), Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) , i);
				#endif
				
				// schau mal nach ob vorher ein TCP-Packet kam was in der falschen Reinfolge an kam
				if ( TCP_Unsort.socket == Socket && TCP_Unsort.lenght != 0 )
				{
					if ( TCP_sockettable[ Socket ].AcknowledgeNumber + TCP_Unsort.lenght == TCP_Unsort.Sequencenumber + TCP_Unsort.lenght )
					{
						LockEthernet();
						// SREG sichern um state zu behalten und interrupts freigeben
						sreg_tmp = SREG;
						sei();
						Put_Block_in_FIFO ( TCP_sockettable[ Socket ].fifo , TCP_Unsort.lenght, TCP_Unsort.Recivebuffer );
						// SREG wiederherstellen
						SREG = sreg_tmp;
						FreeEthernet();
						
						TCP_sockettable[ Socket ].AcknowledgeNumber = TCP_Unsort.Sequencenumber + i ;
						MakeTCPheader( Socket, TCP_ACK_FLAG, 0 , ( MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) ) , ethernetbuffer );
					}
					else
					{
						RXErrorCounter++;
					}
				}
				TCP_Unsort.lenght = 0;

			}
			else
			{
				
				// Naja, falsche reihnfolge kann man nur sagen :-) aber kein Problem
				#ifdef _DEBUG_
					printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
					printf_P( PSTR("Falsche reinfolge der Packete!\r\n") );
				#endif

				// alte sequencenummer ? wenn ja nochmal bestätigen
				if ( ( TCP_sockettable[ Socket ].AcknowledgeNumber + i ) > ( ChangeEndian32bit ( TCP_packet->TCP_SequenceNumber ) + i ) )
				{
					unsigned long ACK;
					
					ACK = TCP_sockettable[ Socket ].AcknowledgeNumber;
					TCP_sockettable[ Socket ].AcknowledgeNumber = TCP_packet->TCP_SequenceNumber;
					MakeTCPheader( Socket, TCP_ACK_FLAG, 0 , ( MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) ) , ethernetbuffer );
					TCP_sockettable[ Socket ].AcknowledgeNumber = ACK;
					
					RXErrorOldSeq++;
				}
				else
				{
					// speicher mal die Daten zwischen, damit sie nicht verloren gehen, aber nur wenn der Puffer noch nicht belegt ist, sonst RX-Error
					if ( TCP_Unsort.lenght == 0 )
					{
						unsigned int Offset = ETHERNET_HEADER_LENGTH + ( IP_packet->IP_Version_Headerlen & 0x0f ) * 4 + ( ( TCP_packet->TCP_DataOffset & 0xf0 ) >> 2 ) ;				
						TCP_Unsort.Sequencenumber = ChangeEndian32bit ( TCP_packet->TCP_SequenceNumber );
						TCP_Unsort.socket = Socket ;
						
						LockEthernet();
						// SREG sichern um state zu behalten und interrupts freigeben
						sreg_tmp = SREG;
						sei();
						memcpy( TCP_Unsort.Recivebuffer , &ethernetbuffer[ Offset ] , i );
						// SREG wiederherstellen
						SREG = sreg_tmp;
						FreeEthernet();
						
						TCP_Unsort.lenght = i;					
						RXErrorUnsort++;
						}
					else
					{
						RXErrorCounter++;
					}
				}
			}
		}

		// Anforderung für das schliessen des Socket/Verbindung
		if ( TCP_packet->TCP_ControllFlags == ( TCP_FIN_FLAG ) )
		{
			// haben wir die Schliessung angefordert ?
			if ( TCP_sockettable[ Socket ].ConnectionState == SOCKET_WAIT2FIN )
			{
				TCP_sockettable[ Socket ].AcknowledgeNumber = ChangeEndian32bit ( TCP_packet->TCP_SequenceNumber ) + 1;
				MakeTCPheader( Socket, TCP_ACK_FLAG, 0 , ( MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) ) , ethernetbuffer );
				TCP_sockettable[ Socket ].ConnectionState = SOCKET_NOT_USE ;
			}
			// haben wir die Schliessung angefordert ?
			if ( TCP_sockettable[ Socket ].ConnectionState == SOCKET_WAIT2FINACK )
			{
				TCP_sockettable[ Socket ].AcknowledgeNumber = ChangeEndian32bit ( TCP_packet->TCP_SequenceNumber ) + 1;
				MakeTCPheader( Socket, TCP_ACK_FLAG, 0 , ( MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) ) , ethernetbuffer );
				TCP_sockettable[ Socket ].ConnectionState = SOCKET_NOT_USE ;
			}
			// Client hat Schliessung angefordert
			else if ( TCP_sockettable[ Socket ].ConnectionState == SOCKET_READY ) 
			{
				TCP_sockettable[ Socket ].SequenceNumber = ChangeEndian32bit ( TCP_packet->TCP_AcknowledgeNumber ) + 1;
				MakeTCPheader( Socket, TCP_ACK_FLAG, 0 , ( MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) ) , ethernetbuffer );
				MakeTCPheader( Socket, TCP_FIN_FLAG , 0 , ( MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) ) , ethernetbuffer );
				TCP_sockettable[ Socket ].ConnectionState = SOCKET_WAIT2FIN ;
			}				

			// Laut RFC ist Seqnummer um 1 zu erhöhen beim FIN
			TCP_sockettable[ Socket ].SequenceNumber++;
			#ifdef _DEBUG_
				printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
				printf_P(PSTR("Verbindung wird geschlossen [FIN + ACK]\r\n"), Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) , i);
			#endif
			return;
		}

		// normale bestätigung der gesendeten Daten
		if ( TCP_packet->TCP_ControllFlags == TCP_ACK_FLAG ) 						// mach mal wenn ACK empfangen
			{
				// War das ein ACK von einen Verbindungsaufbau ? Wenn ja steht die verbindung jetzt
				if ( TCP_sockettable[ Socket ].ConnectionState == SOCKET_WAIT2SYNACK )  			// Wartet socket auf SYN + ACK ?
				{
					TCP_sockettable[ Socket ].ConnectionState = SOCKET_READY2USE;					// Socket auf Ready2Use setzen
					TCP_sockettable[ Socket ].SendState = SOCKET_READY2SEND;						// Socket auf Bereit um senden setzen
					#ifdef _DEBUG_
						printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
						printf_P( PSTR("Verbindung ist aufgebaut und READY2USE [ACK]\r\n"));
					#endif
				}				
				else if ( TCP_sockettable[ Socket ].ConnectionState == SOCKET_WAIT2FIN ) 
				{
					TCP_sockettable[ Socket ].ConnectionState=SOCKET_NOT_USE;
					#ifdef _DEBUG_
						printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
						printf_P( PSTR("Verbindung geschlossen [ACK]\r\n"));
					#endif
				}
				else if ( TCP_sockettable[ Socket ].ConnectionState == SOCKET_WAIT2FINACK )
				{
					TCP_sockettable[ Socket ].AcknowledgeNumber = ChangeEndian32bit ( TCP_packet->TCP_SequenceNumber ) + 1;
					TCP_sockettable[ Socket ].SequenceNumber = ChangeEndian32bit ( TCP_packet->TCP_AcknowledgeNumber );
					MakeTCPheader( Socket, TCP_ACK_FLAG, 0 , ( MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) ) , ethernetbuffer );
					TCP_sockettable[ Socket ].ConnectionState = SOCKET_WAIT2FIN ;
				}
				else if ( TCP_sockettable[ Socket ].SequenceNumber == ChangeEndian32bit ( TCP_packet->TCP_AcknowledgeNumber ) )
				{
					#ifdef _DEBUG_
						printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
						printf_P( PSTR("Empfang bestaetig [ACK] SendState aud READY2SEND\r\n"));
					#endif
					TCP_sockettable[ Socket ].SendetBytes = 0;
				}
			}
		return;
	}
		
/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Der TCP-Timeouthandler
 * Dieser Handler schaut zyklisch in alle Sockets und verringert den Timeoutwert. Wenn ein Timeoutwert 0 erreicht
 * hat, wird das Socket geschlossen und der Connectionstats richtig gesetz.
 * \warning Die Funktion wir in der tcp_init() beim timerinterrupt registiert und fortan zyklisch aufgerufen.
 * \retval	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/
void TCPTimeOutHandler( void )
	{
		unsigned int Socket;
		// durchläufe MAX_TCP_CONNECTIONS

		
		for ( Socket = 0 ; Socket < MAX_TCP_CONNECTIONS ; Socket ++ )
			{
				// Wenn TimeOutcounter unsgleich 0 dann verringern
				if ( TCP_sockettable[ Socket ].Timeoutcounter != 0 ) TCP_sockettable[ Socket ].Timeoutcounter--;
				
				if ( TCP_sockettable[ Socket ].Timeoutcounter == 0 && TCP_sockettable[ Socket ].ConnectionState != SOCKET_NOT_USE )
				{
					unsigned char * ethernetbuffer;
					ethernetbuffer = (unsigned char*) __builtin_alloca (( size_t ) ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT + TCP_HEADER_LENGHT );
					LockEthernet();
					MakeTCPheader( Socket, TCP_FIN_FLAG | TCP_ACK_FLAG , 0 , 0 , ethernetbuffer );
					TCP_sockettable[ Socket ].ConnectionState = SOCKET_NOT_USE ;						
					FreeEthernet();
					#ifdef _DEBUG_
						printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
						printf_P( PSTR("Connection timeout oder Verbindungsabriss\r\n"));
					#endif
				}
			}
	}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Holt das naechste freie Socket
 * \warning Die Funktion wir in der tcp_init() beim timerinterrupt registiert und fortan zyklisch aufgerufen.
 * \retval	SOCKET	Gibt die Nummer des naechsten Freien SOcket.
 */
/*------------------------------------------------------------------------------------------------------------*/
unsigned int Getfreesocket( void )
	{
		unsigned int Socket;
		for ( Socket = 0 ; Socket < MAX_TCP_CONNECTIONS ; Socket++ )
			{
				if ( TCP_sockettable[ Socket ].ConnectionState == SOCKET_NOT_USE ) return( Socket );
			}
		return(0xffff);
	}		

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Ordnet das Packet einen Socket zu.
 * \param	ethernetbuffer	Zeiger auf den Ethernetpuffer.
 * \retval	Socket	Im Erfolgsfall die Socketnummer, im Fehlerfall 0xffff
 */
/*------------------------------------------------------------------------------------------------------------*/
unsigned int GetSocket( unsigned char * ethernetbuffer )
	{
		struct ETH_header *ETH_packet; 		// ETH_struct anlegen
		ETH_packet = (struct ETH_header *) ethernetbuffer;
		struct IP_header *IP_packet;		// IP_struct anlegen
		IP_packet = ( struct IP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH];
		struct TCP_header *TCP_packet;		// TCP_struct anlegen
		TCP_packet = ( struct TCP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH + ((IP_packet->IP_Version_Headerlen & 0x0f) * 4 )];

		unsigned int Socket;
		
		for ( Socket = 0 ; Socket < MAX_TCP_CONNECTIONS ; Socket++ ) 
			{
				if ( 	TCP_sockettable[ Socket ].ConnectionState != SOCKET_NOT_USE 
						&& TCP_sockettable[ Socket ].SourcePort == TCP_packet->TCP_SourcePort 
						&& TCP_sockettable[ Socket ].DestinationPort == TCP_packet->TCP_DestinationPort 
						&& TCP_sockettable[ Socket ].SourceIP == IP_packet->IP_SourceIP ) return( Socket );
			}
		return(0xffff);
	}		
	
/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Reistriert einen neuen Socket wenn noch einer frei ist und sichert alle nötigen daten in ihn.
 * \param	ethernetbuffer	Zeiger auf den Ethernetpuffer.
 * \retval	Socket			Im Erfolgsfall die Socketnummer, im Fehlerfall 0xffff
 */
/*------------------------------------------------------------------------------------------------------------*/
unsigned int RegisterSocket( unsigned char *ethernetbuffer)
	{
		struct ETH_header *ETH_packet; 		// ETH_struct anlegen
		ETH_packet = (struct ETH_header *) ethernetbuffer;
		struct IP_header *IP_packet;		// IP_struct anlegen
		IP_packet = ( struct IP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH];
		struct TCP_header *TCP_packet;		// TCP_struct anlegen
		TCP_packet = ( struct TCP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH + ((IP_packet->IP_Version_Headerlen & 0x0f) * 4 )];

		unsigned int Socket;
		
		Socket = Getfreesocket();
		if ( Socket == 0xffff ) return(0xffff);
		
		// Register den SOCKET
		TCP_sockettable[ Socket ].SourcePort = TCP_packet->TCP_SourcePort;
		TCP_sockettable[ Socket ].DestinationPort = TCP_packet->TCP_DestinationPort;
		TCP_sockettable[ Socket ].SourceIP = IP_packet->IP_SourceIP;
		TCP_sockettable[ Socket ].SequenceNumber =~ ChangeEndian32bit( TCP_packet->TCP_SequenceNumber );
		TCP_sockettable[ Socket ].AcknowledgeNumber = ChangeEndian32bit( TCP_packet->TCP_SequenceNumber );
		TCP_sockettable[ Socket ].SendState = SOCKET_READY2SEND;
		TCP_sockettable[ Socket ].Timeoutcounter = 10;
		Flush_FIFO ( TCP_sockettable[ Socket ].fifo );
		TCP_sockettable[ Socket ].Windowsize = TCP_packet->TCP_Window ;
		TCP_sockettable[ Socket ].SendetBytes = 0;
		
		for ( unsigned int i = 0 ; i < 6 ; i++ ) TCP_sockettable[ Socket ].MACadress[i] = ETH_packet->ETH_sourceMac[i];
		
		return( Socket );
	}

/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Baut einen TCP-header und berechnet den Pseudoheader und die Checksumme 
 *	Übergeben werden müssen die TCP_FLAGS, Datenlänge den Datensegments, die Windowsize, und der Pointer auf den Buffer
 * \param	ethernetbuffer	Zeiger auf den Ethernetpuffer.
 * \retval	Socket			Im Erfolgsfall die Socketnummer, im Fehlerfall 0xffff
 */
/*------------------------------------------------------------------------------------------------------------*/
void MakeTCPheader( unsigned int Socket, unsigned char TCP_flags, unsigned int Datalenght, unsigned int Windowsize, unsigned char * ethernetbuffer )
	{
		struct TCP_header *TCP_packet;		// TCP_struct anlegen
		TCP_packet = ( struct TCP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT ];

		struct IP_Pseudoheader *IP_pseudopacket;
		IP_pseudopacket = ( struct IP_Pseudoheader *) &ethernetbuffer[ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT - IP_PSEUDOHEADER_LENGHT ];

		// Bastel mal den Pseudoheader
		IP_pseudopacket->IP_SourceIP = TCP_sockettable[ Socket ].SourceIP;
		IP_pseudopacket->IP_DestinationIP = myIP;
		IP_pseudopacket->IP_ZERO = 0x0;
		IP_pseudopacket->IP_Protokoll = 0x6;
		IP_pseudopacket->IP_TCP_lenght = ChangeEndian16bit( TCP_HEADER_LENGHT + Datalenght );										
			
		// TCP_header basteln
		TCP_packet->TCP_SourcePort = TCP_sockettable[ Socket ].DestinationPort;
		TCP_packet->TCP_DestinationPort = TCP_sockettable[ Socket ].SourcePort;
		TCP_packet->TCP_SequenceNumber = ChangeEndian32bit( TCP_sockettable[ Socket ].SequenceNumber );
		TCP_packet->TCP_AcknowledgeNumber = ChangeEndian32bit( TCP_sockettable[ Socket ].AcknowledgeNumber );
		TCP_packet->TCP_DataOffset = ( ( TCP_HEADER_LENGHT << 2 ) & 0xf0 ) ;
		TCP_packet->TCP_ControllFlags = TCP_flags ;
		TCP_packet->TCP_Window = ChangeEndian16bit( Windowsize );
		TCP_packet->TCP_Checksum = 0x0;
		TCP_packet->TCP_UrgentPointer = 0x0;
		
		if ( TCP_sockettable[ Socket ].ConnectionState == SOCKET_SYNINIT )
		{
			// beschreibe das Optionfeld mit MSS = MAX_TCP_Datalenght wenn der SYN ausgehandelt wird
			TCP_packet->TCP_Options[0] = 0x02;
			TCP_packet->TCP_Options[1] = 0x04;
			TCP_packet->TCP_Options[2] = ( MAX_TCP_Datalenght >> 8 ) & 0x00ff ;
			TCP_packet->TCP_Options[3] = MAX_TCP_Datalenght & 0x00ff ;
		}
		else
		{
			// lasse Optionfeld leer wenn kein SYN
			TCP_packet->TCP_Options[0] = 0;
			TCP_packet->TCP_Options[1] = 0;
			TCP_packet->TCP_Options[2] = 0;
			TCP_packet->TCP_Options[3] = 0;
		}
			
		TCP_packet->TCP_Checksum = ChangeEndian16bit( Checksum_16( 	&ethernetbuffer[ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT - IP_PSEUDOHEADER_LENGHT ],
																	TCP_HEADER_LENGHT + IP_PSEUDOHEADER_LENGHT  + Datalenght ) ) ;
		MakeIPheader( TCP_sockettable[ Socket ].SourceIP, PROTO_TCP, TCP_HEADER_LENGHT + Datalenght , ethernetbuffer );
		MakeETHheader( TCP_sockettable[ Socket ].MACadress, ethernetbuffer );
		sendEthernetframe( ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT + TCP_HEADER_LENGHT + Datalenght, ethernetbuffer );
		
		return;
	}
		
/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Kopiert die Daten auf den TCP-packet in den Socketpuffer
 * \param	Socket		Die Socketnummer die zum versnden benutzt werden soll.
 * \param	Datalenght	Die größe der Daten in Bytes.
 * \param	ethernetbuffer		Zeiger auf den Speicher wo die Daten hin kopiert werden soll.
 * \retval	0
 */
/*------------------------------------------------------------------------------------------------------------*/	
unsigned int CopyTCPdata2socketbuffer( unsigned int Socket, unsigned int Datalenght , unsigned char *ethernetbuffer )
	{
		char sreg_tmp;
		
		struct ETH_header *ETH_packet; 		// ETH_struct anlegen
		ETH_packet = (struct ETH_header *) ethernetbuffer;
		struct IP_header *IP_packet;		// IP_struct anlegen
		IP_packet = ( struct IP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH];
		struct TCP_header *TCP_packet;		// TCP_struct anlegen
		TCP_packet = ( struct TCP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH + ((IP_packet->IP_Version_Headerlen & 0x0f) * 4 )];
			
		// Abbrechen wenn die Datem nicht mehr in den buffer passen
		if ( ( Get_FIFOrestsize ( TCP_sockettable[ Socket ].fifo ) ) < Datalenght ) return( 0xffff );
		
		unsigned int Offset = ETHERNET_HEADER_LENGTH + ( IP_packet->IP_Version_Headerlen & 0x0f ) * 4 + ( ( TCP_packet->TCP_DataOffset & 0xf0 ) >> 2 ) ;

		// gibt mal interrupts im interrupt frei außer den netzwerkinterrupt, da das kopieren der daten manchmal zu lange dauert und sonst
		// die Clock hinterher hinkt :-(
		LockEthernet();
   		sreg_tmp = SREG;    /* Sichern */
		sei();
		Put_Block_in_FIFO ( TCP_sockettable[ Socket ].fifo, Datalenght, &ethernetbuffer[ Offset ] );
		SREG = sreg_tmp;
		FreeEthernet();

		return( Datalenght );
	}		
	
/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Registriert einen Port auf den gelauscht wird für einegehende Verbindungen
 * \param	Port	Der Port auf den Gelauscht werden soll
 * \retval	Im Erfolgsfall 0, im Fehlerfall 0xffff
 */
/*------------------------------------------------------------------------------------------------------------*/
unsigned int RegisterTCPPort( unsigned int Port )
	{
		unsigned int i;
		
		for ( i = 0 ; i < MAX_LISTEN_PORTS ; i ++ ) 
			{
				if ( TCP_porttable[i].TCP_Port == TCP_Port_not_use ) 
				{
					TCP_porttable[i].TCP_Port = Port;
					return(0);
				}
			}
		return(0xffff);
	}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Traegt einen Port aus der Liste der Port aus auf denen gelauscht werden soll
 * \param	Port	Der Port auf den Gelauscht werden soll
 * \return	Im Erfolgsfall 0, im Fehlerfall 0xffff
 */
/*------------------------------------------------------------------------------------------------------------*/
void UnRegisterTCPPort( unsigned int Port )
	{
		unsigned int i;
		
		for ( i = 0 ; i < MAX_LISTEN_PORTS ; i ++ ) 
			{
				if ( TCP_porttable[i].TCP_Port == Port ) TCP_porttable[i].TCP_Port = TCP_Port_not_use;
			}
		return;
	}
	
/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Holt empfangende Daten bis zum "\r\n" aus den Socketpuffer.
 * \param	Port	Der Port der gecheckt werden soll.
 * \retval	0x0 wenn der Port in der Liste ist und 0xffff wenn er nicht in der Liste ist.
 */
/*------------------------------------------------------------------------------------------------------------*/	
unsigned int CheckPortInList( unsigned int Port )
	{
		unsigned int i;
		
		for ( i = 0 ; i < MAX_LISTEN_PORTS ; i ++ ) 
			{
				if ( TCP_porttable[i].TCP_Port == Port ) return(0);
			}
		return( 0xffff );
	}
	
/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Schaut ob auf einen Port ein Verbindung eingegangen ist.
 * \param	Port	Der Port der abgefragt wird.
 * \return	Im Erfolgsfall die Socketnummer, im Fehlerfall 0xffff
 */
/*------------------------------------------------------------------------------------------------------------*/	
unsigned int CheckPortRequest( unsigned int Port )
	{
		for ( unsigned int i = 0 ; i < MAX_TCP_CONNECTIONS ; i ++ ) 
			{
				if ( TCP_sockettable[i].DestinationPort == ChangeEndian16bit( Port ) && TCP_sockettable[i].ConnectionState == SOCKET_READY2USE ) 
				{
					TCP_sockettable[i].ConnectionState = SOCKET_READY;
					return(i);
				}
			}
		return( 0xffff );
	}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Gibt den Status des Socket aus.
 * \param	Socket		Die Socketnummer vom dem der Status zurueckgegeben werden soll.
 * \return	Der Socketstate
 */
/*------------------------------------------------------------------------------------------------------------*/	
unsigned int CheckSocketState( unsigned int Socket )
	{
		return( TCP_sockettable[ Socket ].ConnectionState );
	}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Schliesst ein Socket und Beendet die TCP-Verbindung.
 * \param	Socket		Die Socketnummer die geschlossen werden soll.
 */
/*------------------------------------------------------------------------------------------------------------*/		
void CloseTCPSocket( unsigned int Socket)
	{
		if ( Socket >= MAX_TCP_CONNECTIONS || TCP_sockettable[Socket].ConnectionState == SOCKET_NOT_USE ) return;
					
		unsigned char timer;
		
		timer = CLOCK_RegisterCoundowntimer();
		CLOCK_SetCountdownTimer( timer , CLOSETIMEOUT, MSECOUND );

		while ( 1 )
		{
			if ( ( TCP_sockettable[ Socket ].SendState == SOCKET_READY2SEND ) && ( CLOCK_GetCountdownTimer( timer ) != 0 ) )
			{
				CLOCK_ReleaseCountdownTimer( timer );
				break;
			}
			if ( CLOCK_GetCountdownTimer( timer ) == 0 ) 
			{
				#ifdef _DEBUG_
					printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
					printf_P( PSTR("Close-Timeout. Verbindung wird geschlossen, Socket nicht bereit!\r\n") );
				#endif
				TCP_sockettable[ Socket ].ConnectionState = SOCKET_NOT_USE ;
				CLOCK_ReleaseCountdownTimer( timer );
				return;
			}
		}

		#ifdef _DEBUG_
			printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
			printf_P( PSTR("Verbindung kann geschlossen werden\r\n") );
		#endif
		
		LockEthernet();
		
		unsigned char * ethernetbuffer;
		ethernetbuffer = (unsigned char*) __builtin_alloca (( size_t ) ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT + TCP_HEADER_LENGHT );
		
		struct TCP_header *TCP_packet;		// TCP_struct anlegen
		TCP_packet = ( struct TCP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT ];

		MakeTCPheader( Socket, TCP_FIN_FLAG | TCP_ACK_FLAG , 0, 0, ethernetbuffer );

		TCP_sockettable[ Socket ].ConnectionState = SOCKET_WAIT2FINACK;
	
		FreeEthernet();		

		timer = CLOCK_RegisterCoundowntimer();
		CLOCK_SetCountdownTimer( timer , CLOSETIMEOUT, MSECOUND );
		
		while ( 1 )
		{
			LockEthernet();
			
			if ( ( TCP_sockettable[ Socket ].ConnectionState == SOCKET_NOT_USE ) && ( CLOCK_GetCountdownTimer( timer ) != 0 ) )
			{
				CLOCK_ReleaseCountdownTimer( timer );
				FreeEthernet();
				break;
			}
			
			FreeEthernet();

			if ( CLOCK_GetCountdownTimer( timer ) == 0 ) 
			{
				#ifdef _DEBUG_
					printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
					printf_P( PSTR("Close-Timeout. Verbindung wird geschlossen. State von Socket %d auf SOCKET_NOT_USE\r\n"), Socket );
				#endif

				LockEthernet();
				MakeTCPheader( Socket, TCP_FIN_FLAG , 0, 0, ethernetbuffer );
				TCP_sockettable[ Socket ].ConnectionState = SOCKET_NOT_USE ;
				FreeEthernet();

				CLOCK_ReleaseCountdownTimer( timer );
				break;
			}
		}
		
		// TCP_sockettable[ Socket ].fifo = TCP_sockettable[ Socket ].old_fifo;

		return;
	}	

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Sendet Daten ueber ein Socket aus dem RAM/FLASH/EEPROM.
 * \param	Socket		Die Socketnummer die zum versnden benutzt werden soll.
 * \param	Datalenght	Die Datenlaenge in Bytes die sersendet werden soll.
 * \param	Sendbuffer	Zeiger auf die Dten im RAM der versendet werden soll.
 * \param	Mode		Art des Zeigers (RAM/FLASH/EEPROM).
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/	
int SendData_RPE( unsigned int Socket, unsigned int Datalenght, unsigned char * Sendbuffer, unsigned char Mode, int RetransmissionCounter )	
	{
		if ( Socket >= MAX_TCP_CONNECTIONS || TCP_sockettable[Socket].ConnectionState == SOCKET_NOT_USE ) return;
		
		unsigned char timer;
		
		unsigned char * ethernetbuffer;
		ethernetbuffer = (unsigned char*) __builtin_alloca (( size_t ) ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT + TCP_HEADER_LENGHT + Datalenght );

		struct TCP_header *TCP_packet;		// TCP_struct anlegen
		TCP_packet = ( struct TCP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT ];	

		unsigned int Offset = ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT + TCP_HEADER_LENGHT ;
		unsigned int i;

		switch ( Mode )
		{
			case RAM:	for ( i = 0 ; i < Datalenght ; i++ )
								ethernetbuffer[ Offset + i ] = Sendbuffer[ i ]; // kopiere die daten in den Sendebiffer
						break;
			case FLASH:	for ( i = 0 ; i < Datalenght ; i++ )
								ethernetbuffer[ Offset + i ] = pgm_read_byte ( Sendbuffer + i ) ;
						break;
			default:	return( 0xffff );
		}

		LockEthernet();
		
		MakeTCPheader( Socket, TCP_PSH_FLAG | TCP_ACK_FLAG , Datalenght, MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) , ethernetbuffer );
		TCP_sockettable[ Socket ].SequenceNumber = TCP_sockettable[ Socket ].SequenceNumber + i ;
		TCP_sockettable[ Socket ].SendetBytes = Datalenght ;

		FreeEthernet();
		
		timer = CLOCK_RegisterCoundowntimer();
		
		if ( RetransmissionCounter < 1 )
			CLOCK_SetCountdownTimer( timer , RETRANSMISSIONTIMEOUT/8 , MSECOUND );
		else if ( RetransmissionCounter < MAX_TCP_RETRANSMISSIONS/4 )
			CLOCK_SetCountdownTimer( timer , RETRANSMISSIONTIMEOUT/4 , MSECOUND );
		else if ( RetransmissionCounter < MAX_TCP_RETRANSMISSIONS/2 )
			CLOCK_SetCountdownTimer( timer , RETRANSMISSIONTIMEOUT , MSECOUND );
		else if ( RetransmissionCounter < MAX_TCP_RETRANSMISSIONS )
			CLOCK_SetCountdownTimer( timer , RETRANSMISSIONTIMEOUT , MSECOUND );

		while(1)
		{
			LockEthernet();
			if( CLOCK_GetCountdownTimer( timer ) == 0 )
			{
				CLOCK_ReleaseCountdownTimer ( timer );
				TCP_sockettable[ Socket ].SequenceNumber = TCP_sockettable[ Socket ].SequenceNumber - i ;
				FreeEthernet();
				TXErrorCounter++;
				return( -1 );
			}
			else
			{
				if ( TCP_sockettable[ Socket ].SendetBytes == 0 )
				{
					CLOCK_ReleaseCountdownTimer ( timer );
					FreeEthernet();
					#ifdef _DEBUG_
						printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
						printf_P( PSTR("Daten erfolgreich gesendet.\r\n"), Socket );
					#endif
					return( Datalenght );
				}
			}
			FreeEthernet();
		}
	}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Sendet Daten ueber ein Socket aus dem RAM/FLASH/EEPROM.
 * \param	Socket		Die Socketnummer die zum versnden benutzt werden soll.
 * \param	Datalenght	Die Datenlaenge in Bytes die sersendet werden soll.
 * \param	Sendbuffer	Zeiger auf die Daten im FLASH der versendet werden soll.
 * \param	Mode		Wie der Zeiger interpretiert werden soll, als RAM/FLASH/EEPROM.
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/	
unsigned int PutSocketData_RPE( unsigned int Socket, unsigned int Datalenght, unsigned char * Sendbuffer, unsigned char Mode )
	{
		// schau mal ob socket gültig ist
		if ( Socket >= MAX_TCP_CONNECTIONS || TCP_sockettable[Socket].ConnectionState == SOCKET_NOT_USE ) return( NO_SOCKET_USED );
			
		int Transmission=0,i,packet,TransmissionCounter ;		

			// Anzahl der Packet berechnen die die maximallänge haben
		packet = ( Datalenght / MAX_TCP_Datalenght );
		
		// Packet mit maximallänge senden
		for ( i = 0 ; i < packet ; i++ )
		{
			Transmission = 1;
			TransmissionCounter = 0;
			while( Transmission != 0 && TransmissionCounter < MAX_TCP_RETRANSMISSIONS )
			{
				switch ( Mode )
				{
					case RAM:	if ( SendData_RPE( Socket, MAX_TCP_Datalenght , &Sendbuffer[i * MAX_TCP_Datalenght ], RAM, TransmissionCounter ) != -1 )
								{
									Transmission = 0;
								}
								else
								{
									Transmission = 1;
									TransmissionCounter++;
								}
								break;
					case FLASH:	if ( SendData_RPE( Socket, MAX_TCP_Datalenght , &Sendbuffer[i * MAX_TCP_Datalenght ], FLASH, TransmissionCounter ) != -1 )
								{
									Transmission = 0;
								}
								else
								{
									Transmission = 1;
									TransmissionCounter++;
								}
								break;
					default:	return( 0xffff );
				}
			}
		}
		
		
		// Wenn noch Byte übrig sind, senden
		if ( ( Datalenght % MAX_TCP_Datalenght ) != 0 )
		{
			Transmission = 0;
			TransmissionCounter = 0;
			while( Transmission == 0 && TransmissionCounter < MAX_TCP_RETRANSMISSIONS )
			{
				switch ( Mode )
				{
					case RAM:	if ( SendData_RPE( Socket, Datalenght % MAX_TCP_Datalenght , &Sendbuffer[i * MAX_TCP_Datalenght ], RAM, TransmissionCounter ) != -1 )
								{
									Transmission = 1;
								}
								else
								{
									Transmission = 0;
									TransmissionCounter++;
								}
								break;
					case FLASH:	if ( SendData_RPE( Socket, Datalenght % MAX_TCP_Datalenght , &Sendbuffer[i * MAX_TCP_Datalenght ], FLASH, TransmissionCounter ) != -1)
								{
									Transmission = 1;
								}
								else
								{
									Transmission = 0;
									TransmissionCounter++;
								}
								break;
					default:	return( 0xffff );
				}
			}
		}
						
		// anzahl der gesendet daten zurück geben
		return( Datalenght );		
	}
	
/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Sendet Daten ueber ein Socket aus dem FLASH.
 * \todo Die Funktion ist veraltet und sollte nicht mehr genutzt werden, fliegt mit den nächsten Revisionen Raus.
 * \param	Socket		Die Socketnummer die zum versnden benutzt werden soll.
 * \param	Datalenght	Die Datenlaenge in Bytes die sersendet werden soll.
 * \param	Sendbuffer	Zeiger auf die Daten im FLASH der versendet werden soll.
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/	
unsigned int PutSocketData_P( unsigned int Socket, unsigned int Datalenght, const prog_char * Sendbuffer )
	{
		return( PutSocketData_RPE( Socket, Datalenght, Sendbuffer, FLASH ) );
	}
	
/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Sendet Daten ueber ein Socket aus dem RAM.
 * \todo Die Funktion ist veraltet und sollte nicht mehr genutzt werden, fliegt mit den nächsten Revisionen Raus.
 * \param	Socket		Die Socketnummer die zum versnden benutzt werden soll.
 * \param	Datalenght	Die Datenlaenge in Bytes die sersendet werden soll.
 * \param	Sendbuffer	Zeiger auf die Dten im RAM der versendet werden soll.
 * \return	NONE
 */
/*------------------------------------------------------------------------------------------------------------*/	
unsigned int PutSocketData( unsigned int Socket, unsigned int Datalenght, unsigned char * Sendbuffer )
	{
		return( PutSocketData_RPE( Socket, Datalenght, Sendbuffer, RAM ) );
	}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Holt empfangende Daten bis zum "\r\n" aus den Socketpuffer.
 * \param	Socket		Die Socketnummer die zum versnden benutzt werden soll.
 * \param	bufferlen	Die größe des Puffer in den die Daten gespeichert werden sollen.
 * \param	buffer		Zeiger auf den Speicher wo die Daten hin kopiert werden soll.
 * \retval	Die Anzahl der kopiert Bytes.
 */
/*------------------------------------------------------------------------------------------------------------*/	
unsigned int GetSocketNextLine( unsigned int Socket , unsigned int bufferlen, unsigned char *buffer)
	{
		if ( Socket >= MAX_TCP_CONNECTIONS || TCP_sockettable[Socket].ConnectionState == SOCKET_NOT_USE ) return( NO_SOCKET_USED );
		if ( Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) == 0 ) return(0);
		
		LockEthernet();
		
		unsigned int i,j=0,Byte_in_fifo;
		unsigned char Data;

		Byte_in_fifo = Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo );
		// Buffer kopieren bis \r\n gefunden wurden oder ende erreicht wurden
		for ( i = 0 ; ( i < bufferlen ) && ( i < Byte_in_fifo ) ; i++ )
		{
			Data = Get_Byte_from_FIFO ( TCP_sockettable[ Socket ].fifo );
			buffer[ i ] = Data;
				
			if ( Data == 0x13 || Data == 0x10 ) 
			{
				break;
			}
		}
	
		#ifdef _DEBUG_
			printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
			printf_P( PSTR("%d Byte aus Recivebuffer augeholt, %d byte noch im Recivebuffer\n\r"), i , Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) );
		#endif
		
		// sende ein Windowupdate-Paket wenn buffer zu 7/8 frei ist
		if ( Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) < ( MAX_RECIVEBUFFER_LENGHT / 8 ) )
		{
			unsigned char * ackbuffer;
			ackbuffer = (unsigned char*) __builtin_alloca (( size_t ) ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT + TCP_HEADER_LENGHT );

			struct TCP_header *TCP_packet;		// TCP_struct anlegen
			TCP_packet = ( struct TCP_header *)&ackbuffer[ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT ];							

			MakeTCPheader( Socket, TCP_ACK_FLAG , 0 , MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) , ackbuffer );

			#ifdef _DEBUG_
				printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
				printf_P( PSTR("Neue Windowsize von %d Byte gesendet [ACK]\r\n"), MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) );
			#endif

		}

		FreeEthernet();

		return(i);		
	}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Holt empfangende Daten aus den Socketpuffer.
 * \param	Socket		Die Socketnummer die zum versnden benutzt werden soll.
 * \param	bufferlen	Die größe des Puffer in den die Daten gespeichert werden sollen.
 * \param	buffer		Zeiger auf den Speicher wo die Daten hin kopiert werden soll.
 * \retval	Die Anzahl der kopierten Bytes.
 */
/*------------------------------------------------------------------------------------------------------------*/	
unsigned int GetSocketDataToFIFO( unsigned int Socket , unsigned int fifo, unsigned int bufferlen )
	{
		if ( Socket >= MAX_TCP_CONNECTIONS || TCP_sockettable[Socket].ConnectionState == SOCKET_NOT_USE ) return( NO_SOCKET_USED );
		if ( Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) == 0 ) return ( 0 );
		
		LockEthernet();
		
		unsigned int i = 0;
		
		if ( ( Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) >= bufferlen ) && ( Get_FIFOrestsize ( fifo ) >= bufferlen ) )
		{
			i = Get_FIFO_to_FIFO( TCP_sockettable[ Socket ].fifo , bufferlen, fifo );
		}
		else
		{
			FreeEthernet();
			return( 0 );
		}
		
		#ifdef _DEBUG_
			printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
			printf_P( PSTR("%d Byte aus Recivebuffer augeholt, %d byte noch im Recivebuffer\n\r"), i , Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) );
		#endif
		
		// sende ein Windowupdate-Paket wenn buffer zu 7/8 frei ist
		if ( Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) < ( MAX_RECIVEBUFFER_LENGHT / 2 ) )
		{
			unsigned char * ackbuffer;
			ackbuffer = (unsigned char*) __builtin_alloca (( size_t ) ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT + TCP_HEADER_LENGHT );

			struct TCP_header *TCP_packet;		// TCP_struct anlegen
			TCP_packet = ( struct TCP_header *)&ackbuffer[ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT ];							

			MakeTCPheader( Socket, TCP_ACK_FLAG , 0 , MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) , ackbuffer );

			#ifdef _DEBUG_
				printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
				printf_P( PSTR("Neue Windowsize von %d Byte gesendet [ACK]\r\n"), MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) );
			#endif
		}

		FreeEthernet();
		return( bufferlen );
	}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Holt empfangende Daten aus den Socketpuffer.
 * \param	Socket		Die Socketnummer die zum versnden benutzt werden soll.
 * \param	bufferlen	Die größe des Puffer in den die Daten gespeichert werden sollen.
 * \param	buffer		Zeiger auf den Speicher wo die Daten hin kopiert werden soll.
 * \retval	Die Anzahl der kopierten Bytes.
 */
/*------------------------------------------------------------------------------------------------------------*/	
unsigned int GetSocketData( unsigned int Socket , unsigned int bufferlen, unsigned char *buffer)
	{
		if ( Socket >= MAX_TCP_CONNECTIONS || TCP_sockettable[Socket].ConnectionState == SOCKET_NOT_USE ) return( NO_SOCKET_USED );
		if ( Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) == 0 ) return ( 0 );
		
		LockEthernet();
		
		unsigned int i = 0, Byte_in_fifo=0;
		
		// Wenn der Buffer kleiner als der Empfangsbuffer Buffer voll machen, sonst alles kopieren
		if ( Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) > bufferlen )
		{
			Get_Block_from_FIFO ( TCP_sockettable[ Socket ].fifo, bufferlen, buffer );	
		}
		else
		{
			Byte_in_fifo = Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo );
//			Get_Block_from_FIFO ( TCP_sockettable[ Socket ].fifo, Byte_in_fifo , buffer );	
			for ( i = 0 ; i < Byte_in_fifo ; i++ )
				buffer[ i ] = Get_Byte_from_FIFO ( TCP_sockettable[ Socket ].fifo );
			Flush_FIFO ( TCP_sockettable[ Socket ].fifo );
		}
		
		#ifdef _DEBUG_
			printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
			printf_P( PSTR("%d Byte aus Recivebuffer augeholt, %d byte noch im Recivebuffer\n\r"), i , Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) );
		#endif
		
		// sende ein Windowupdate-Paket wenn buffer zu 3/4 frei ist
		if ( Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) < ( MAX_RECIVEBUFFER_LENGHT / 2 ) )
		{
			unsigned char * ackbuffer;
			ackbuffer = (unsigned char*) __builtin_alloca (( size_t ) ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT + TCP_HEADER_LENGHT );

			struct TCP_header *TCP_packet;		// TCP_struct anlegen
			TCP_packet = ( struct TCP_header *)&ackbuffer[ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT ];							

			MakeTCPheader( Socket, TCP_ACK_FLAG , 0 , MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) , ackbuffer );

			#ifdef _DEBUG_
				printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
				printf_P( PSTR("Neue Windowsize von %d Byte gesendet [ACK]\r\n"), MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) );
			#endif
		}

		FreeEthernet();
		return( i );
	}
	
/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Holt empfangende Daten aus den Socketpuffer.
 * \param	Socket		Die Socketnummer die zum versnden benutzt werden soll.
 * \param	bufferlen	Die größe des Puffer in den die Daten gespeichert werden sollen.
 * \param	buffer		Zeiger auf den Speicher wo die Daten hin kopiert werden soll.
 * \retval	Die Anzahl der kopierten Bytes.
 */
/*------------------------------------------------------------------------------------------------------------*/	
signed int FlushSocketData( unsigned int Socket )
	{
		if ( Socket >= MAX_TCP_CONNECTIONS || TCP_sockettable[Socket].ConnectionState == SOCKET_NOT_USE ) return( -1 );
		
		return( Flush_FIFO( TCP_sockettable[ Socket ].fifo ) );
	}
	
/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Holt empfangende Daten aus den Socketpuffer.
 * \param	Socket		Die Socketnummer die zum versnden benutzt werden soll.
 * \param	bufferlen	Die größe des Puffer in den die Daten gespeichert werden sollen.
 * \param	buffer		Zeiger auf den Speicher wo die Daten hin kopiert werden soll.
 * \retval	Die Anzahl der kopierten Bytes.
 */
signed int GetBytesInSocketData( unsigned int Socket )
/*------------------------------------------------------------------------------------------------------------*/	
	{
		if ( Socket >= MAX_TCP_CONNECTIONS || TCP_sockettable[Socket].ConnectionState == SOCKET_NOT_USE ) return( -1 );
		
		return( Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) );
	}
	
	
/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Holt empfangende Daten aus den Socketpuffer.
 * \param	Socket		Die Socketnummer die zum versnden benutzt werden soll.
 * \param	bufferlen	Die größe des Puffer in den die Daten gespeichert werden sollen.
 * \param	buffer		Zeiger auf den Speicher wo die Daten hin kopiert werden soll.
 * \retval	Die Anzahl der kopierten Bytes.
 */
/*------------------------------------------------------------------------------------------------------------*/	
unsigned char GetByteFromSocketData( unsigned int Socket )
	{
		if ( Socket >= MAX_TCP_CONNECTIONS || TCP_sockettable[Socket].ConnectionState == SOCKET_NOT_USE ) return( 0 );
		if ( Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) == 0 ) return ( 0 );
		
		unsigned char Data;
		
		LockEthernet();
		
		Data = Get_Byte_from_FIFO ( TCP_sockettable[ Socket ].fifo );

		// Sendet ein Update wenn Buffer leer
		if ( Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) == 0 )
		{
			unsigned char * ackbuffer;
			ackbuffer = (unsigned char*) __builtin_alloca (( size_t ) ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT + TCP_HEADER_LENGHT );

			struct TCP_header *TCP_packet;		// TCP_struct anlegen
			TCP_packet = ( struct TCP_header *)&ackbuffer[ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT ];							

			MakeTCPheader( Socket, TCP_ACK_FLAG , 0 , MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) , ackbuffer );
		}
		
		FreeEthernet();

		return( Data );
	}

/*-----------------------------------------------------------------------------------------------------------*/
/*!\brief Baut eine TCP-Verbindung zu einer IP-Adresse auf.
 * \param	IP		Die IP-Adresse des Zielhost.
 * \param	Port	Ziel-Port des Zielhost.
 * \retval	Socket	Die Socketnummer der aufgebauten verbindung oder 0xffff im Fehlerfall.
 */
/*------------------------------------------------------------------------------------------------------------*/	
unsigned int Connect2IP( unsigned long IP, unsigned int Port )
	{
		unsigned int Socket;
		// hole einen freien SOCKET
		
		Socket = Getfreesocket();
		// Wenn kein SOCKET mehr frei, EXIT!
		if ( Socket == NO_SOCKET_USED ) return ( NO_SOCKET_USED );

		// Arp-request senden und schaun ob ip vorhanden
		if ( IS_ADDR_IN_MY_SUBNET( IP, Netmask ) )
		{
			if ( IS_BROADCAST_ADDR( IP, Netmask ) ) 
			{
				for( unsigned char i = 0 ; i < 6 ; i++ ) TCP_sockettable[ Socket ].MACadress[i] = 0xff;
			}
			else if ( GetIP2MAC( IP, &TCP_sockettable[ Socket ].MACadress ) == NO_ARP_ANSWER ) return ( NO_SOCKET_USED );
		}
		else if ( GetIP2MAC( Gateway , &TCP_sockettable[ Socket ].MACadress ) == NO_ARP_ANSWER ) return ( NO_SOCKET_USED );
		
		LockEthernet();
		// Socket reservieren
		TCP_sockettable[ Socket ].ConnectionState = SOCKET_SYNINIT;
		TCP_sockettable[ Socket ].DestinationPort =~ ss + ms;
		
		unsigned char * ethernetbuffer;
		ethernetbuffer = (unsigned char*) __builtin_alloca (( size_t ) ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT + TCP_HEADER_LENGHT );
	
		struct TCP_header *TCP_packet;		// TCP_struct anlegen
		TCP_packet = ( struct TCP_header *) ethernetbuffer[ETHERNET_HEADER_LENGTH + IP_HEADER_LENGHT ];
		
		// Register den SOCKET
		TCP_sockettable[ Socket ].SourcePort = ChangeEndian16bit( Port ) ;
		TCP_sockettable[ Socket ].SourceIP = IP;
		TCP_sockettable[ Socket ].SequenceNumber =~ 0x12345678;
		TCP_sockettable[ Socket ].AcknowledgeNumber = 0x0 ;
		TCP_sockettable[ Socket ].SendState = SOCKET_READY2SEND;
		TCP_sockettable[ Socket ].Timeoutcounter = 10;
		Flush_FIFO ( TCP_sockettable[ Socket ].fifo );
		TCP_sockettable[ Socket ].Windowsize = 0;
		TCP_sockettable[ Socket ].SendetBytes = 0;		

		MakeTCPheader( Socket, TCP_SYN_FLAG , 0 , MAX_RECIVEBUFFER_LENGHT - Get_Bytes_in_FIFO ( TCP_sockettable[ Socket ].fifo ) , ethernetbuffer );
	
		FreeEthernet();

		#ifdef _DEBUG_
			printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
			printf_P( PSTR("Verbindungaufbau angefordert [SYN]\r\n") );
		#endif

		unsigned char timer;
		
		timer = CLOCK_RegisterCoundowntimer();
		CLOCK_SetCountdownTimer( timer , CONNECTTIMEOUT, MSECOUND );

		while ( 1 )
		{
			LockEthernet();
			if ( (  TCP_sockettable[ Socket ].ConnectionState == SOCKET_READY ) && ( CLOCK_GetCountdownTimer( timer ) != 0 ) )
			{
				CLOCK_ReleaseCountdownTimer( timer );
				MakeTCPheader( Socket, TCP_ACK_FLAG, 0 , MAX_RECIVEBUFFER_LENGHT , ethernetbuffer );
				#ifdef _DEBUG_
					printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
					printf_P( PSTR("ACK für SYN empfangen.\r\n") );
				#endif
				FreeEthernet();
				return( Socket );
			}
			if ( CLOCK_GetCountdownTimer( timer ) == 0 ) 
			{
				#ifdef _DEBUG_
					printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
					printf_P( PSTR("SYN-Timeout. Verbindung wird geschlossen\r\n") );
				#endif
				TCP_sockettable[ Socket ].ConnectionState = SOCKET_NOT_USE ;
				CLOCK_ReleaseCountdownTimer( timer );
				FreeEthernet();
				return( NO_SOCKET_USED );
			}
			FreeEthernet();
		}
}
