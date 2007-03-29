/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * */
#include <sys/select.h>
#include <stdio.h>
 
#include "log.h"



#define ESC 0x7D
#define STX 0x7E
#define ETX 0x7F

// Different states of the message
typedef enum {
	STATE_NOTSTARTED,
	STATE_STARTED,
	STATE_ENDED
} state_t;

static unsigned char calcCrc(char *msg, int len) {

	int crc=0xFF,pos;
	for (pos=0; pos<len; pos++) crc = (crc - msg[pos]) & 0xFF;

	return crc;

}

/* writeStream: writes msg to fd with addition of STX, CRC and ETX */
int writeStream(int fd, char *msg, int len)
{

	char mymsg[50];
	int pos=0,cnt;
	
	mymsg[pos++]=STX;
	for (cnt=0; cnt<len; cnt++) {
	// Need to escape?!
		mymsg[pos++] = msg[cnt];
	}
	mymsg[pos++] = calcCrc(msg, len);
	mymsg[pos++] = ETX;
	
		
	cnt=write(fd, mymsg, pos);
	logabba(L_MAX, "Write returned %d while len=%d, checksum is %04X", cnt, len, calcCrc(msg, len));
	
	return cnt;
	
}
/* readStream: reads msg from fd and removes STX, CRC and ETX*/
int readStream(int fd,char *msg, int max)
{

	unsigned char  rxchar;
	fd_set         rfds;
	struct timeval tv;
	int            cnt=0,pos=0,strpos=0;
	state_t        state;
	char           converted[1024];

	int FlushTime = 1 * 1000000/20; 

	unsigned int fifthbyte=0;

	FD_ZERO( &rfds );
	FD_SET( fd, &rfds );
	tv.tv_sec = FlushTime/1000000;
	tv.tv_usec = FlushTime%1000000;
	state = STATE_NOTSTARTED;
	while( select( FD_SETSIZE, &rfds, NULL, NULL, &tv ) > 0 && (state!=STATE_ENDED)) {

		read(fd, &rxchar, 1);
		switch (rxchar) {

			case STX: state = STATE_STARTED; 
				  pos = 0;
				  break;

			case ETX: if (state == STATE_STARTED) state = STATE_ENDED;
					  break;

			default: if (state==STATE_STARTED) {
					 if (msg[pos-1] == ESC) {
						 msg[pos-1] = rxchar ^ (1<<5);
					 } else {
						 msg[pos++] = rxchar;
					 }
				 }

		}

		if (pos==max) break;

	}

	if (pos>0) {

		converted[0]=0;
		for (cnt=0;cnt<pos;cnt++) {
			strpos+=snprintf(converted+strpos,sizeof(converted)-strpos," %02X", msg[cnt]);
		}

		logabba(L_MAX,"Received %d bytes (sum=%04X): %s ",pos,calcCrc(msg,pos-1),converted);

	}

	return pos;

}
