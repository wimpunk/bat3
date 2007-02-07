#include <alloca.h>
#include <stdio.h>
#include "serial.h"
#include "log.h"

// copyright (c)2006 Technologic Systems
// Author: Michael Schmidt
/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * */

inline int serialEscape(byte b) { // PURE
	return (b == 0x7E || b == 0x7D || b == 0x7F || b == 0x11 || b == 0x13);
}

int serialFrameOverhead(ByteArrayRef pkt) { // PURE
	int i,off = 2;

	for (i=0;i<pkt.len;i++) { // count number of escapes needed, calc cksum
		if (serialEscape(pkt.arr[i])) {
			off++;
		}
	}
	return off;
}

int serialFrame(ByteArrayRef src,ByteArrayRef dst) { // CLEAN
	int i,j=0;
	byte b;

	dst.arr[j++] = 0x7E; // 
	for (i=0;i<src.len;i++) {
		if (serialEscape(src.arr[i])) {
			dst.arr[j++] = 0x7D;
			b = src.arr[i] ^ 0x20;
		} else {
			b = src.arr[i];
		}
		dst.arr[j++] = b;
	}
	dst.arr[j++] = 0x7F;
	return 1;
}

ByteArrayRef serialUnframe(ByteArrayRef src) { // IMPURE
	int i=0,j=0,off=2;
	byte b;

	if (src.len <= i || src.arr[i] != 0x7E || src.arr[src.len-1] != 0x7F) {
		return BAR(0,0);
	}
	for (i=1;i<src.len-1;i++) {
		if (src.arr[i] == 0x7D) {
			b = src.arr[++i] ^ 0x20;
			off++;
		} else {
			b = src.arr[i];
		}
		src.arr[j++] = b;
	}
	src.len -= off;
	return src;
}

inline int serialFramedSend(FILE *f,ByteArrayRef src) { // PURE-SE

	byte *buffer;
	ByteArrayRef pkt;
	int i,len;
	char msg[1024];

	len = src.len+serialFrameOverhead(src); // aantal escapes + 2 bijtellen
	buffer = alloca(len);			// malloc maar met free na einde fuctie
	pkt = BAR(buffer,len);			// pkt = ByteArray(len,buffer);
	serialFrame(src,pkt);			// 0x7E Escaped(buffer) 0x7F

	// logging
	for (i=0,len=0; i<pkt.len; i++) {
		len+=snprintf(msg+len, 1024-len, " %02X", pkt.arr[i]);
	}
	logabba(L_MIN,"Writing to bat3:%s",msg);


	//printf("Sending packet: "); fflush(stdout);
	i = write(fileno(f),pkt.arr,pkt.len);
	//  tcdrain(fileno(f));
	//printhex(pkt.arr,pkt.len);
}

