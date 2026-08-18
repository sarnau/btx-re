/*! \file tcp.h \brief Stellt die TCP/IP Funkionalitaet bereit */
//***************************************************************************
// *            tcp.h
// *
// *  Sat Jun  3 23:01:49 2006
// *  Copyright  2006  Dirk Broßwick
// *  Email
//****************************************************************************/
#ifndef __TCP_H__
	#define __TCP_H__

	#include <avr/pgmspace.h>

	extern unsigned int TXErrorCounter;
	extern unsigned int RXErrorCounter;
	extern unsigned int RXErrorUnsort;
	extern unsigned int RXErrorOldSeq;
	void tcp_init( void );

	void tcp( unsigned int packet_lenght, unsigned char *buffer);
	void TCPTimeOutHandler( void );

	unsigned int Getfreesocket(void);
	unsigned int GetSocket( unsigned char * buffer);
	unsigned int RegisterSocket( unsigned char *buffer);
	
	unsigned int CopyTCPdata2socketbuffer( unsigned int Socket, unsigned int Datalenght , unsigned char *buffer);
	void MakeTCPheader( unsigned int Socket, unsigned char TCP_flags, unsigned int Datalenght, unsigned int Windowsize, unsigned char *buffer );
	
	unsigned int RegisterTCPPort( unsigned int Port );
	void UnRegisterTCPPort( unsigned int Port );
	unsigned int CheckPortInList( unsigned int Port );
	unsigned int CheckPortRequest( unsigned int Port );
	unsigned int CheckSocketState( unsigned int Socket );
	void CloseTCPSocket( unsigned int Socket);
	unsigned int Connect2IP( unsigned long IP, unsigned int Port );
	
	unsigned int GetSocketData( unsigned int Socket , unsigned int bufferlen, unsigned char * buffer);
	unsigned int GetSocketNextLine( unsigned int Socket , unsigned int bufferlen, unsigned char * buffer);
	unsigned int PutSocketData( unsigned int Socket, unsigned int Datalenght, unsigned char * Sendbuffer );
	unsigned int PutSocketData_P( unsigned int Socket, unsigned int Datalenght, const prog_char * Sendbuffer );
	unsigned int PutSocketData_RPE( unsigned int Socket, unsigned int Datalenght, unsigned char * Sendbuffer, unsigned char Mode );
	int SendData_RPE( unsigned int Socket, unsigned int Datalenght, unsigned char * Sendbuffer, unsigned char Mode, int RetransmissionCounter );
	signed int FlushSocketData( unsigned int Socket );
	unsigned int GetSocketDataToFIFO( unsigned int Socket , unsigned int FIFO, unsigned int bufferlen );

	unsigned char GetByteFromSocketData( unsigned int Socket );
	signed int GetBytesInSocketData( unsigned int Socket );
	
	#define	RAM		0
	#define FLASH	1
	#define EEPROM	2
	
	// Struktur für den Pseudoheader, dieser wird bei der Berechnung der TCP-Checksumme benötigt und wird vor dem TCP-header gesetzt.
	// Jetzt wird zur berechnung der Checksumme im TCP-header die Checksumme auf 0 gesetzt.
	// Danach wird die Checksumme für den Pseudoheader, TCP-header und dem Nutzdatensgment ( TCP_Segment ) berechnet.
	// wenn die Checksumme errechnet ist kann wird diese in die TCP-header geschrieben.
	// wenn alles okay kann der empfängr auf dem selben weg rückwerts die checksumme kontrollieren und bekommt als ergenis 0 zurück, weil
	// das ergebnis der Checkumme ja auch im TCP_header mit drinne steht.
	
	#define IP_PSEUDOHEADER_LENGHT 12
	
	struct IP_Pseudoheader {
			volatile unsigned long IP_SourceIP;			// Quell IP
			volatile unsigned long IP_DestinationIP;		// Ziel IP
			volatile unsigned char IP_ZERO;				// muss 0 sein
			volatile unsigned char IP_Protokoll;			// bei TCP immer 6
			volatile unsigned int IP_TCP_lenght;			// länge des Nutzdaten Segmentes ( TCP_Segment )
	};
	
	#define MAX_LISTEN_PORTS 4					// Anzahl der Port auf den gelauscht wird
	#define TCP_Port_not_use 0
	
	struct TCP_PORT {							// Struktur für Ports
			volatile unsigned int TCP_Port;
	};
	
	// Struktur für den Connection SOocket. Hier werden die Sockets für die TCP-Verbindungen angelegt und den wichtige Werte gespeichert.
	// damit kann man jede verbindung genau nachverfolgen und auch halten.
	
	#define MAX_TCP_CONNECTIONS			3	// maximale gleichzeitige verbindungen
	#define MAX_TCP_RETRANSMISSIONS		5
	#define MAX_RECIVEBUFFER_LENGHT 	1024*4
	#define CLOSETIMEOUT				200			// angabe in 1/100 sec
	#define RETRANSMISSIONTIMEOUT		200			// angabe in 1/100 sec
														// der timeout für retransmission
	#define CONNECTTIMEOUT				500				// angabe in 1/100 sec
	#define TimeOutCounter 				20				// angabe in 1 sec
														// timeout für das socket wie lange keine daten mehr ausgetauscht wurden

	struct TCP_SOCKET {
			volatile unsigned char ConnectionState;				// 1 Byte
			volatile unsigned char SendState;					// 1 Byte
			volatile unsigned int SourcePort;					// 2 byte
			volatile unsigned int DestinationPort;				// 2 Byte
			volatile unsigned long SourceIP;					// 4 Byte	
			volatile unsigned long SequenceNumber;				// 4 Byte
			volatile unsigned long AcknowledgeNumber;			// 4 Byte
			volatile unsigned int Windowsize;					// 2 Byte
			volatile unsigned long SendetBytes;					// 2 Byte
			volatile unsigned int Timeoutcounter;				// 2 Byte
			volatile unsigned char MACadress[6];				// 6 Byte
			volatile unsigned int MSS;							// 2 Byte
			volatile unsigned int fifo;
			volatile unsigned int old_fifo;
			volatile unsigned char Recivebuffer[ MAX_RECIVEBUFFER_LENGHT ];
	};
	
	// definitionen für die ConnectionState
	
	#define SOCKET_NOT_USE		0x00			// SOCKET ist Frei	
	#define SOCKET_SYNINIT		0x01			// SOCKET SYN init
	#define SOCKET_WAIT2SYNACK	0x04			// SOCKET hat SYN & ACK gesendet und wartet auf ACK
	#define SOCKET_SYNACK_OK	0x05			// SOCKET hat SYN+ACK nachen einen SYN empfangen ( fuer berbindungsaufbau )
	#define SOCKET_READY2USE	0x0F			// SYN erfolgreich und Socket bereit zur übernahme durch den Anwender
	#define SOCKET_READY		0x10			// Socket ist Bereit zur Benutzung
	#define SOCKET_WAIT2FINACK	0x81			// SOCKET wartet auch das FIN + ACK nach einem FIN + ACK
	#define SOCKET_WAIT2FIN		0x82			// SOCKET wartet auch das ACK nach einem FIN + ACK
	#define SOCKET_RESET		0x83			// SOCKET empfängt RST ( Reset )
	#define SOCKET_CLOSE		0xff			// SOCKET geschlossen

    #define NO_SOCKET_USED 		0xffff           // Rueckgabewert
	
	// definitionen für die SendState
	
	#define SOCKET_READY2SEND	0x00			// Socket bereit zum senden
	#define SOCKET_DATASENDED	0x80			// Socket hat Daten gesendet aber Window ist nicht voll und wartet auf ACK für Daten
	#define SOCKET_BUSY			0xff			// Socket hat Daten gesendet und warten auf ACK für Daten
	
	// der TCP-header :-)
	
	#define TCP_HEADER_LENGHT 	24
	#define MAX_TCP_Datalenght 	1452

