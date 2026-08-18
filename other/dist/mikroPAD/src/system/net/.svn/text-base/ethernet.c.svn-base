/***************************************************************************
 *            ethernet.c
 *
 *  Sat Jun  3 17:25:42 2006
 *  Copyright  2006  Dirk Broßwick
 *  Email: sharandac@snafu.de
 ****************************************************************************/
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <stdio.h>
#include "hardware/network/enc28j60.h"
#include "hardware/ext_int/ext_int.h"
#include "hardware/led/led_core.h"
#include "system/clock/clock.h"
#include "ethernet.h"
#include "arp.h"

// #define _DEBUG_

#ifdef _DEBUG_
	#include <stdio.h>
	#include "hardware/uart/uart.h"
#endif

unsigned char mymac[6] = { ENC28J60_MAC0,ENC28J60_MAC1,ENC28J60_MAC2,ENC28J60_MAC3,ENC28J60_MAC4,ENC28J60_MAC5 };
unsigned long PacketCounter;
unsigned long ByteCounter;

/*
 -----------------------------------------------------------------------------------------------------------
   Die Routine die die Packete nacheinander abarbeitet
------------------------------------------------------------------------------------------------------------*/

void ethernet(void)
	{
		unsigned int packet_lenght;
		
		unsigned char * ethernetbuffer;
		ethernetbuffer = (unsigned char*) __builtin_alloca (( size_t ) MAX_FRAMELEN );
		
		// hole ein Frame
		packet_lenght = getEthernetframe( MAX_FRAMELEN, ethernetbuffer);
		// wenn Frame vorhanden packet_lenght != 0
		// arbeite so lange die Frames ab bis keine mehr da sind
		
		while ( packet_lenght != 0 )
			{
				#ifdef _DEBUG_
					printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
					printf_P( PSTR("Packet empfangen, laenge = %d byte ") , packet_lenght );
				#endif
				PacketCounter++;
				ByteCounter = ByteCounter + packet_lenght;
				struct ETH_header *ETH_packet; 		//ETH_struc anlegen
				ETH_packet = (struct ETH_header *) ethernetbuffer; 
				switch ( ETH_packet->ETH_typefield ) // welcher type ist gesetzt 
					{
					case 0x0608:		
										#ifdef _DEBUG_
											printf_P( PSTR("-->> ARP\r\n") );
										#endif
										arp( packet_lenght , ethernetbuffer );
										break;
					case 0x0008:		
										#ifdef _DEBUG_
											printf_P( PSTR("-->> IP\r\n") );										
										#endif
										ip( packet_lenght , ethernetbuffer );
										break;
					}

				// checke ob noch ein packet im ENC28j60 liegt
				packet_lenght = getEthernetframe( MAX_FRAMELEN, ethernetbuffer);
			}	
		return;
	}

/* -----------------------------------------------------------------------------------------------------------
Holt ein Ethernetframe
------------------------------------------------------------------------------------------------------------*/
unsigned int getEthernetframe( unsigned int maxlen, unsigned char *ethernetbuffer)
	{
		return( enc28j60PacketReceive( maxlen , ethernetbuffer) );
	}
	
/* -----------------------------------------------------------------------------------------------------------
Sendet ein Ethernetframe
------------------------------------------------------------------------------------------------------------*/
void sendEthernetframe( unsigned int packet_lenght, unsigned char *ethernetbuffer)
	{
		PacketCounter++;
		ByteCounter = ByteCounter + packet_lenght;
 		enc28j60PacketSend( packet_lenght, ethernetbuffer );
		
		#ifdef _DEBUG_
			printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
			printf_P( PSTR("Packet gesendet, laenge = %d byte\r\n") , packet_lenght );
		#endif
	}
	
/* -----------------------------------------------------------------------------------------------------------
Erstellt den richtigen Ethernetheader zur passenden Verbindung die gerade mit TCP_socket gewählt ist
------------------------------------------------------------------------------------------------------------*/
void MakeETHheader( unsigned char * MACadress , unsigned char * ethernetbuffer )
	{
		struct ETH_header *ETH_packet; 		// ETH_struct anlegen
		ETH_packet = (struct ETH_header *) ethernetbuffer;

		unsigned int i;			

		ETH_packet->ETH_typefield = 0x0008;
		
		for ( i = 0 ; i < 6 ; i++ ) 
		{
			ETH_packet->ETH_sourceMac[i] = mymac[i];			
			ETH_packet->ETH_destMac[i] = MACadress[i];
		}
		return;		
	}

/* -----------------------------------------------------------------------------------------------------------
Erstellt den richtigen Ethernetheader zur passenden Verbindung die gerade mit TCP_socket gewählt ist
------------------------------------------------------------------------------------------------------------*/
void LockEthernet( void )
{
	block_extern_interrupt ( interrupt );
	#ifdef _DEBUG_
		printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
		printf_P( PSTR("IRQ Emfpang abgeschaltet\r\n"));
	#endif
}

/* -----------------------------------------------------------------------------------------------------------
Erstellt den richtigen Ethernetheader zur passenden Verbindung die gerade mit TCP_socket gewählt ist
------------------------------------------------------------------------------------------------------------*/
void FreeEthernet( void )
{
	free_extern_interrupt ( interrupt );
	#ifdef _DEBUG_
		printf_P( PSTR("%02d:%02d:%02d.%02d: ") ,hh,mm,ss,ms);
		printf_P( PSTR("IRQ Emfpang eingeschaltet\r\n"));
	#endif
}

/* -----------------------------------------------------------------------------------------------------------
führt den Init durch
------------------------------------------------------------------------------------------------------------*/
void EthernetInit( void )
{
		// ENC Initialisieren //
		enc28j60Init();

		// Alle Packet lesen und ins leere laufen lassen damit ein definierter zustand herrscht
		unsigned char * ethernetbuffer;
		ethernetbuffer = (unsigned char*) __builtin_alloca (( size_t ) MAX_FRAMELEN );
		while ( getEthernetframe( MAX_FRAMELEN, ethernetbuffer) != 0 ) { };
		
		init_extern_interrupt ( interrupt , SENSE_LOW , ethernet );
		// gibt Ethernet frei
		FreeEthernet();
}
