/*! \file fifo.h \brief Stellt die FIFO Funkionalitaet bereit */
//***************************************************************************
//*            fifo.h
//*
//*  Thu Apr  3 23:01:42 2008
//*  Copyright  2008 Dirk Broßwick
//*  Email: sharandac@snafu.de
//****************************************************************************/
///	\ingroup system
///	\defgroup FIFO Die FIFO-Puffer funktionalität (fifo.c)
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
#ifndef _FIFO_H
	#define FIFO_H

	unsigned int Get_FIFO( unsigned char * buffer, unsigned int bufferlenght );
	unsigned char Free_FIFO ( unsigned int FIFO );
	unsigned int Get_Bytes_in_FIFO( unsigned int FIFO );
	unsigned char Put_Byte_in_FIFO( unsigned int FIFO, unsigned char Byte );
	unsigned int Get_Block_from_FIFO( unsigned int FIFO, unsigned int bufferlenght, unsigned char * buffer );
	unsigned int Put_Block_in_FIFO( unsigned int FIFO, unsigned int bufferlenght, unsigned char * buffer );
	unsigned char Get_Byte_from_FIFO( unsigned int FIFO );
	unsigned int Flush_FIFO( unsigned int FIFO );
	unsigned int Get_FIFOsize( unsigned int FIFO );
	unsigned int Get_FIFOrestsize( unsigned int FIFO );
	unsigned int Get_FIFO_to_FIFO( unsigned int Src_FIFO, unsigned int bufferlenght, unsigned int Dest_FIFO );

	#define	MAX_FIFO_BUFFERS	16

	#define	FIFO_ERROR		0xffff
	#define FIFO_OK			0x00
	#define UNLOCK			0xff
	#define	LOCK			0x00
	#define NULL			0

	struct FIFO {
		unsigned char	* buffer;
		unsigned int 	bufferlenght;
		unsigned int	writepointer;
		unsigned int	readpointer;
		unsigned int	byteinbuffer;
		unsigned long	Blockcopyhit;
		unsigned long	Bytecopyhit;
		unsigned char	lock;
	};

#endif /* FIFO_H */
//@}
