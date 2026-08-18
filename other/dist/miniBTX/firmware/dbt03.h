/*
   ####################################################################################
   #                                                                                  #
   #                        Bildschirmtricks Firmware V2.0.0                          #
   #                              dbt03 emulator layer                                #
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
#ifndef DBT03_H
#define DBT03_H

/*  

==OVERVIEW==

 Note: Every BTX Terminal was connected to a propritary modem that that could only dial the number
       of the BTX-Mainframe (PAD). The microcontroller in the modem also conained a secret that was 
       necessary to associate with the BTX-Mainfraime. The Modem were sealed and opening was a 
       punishable offence.

               2               7=START (Power modem up and initaiate connection)
               o               5=ED (Data receive and status report, 1200 baud, 8n1)
        4   o     o   5	       6=SD (Data send port, 75 baud)
        1  o       o  3        2=E (GND)
        6   o     o   7
               ^
	 View at the 
        terminal/modem
             side

 The DBT03 was a V.23 modem from the BUNDESPOST (a telecommunications service in germany, now history) 
 to dial into the BTX (=BILDSCHIRMTEXT) system. This modem was switched on by pulling the the START
 line to high. Then the modem dials automaticly and does the BTX negotiation. Then the modem reports
 the connection status in a very easy way: It just takes the dialtones and hands them through as normal
 TTL signals. 

            ________________________________________________________________________________________
 START  ___|

                                                                           ___ Start bits ____
                                                                          |     Stop bit ___  |
                                                                          V                 | V
            _______________        ______________       ______            _   _ _ _   _   _ V _
 ED     ___|               ||||||||              |||||||      ||||||_____| |_|     |_| |_| |_|.|____  And so on....

           <---------------><-----><------------><-----><----><----><----><---Serial data at 1200 Baud-->
                T1            T2         T3         T4    T5    T6    T7
                          (dialtone)  (dialing)   (ring)


	T1: Time between modem activision and hook-off, typicaly 3s.
            Configurable with DBT03_HOOK_OFF_DELAY macro.

        T2: Dialtone appeareance time, a dialtone cycle is about 2,35ms long.
	    Number of cyceles is configurable with DBT03_DAILTONE_CYCLES

	T3: Dialin delay time, time that goes by whil a real modem dials the BTX-PAD number (019 or 01910)
            Typicaly it takes 4 seconds to dial that number. The dialin delay time is configurable 
            with DBT03_DIALIN_DELAY

	T4: Ringtone appearance time, nealy the same as T2 but in this case this tone stands for 
            a successful dial process and tells the BTX-Terminal that the pone of the BTX-PAD is ringing.
            In this emulation we assume that the BTX-PAD is fast and hooks off with the first ringtone.
	    The ringtone-appeareance-time is configurable with DBT03_RINGTONE_CYCLES

	T5: Post ringtone delay, time between the dialtone and the carrier signal. This time can be configured
            with DBT03_POST_RINGTONE_DELAY

	T6: Carrier tone appereance time: The original DBT-03 modem needed about 1,6 - 1,7 ms time to detect the
            carrier signal. As long as the carrier signal is not recognized the modem passes the signal through.
            That is why the carrier signal is also a part of this simulation.

	T7: To finish the dialin procedure the modem pulls the ED line to low level and waits some time.
            This time is configurable with DBT03_POST_DIALIN_DELAY


	Note: The configured values are based on some old documentation i found on the net as well as on
              experiments with various types of terminals. if this does not work on your setup or if you 
              unhappy with the long dialin procedure you can try to tune the timing.


==CONFIGURATION== */

/* Timing configuration for serial data transmission */
#define DBT03_TX_BAUDRATE 82				/* (x10us) Should be 1200 baud/833,333us bit legth */
#define DBT03_RX_BAUDRATE 133				/* (x100us) Should be 75 baud/13,333ms bit legth */

/* Timing configuration for dialin simulation */
#define DBT03_HOOK_OFF_DELAY 30				/* (x100ms) Hook off delay time, typical 3,0s */
#define DBT03_DAILTONE_CYCLES 800			/* (x2,35ms) Dialtone appearance time */
#define DBT03_DIALIN_DELAY 40				/* (x100ms) Dailin delay time, maximum 4,3s */
#define DBT03_RINGTONE_CYCLES 200			/* (x2,35ms) Ringtone appearance time */
#define DBT03_POST_RINGTONE_DELAY 5			/* (x100ms) Time between ringtone and carrier tone */
#define DBT03_CARRIERTONE_CYCLES 2080			/* (x760us) Carrier tone appereance time */
#define DBT03_POST_DIALIN_DELAY 5			/* (x100ms) Time between successful dialin notification and transmission start */


/* ==Function set== 
   This proviedes a set of basic functions that emulate the typical dbt03 signals */

void systemDbt03Init(void);				/* Initalize dbt03 emulation layer */
void systemDbt03Transmit(uint8_t character);		/* Send character to the BTX-Terminal */
uint8_t systemDbt03Receive(void);			/* Receive a character from the BTX-Terminal */
void systemDbt03ConnectionOk(void);			/* Tell the BTX-Terminal that connection went ok */
void systemDbt03ConnectionTerminate(void);		/* Tell the BTX-Terminal that the connection has terminated */

#endif /*DBT03_H*/
/* #################################################################################### */
 
