/***************************************************************************
 *            ethernet.h
 *
 *  Sat Jun  3 14:57:38 2006
 *  Copyright  2006  Dirk Broßwick
 *  Email: sharandac@snafu.de
 ****************************************************************************/

#ifndef __ETHERNET_H__
	
	#define __ETHERNET_H__
	
	extern unsigned char mymac[6];
	extern unsigned long PacketCounter;
	extern unsigned long ByteCounter;
	
	void ethernetloop( void );
	unsigned int getEthernetframe( unsigned int maxlen, unsigned char *buffer);
	void MakeETHheader( unsigned char * MACadress , unsigned char * buffer );
	void sendEthernetframe( unsigned int packet_lenght, unsigned char *buffer);
	void EthernetInit( void );
	void LockEthernet( void );
	void FreeEthernet( void );
	void alive( void );
	
	#define ETHERNET_MIN_PACKET_LENGTH	0x3C
	#define ETHERNET_HEADER_LENGTH		14

	#define interrupt					4

	struct ETH_header {
		unsigned char ETH_destMac[6];	
		unsigned char ETH_sourceMac[6];
		unsigned int ETH_typefield;
	};

#endif
