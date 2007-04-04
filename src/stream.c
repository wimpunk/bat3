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
#include <unistd.h>
#include <string.h>
#include <errno.h>


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "term.h"
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

static unsigned char calcCrc(unsigned char *msg, int len) {
    
    int crc=0xFF, pos;
    
    for (pos=0; pos<len; pos++) crc = (crc - msg[pos]) & 0xFF;
    
    return crc;
    
}

int openStream(char *device) {
    
    int fd;
    
    if ((fd = open(device, O_NONBLOCK|O_RDWR))!=-1) {
	termConfigRaw(fd);
    }
    
    return fd;
    
}

/* writeStream: writes msg to fd with addition of STX, CRC and ETX
 adds ESC when needed*/
int writeStream(int fd, unsigned char *msg, int len) {
    
    unsigned char mymsg[50];
    int pos=0, cnt;
    char converted[1024];
    int strpos;
    unsigned char crc;
    // char b;
    
    mymsg[pos++]=STX;
    for (cnt=0; cnt<len; cnt++) {
	
	if (msg[cnt] == 0x7E || msg[cnt] == 0x7D || msg[cnt] == 0x7F || msg[cnt] == 0x11 || msg[cnt] == 0x13) {
	    // logabba(L_MIN, "writeStream changed one");
	    mymsg[pos++] = ESC;
	    mymsg[pos++] = msg[cnt] ^ (1<<5);
	} else {
	    mymsg[pos++] = msg[cnt];
	}
    }
    
    crc = calcCrc(msg, len);
    if (crc == 0x7E || crc == 0x7D || crc == 0x7F || crc == 0x11 || crc == 0x13) {
	// logabba(L_MIN, "writeStream changed crc");
	mymsg[pos++] = ESC;
	mymsg[pos++] = crc ^ (1<<5);
    }
    else {
	// crc = calcCrc(mymsg+1, pos-1);
	mymsg[pos++] = crc;
    }
    mymsg[pos++] = ETX;
    
    converted[0]=0;
    strpos=0;
    for (cnt=0;cnt<pos;cnt++) {
	strpos+=snprintf(converted+strpos, sizeof(converted)-strpos, " %02X", mymsg[cnt]);
    }
    
    // logabba(L_MIN, "strpos = %d, cnt=%d, converted=%s, strlen=%d", strpos, cnt, converted, strlen(converted));
    
    cnt = write(fd, mymsg, pos);
    
    logabba(L_INFO, "Write returned %d while pos=%d, checksum is %04X for %s", cnt, pos, calcCrc(msg, len), converted);
    
    return cnt;
    
}
/* readStream: reads msg from fd and removes STX, CRC and ETX*/
int readStream(int fd, unsigned char *msg, int max) {
    
    unsigned char  rxchar;
    fd_set         rfds;
    struct timeval tv;
    int            cnt=0, pos=0, strpos=0;
    state_t        state;
    char           converted[1024];
    
    int FlushTime = 1000 * 1000;
    // int crc=0;
    int lastesc=-1;
    // unsigned int fifthbyte=0;
    
    FD_ZERO( &rfds );
    FD_SET( fd, &rfds );
    tv.tv_sec = FlushTime/1000000;
    tv.tv_usec = FlushTime%1000000;
    state = STATE_NOTSTARTED;
    
    
    while( select( FD_SETSIZE, &rfds, NULL, NULL, &tv ) > 0 && (state!=STATE_ENDED)) {
	
	read(fd, &rxchar, 1);
	switch (rxchar) {
	    
	    case STX:
		state = STATE_STARTED;
		pos = 0;
		break;
		
	    case ETX:
		if (state == STATE_STARTED) {
		    state = STATE_ENDED;
		}
		break;
		
	    default:
		if (state==STATE_STARTED) {
		    if (msg[pos-1] == ESC && lastesc!=pos) {
			logabba(L_NOTICE, "converted one on pos %d", pos);
			lastesc=pos;
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
	    strpos+=snprintf(converted+strpos, sizeof(converted)-strpos, " %02X", msg[cnt]);
	}
	
	logabba(L_NOTICE, "Received %d bytes (calcCrc=%04X,crc=%04X): %s ",
	pos, calcCrc(msg, pos-1), msg[pos-1], converted);
	
	// TODO if (calcCrc(msg, pos-1) == msg[pos-1])
	// CRC is calculated on the original message!
	// if (pos==19) return pos-1; //don't send the crc
	return pos-1; //don't send the crc
	
    }
    
    return 0; // error
    
}


