/*!\file ip.c \brief Stellt die IP Funktionalitaet bereit*/
//***************************************************************************
//*            ip.c
//*
//*  Mon Jul 31 21:46:47 2007
//*  Copyright  2007  Dirk Broßwick
//*  Email
///	\ingroup network
///	\defgroup IP Der IP Stack fuer Mikrocontroller (ip.c)
///	\code #include "arp.h" \endcode
///	\code #include "ethernet.h" \endcode
///	\code #include "ip.h" \endcode
///	\code #include "icmp.h" \endcode
///	\code #include "udp.h" \endcode
///	\code #include "tcp.h" \endcode
///	\par Uebersicht
///		Der IP-Stack für Mikrocontroller. Behandelt die Verarbeitung der eingehenden IP
/// Packete und reicht diese weiter an UDP, TCP und ICMP.
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

#include <stdlib.h>
#include <stdio.h>

#include "arp.h"
#include "ip.h"
#include "icmp.h"
#include "ethernet.h"
#include "udp.h"
#include "tcp.h"

#include "system/math/math.h"
#include "system/math/checksum.h"

#include <avr/io.h>

#ifndef MY_IP_ADDRESS
unsigned long myIP = 0x6102a8c0;
#else
unsigned long myIP = MY_IP_ADDRESS;
#endif

#ifndef MY_IP_NETWORK_MASK
unsigned long Netmask = 0x00ffffff;
#else
unsigned long Netmask = MY_IP_NETWORK_MASK;
#endif

unsigned long Gateway = 0x0102a8c0;

unsigned long DNSserver = 0x0102a8c0;

/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Verarbeitet ein eingehendes Packet und reicht es an das entsprechende Protokoll weiter
 * \param	packet_lenght	Die größe des IP-Packet mit ethernetheader
 * \param	buffer			Zeiger auf das komplette packet wo es im Speicher liegt
 * \return  kein
 */
/* -----------------------------------------------------------------------------------------------------------*/
void ip( unsigned int packet_lenght , unsigned char *buffer )
	{
		struct ETH_header *ETH_packet; 		// ETH_struct anlegen
		ETH_packet = (struct ETH_header *)&buffer[0];
		struct IP_header *IP_packet;		// IP_struct anlegen
		IP_packet = ( struct IP_header *)&buffer[ETHERNET_HEADER_LENGTH];
											// checke mal ob dat Ã¼berhaupt fÃ¼r uns ist
		// if ( IP_packet->IP_DestinationIP != myIP || IP_packet->IP_DestinationIP != 0xffffffff ) return;
			
		switch ( IP_packet->IP_Protocol )
			{
				case 0x01:			if ( IP_packet->IP_DestinationIP != myIP ) return;
									icmp( packet_lenght , buffer);
									break;
				case 0x06:			if ( IP_packet->IP_DestinationIP != myIP ) return;
									tcp( packet_lenght , buffer );
									break;
				case 0x11:			udp( packet_lenght , buffer );
									break;
			}
	}

/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Erezugt ein IP-Packet.
 * \param	SourceIP		Die Quelladresse an die das PAcket gesendet werden soll
 * \param	Protocoll		IP-Protokollnummer (tcp, udp, icmp , ... )
 * \param	Datalenght		Länge des Ethernetbuffers.
 * \param	ethernetbuffer	Zeiger auf das komplette packet wo es im Speicher liegt
 * \return  kein
 */
/* -----------------------------------------------------------------------------------------------------------*/
void MakeIPheader( unsigned long SourceIP, unsigned char Protocoll, unsigned int Datalenght , unsigned char *ethernetbuffer )
	{
		struct ETH_header *ETH_packet; 		// ETH_struct anlegen
		ETH_packet = (struct ETH_header *) ethernetbuffer;
		struct IP_header *IP_packet;		// IP_struct anlegen
		IP_packet = ( struct IP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH];
		struct TCP_header *TCP_packet;		// TCP_struct anlegen
		TCP_packet = ( struct TCP_header *) &ethernetbuffer[ETHERNET_HEADER_LENGTH + ((IP_packet->IP_Version_Headerlen & 0x0f) * 4 )];

		IP_packet->IP_Version_Headerlen = 0x45;
		IP_packet->IP_TOS = 0x0;
		IP_packet->IP_Totallenght = ChangeEndian16bit ( IP_HEADER_LENGHT + Datalenght );
		IP_packet->IP_Identification = 0x1DAC;
		IP_packet->IP_Flags = 0x40;
		IP_packet->IP_Fragmentoffset = 0x0;
		IP_packet->IP_TTL = 64 ;		
		IP_packet->IP_Protocol = Protocoll;
		IP_packet->IP_Headerchecksum = 0x0;
		IP_packet->IP_SourceIP = myIP;
		IP_packet->IP_DestinationIP = SourceIP;
		IP_packet->IP_Headerchecksum = ChangeEndian16bit( Checksum_16( &ethernetbuffer[ETHERNET_HEADER_LENGTH] ,(IP_packet->IP_Version_Headerlen & 0x0f) * 4 ) );

		return;
	}

/* -----------------------------------------------------------------------------------------------------------*/
/*!\brief Wandelt einen String in ein IP die als unsigned long zurück gegeben wird.
 * \param	buffer		Pointer auf den String der umgewandelt werden soll.
 * \return  	IP		Die IP oder 0 wenn keine gültiger String da war.
 */
/* -----------------------------------------------------------------------------------------------------------*/
unsigned long strtoip( unsigned char * buffer )
{
	unsigned int i , len , y = 0;
	unsigned long IP = 0;
	
	len = strlen( buffer );
	// suchen ob string nur gültige zeichen enthält
	for( i = 0 ; i < len ; i++ )
		if ( !( buffer[i] >= '0' || buffer[i] == '.' || buffer[i] <= '9' ) )
		{
			return( 0 );
		}
	
	// string zerlegen
	for( i = 0 ; i < len && y < 4 ; i++ )
	{
		IP = IP<<8;
		// zeichen in zahl wandeln
		IP |= atoi( &buffer[i] );
		
		// nach dem nächsten punkt suchen
		for ( ; i < len ; i++ )
		{
			if ( buffer[i] == '.' )
			{
				y++;
				break;
			}
		}
	}	

	// wenn schon 4 zahlen gewandelt, verlassen
	if( y == 3 )
		return( ChangeEndian32bit(IP) );
	else
		return( 0 );
}
//@}
