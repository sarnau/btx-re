/*! \mainpage Willkommen zum Mikrowebserver Projekt
 *
 * \section intro_sec Übersicht
 *
 * Zu Beginn der Aufstellung von Anforderungen an das uns gedanklich vorschwebende Projekt 
 * war eine Präzisierung der Funktionen des Microwebservers notwendig. Hier ging es vor allem 
 * um die vielseitige Nutzung des Gerätes, sowohl für Schüler, Studenten und Anfänger auf dem 
 * Gebiet der Programmierung als auch für fortgeschrittene Programmierer, Hobbyelektroniker 
 * und Dozenten in entsprechenden Unterrichtsfächern oder Kursen. Es sollte eine einfach zu 
 * bedienende und recht anschlussfreudige Entwicklungsumgebung für Schulungszwecke geschaffen werden, 
 * die aber auch im Hausgebrauch oder in der Industrie sinnvoll eingesetzt werden kann. 
 * Uns war aber auch klar, dass es schon eine Vielzahl von Mikrocontrollersystemen gab. Deshalb 
 * konzipierten wir ein System, welches den Ansprüchen an ein vollwertige Embbeded System gerecht wird, 
 * aber den Laien nicht mit Komplexität erschlägt. Folgende Funktionen sollte unser System vorweisen:
 *<br>
 *<br>
 *   * leicht zu programmierender Microcontroller <br>
 *   * kein Ausbau des Controllers aus der Hardware zum Programmieren (ISP)<br>
 *   * Programmieren in Assembler und C/C++ möglich<br>
 *   * vielfältige Schnittstellenverfügbarkeit wie RS232, SPI, USART, I2C(TWI), Netzwerk und Analogeingänge<br>
 *   * Debuggmodus (Industristandart JTAG) für Schulungs- und Entwicklungszwecke muss möglich sein<br>
 *   * hohe Taktrate des Systems (>10MHz)<br>
 *   * ausreichend Speicher (256kB Flash, 128kB RAM)<br>
 *   * kompakte Abmessungen der Platine<br>
 *   * wenige Bauelemente, geringe Beschaltung der IC`s<br>
 *   * großer Spannungsversorgungsbereich<br>
 *   * kostengünstige und leicht verfügbare Programmiersoftware und Programmiergeräte <br>
 *<br>
 * Dieses Projekt hat das Ziel, eine Entwicklungsumgebung für einen ATmega2561 mit einem
 * Netzwerkinterface zu erstellen, das mit Hilfe des ENC28j60 von Microchip realisiert wird. Ziel ist
 * es, eine Plattform für Entwickler zu bieten, der die wichtigsten funktionen zur verfügung 
 * stellt, so dass der Entwickler sich auf die eigentliche Applikation konzentrieren kann.
 */

/****************************************************************************
 *            main.c
 *
 *  Mon May 29 20:26:59 2007
 *  Copyright  2007  Dirk Broßwick
 *  Email sharandac(at)snafu.de
 ****************************************************************************/
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
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include <avr/version.h>

#include "hardware/memory/xram.h"
#include "hardware/network/enc28j60.h"
#include "hardware/led/led_core.h"
#include "hardware/uart/uart.h"
#include "hardware/vs10xx/vs10xx.h"

#include "system/clock/clock.h"
#include "system/net/ethernet.h"
#include "system/net/ip.h"
#include "system/net/arp.h"
#include "system/net/udp.h"
#include "system/net/tcp.h"
#include "system/net/dhcpc.h"
#include "system/net/ntp.h"
#include "system/stdout/stdout.h"

#include "apps/telnet/telnet.h"
#include "apps/httpd/httpd2.h"
#include "apps/udp-echo/udp-echo.h"
#include "apps/mp3-streamingclient/mp3-streaming.h"

#include "apps/btx/main.h"

// Timerinterrupt mit abstand 1 sekunde, dabei wird die LED 0 getoggelt
void blinkinglights( void )
{
	LED_toggle(0);
}

