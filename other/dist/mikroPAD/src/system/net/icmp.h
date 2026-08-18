/***************************************************************************
 *            icmp.h
 *
 *  Sat Jun  3 18:53:56 2006
 *  Copyright  2006  Dirk Broßwick
 *  Email: sharandac@snafu.de
 ****************************************************************************/

#ifndef __ICMP_H__
	
	#define __ICMP_H__

	void icmp( unsigned int packet_lenght, unsigned char *buffer);

	struct ICMP_header{
		unsigned char ICMP_type;
		unsigned char ICMP_code;
		unsigned char ICMP_checksumByteOne;
		unsigned char ICMP_checksumByteTwo;
	};

#endif