//	#define MAX_TCP_Datalenght 	1460
	struct TCP_UNSORT {
			volatile unsigned int lenght;
			volatile unsigned int socket;
			volatile unsigned long Sequencenumber;
			volatile unsigned char Recivebuffer[ MAX_TCP_Datalenght ];
	};

	struct TCP_header {
			volatile unsigned int TCP_SourcePort;
			volatile unsigned int TCP_DestinationPort;
			volatile unsigned long TCP_SequenceNumber;
			volatile unsigned long TCP_AcknowledgeNumber;
			volatile unsigned char TCP_DataOffset;
			volatile unsigned char TCP_ControllFlags;
			volatile unsigned int TCP_Window;
			volatile unsigned int TCP_Checksum;
			volatile unsigned int TCP_UrgentPointer;
			volatile unsigned char TCP_Options[4];
	};
			
	// die einzelnen Controllflags des TCP-headers
	
	#define  TCP_NON_FLAG			0x00		// kein Flag gesetzt
	#define  TCP_FIN_FLAG			0x01		// Verbindung beenden gesenden/empfangen
	#define  TCP_SYN_FLAG			0x02		// Verbindung aufbauen gesenden/empfangen
	#define  TCP_RST_FLAG			0x04		// Reset gesenden/empfangen
	#define  TCP_PSH_FLAG			0x08		// Push gesendet/empfangen
	#define  TCP_ACK_FLAG			0x10        // Acknowledge gesendet/empfangen
	#define  TCP_URG_FLAG			0X20		// Urgentflag für dringlichkeit gesetzt
	
#endif