void main( void ) 
{
	// Interrupts freigeben
	sei();

	// Union für IP
	union IP_ADDRESS IP;
	
	// struct für die Zeit anlegen
	struct TIME Time;

	// RS232 starten und printf auf RS232 verbiegen
	UART_init();

	STDOUT_INIT ();
	STDOUT_Set_RS232();
	CLOCK_init();

	printf_P( PSTR("%c[2J"),27 ); // Bildschirm löschen in Terminalprogram
	printf_P( PSTR("New_microwebserver build on AVR-libc version: " __AVR_LIBC_VERSION_STRING__ "/" __AVR_LIBC_DATE_STRING__ " $Id: main.c 28 2008-06-22 20:28:10Z sharan $ $Revision: 67 $\r\n"));
	printf_P( PSTR("UART initialisiert\r\n"));

	// LED_core starten
	LED_init();
	printf_P( PSTR("LED_core initialisiert\r\n"));

	// Clock starten
	CLOCK_init();
	printf_P( PSTR("CLOCK initialisiert\r\n"));

	// Callbackfunktion für die blinkende LED eintragen
	CLOCK_RegisterCallbackFunction ( blinkinglights, SECOUND );
	
	// VS10xx starten
	printf_P( PSTR("VS10xx initialisieren, "));
	if ( VS10xx_INIT () == RESET_OK )
		printf_P( PSTR("Clockspeed = %d.%03dMHz\r\n"), (VS10xx_read( VS10xx_Register_CLOCKF ) * 2)/1000 ,  (VS10xx_read( VS10xx_Register_CLOCKF ) * 2)%1000 );
	else
		printf_P( PSTR("failed") );
		
	// Ethernet starten
	EthernetInit();
	printf_P( PSTR("ENC28j60 initialisiert ( HW-Add: %02x:%02x:%02x:%02x:%02x:%02x )\r\n"),ENC28J60_MAC0,ENC28J60_MAC1,ENC28J60_MAC2,ENC28J60_MAC3,ENC28J60_MAC4,ENC28J60_MAC5);
	
	// ARP starten
	ARP_INIT ();
	printf_P( PSTR("-+-> ARP initialisiert\r\n"));
	
	// UDP starten
	UDP_init();
	printf_P( PSTR(" |-> UDP initialisiert\r\n"));
	
	// tcp starten
	tcp_init();
	printf_P( PSTR(" |-> TCP initialisiert\r\n"));

	// DHCP-Config holen
	printf_P( PSTR(" |-> Versuche DHCP-Config zu holen. "));
	if ( !DHCP_GetConfig () ) printf_P( PSTR("DHCP-Config geholt\r\n"));
	else printf_P( PSTR("DHCP-Config Fehlgeschlagen\r\n"));

	IP.IP = myIP;
	printf_P( PSTR(" |   IP     : %d.%d.%d.%d\r\n"), IP.IPbyte[0],IP.IPbyte[1],IP.IPbyte[2],IP.IPbyte[3]);
	IP.IP = Netmask;
	printf_P( PSTR(" |   Netmask: %d.%d.%d.%d\r\n"), IP.IPbyte[0],IP.IPbyte[1],IP.IPbyte[2],IP.IPbyte[3]);
	IP.IP = Gateway;
	printf_P( PSTR(" |   Gateway: %d.%d.%d.%d\r\n"), IP.IPbyte[0],IP.IPbyte[1],IP.IPbyte[2],IP.IPbyte[3]);
	IP.IP = DNSserver;
	printf_P( PSTR(" |   DNS    : %d.%d.%d.%d\r\n"), IP.IPbyte[0],IP.IPbyte[1],IP.IPbyte[2],IP.IPbyte[3]);

	// Uhr einstellen
	if( NTP_GetTime( IPDOT( 130l,133l,1l,10l ) , 0 ) == NTP_OK )
	{
		CLOCK_GetTime ( &Time );
		printf_P( PSTR(" |-> NTP-Server Zeit aktualisieren. Zeit: %02d:%02d:%02d.%02d\r\n"),Time.hh,Time.mm,Time.ss,Time.ms);
	}
	else
		printf_P( PSTR(" |-> NTP-Server Zeit aktualisieren fehlgeschlagen\r\n"));
	
	LED_on(2);
	


	// Dienste starten

	/* Note: We do not need that here, so we better switch it off.
	telnet_init();
	httpd_init();
	UDP_echo_init();
	mp3client_init();
	*/

	applicationBtxInit();
	
	getchar();

	// die mainloop, hier alles eintragen, was sich dienst nennt
	while(1)
	{	
		/* Note: We do not need that here, so we better switch it off.
		telnet_thread();
		httpd_thread();
		UDP_echo();
		mp3client_thread();
		*/
	}
}
