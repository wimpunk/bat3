/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * */
#include <stdio.h>
#include <stdlib.h>
#include <sys/termios.h> 

#include "avr.h"
#include "file.h"
#include "term.h"

// copyright (c)2006 Technologic Systems
// Author: Michael Schmidt

ByteArrayRef AVRreplyGet(FILE *f,ByteArrayRef pReply) {
	ByteArrayRef b;
	byte ck;

	pReply.len = 0;
	b = filePacketGet(f,pReply);
	if (!b.arr) {
		printf("Failed to communicate with AVR\n");
		exit(3);
	}
	return b;
}

// AVR send callback returns:
// 0: to indicate that its satified that the request has succeeded
// 1: to indicate that the request should be re-sent
// 2: to indicate that it wants another reply without a re-send
ByteArrayRef AVRsendRequest(FILE *f,ByteArrayRef req,ByteArrayRef ret,
		int (*callback)(ByteArrayRef,ByteArrayRef)) {
	ByteArrayRef b;
	int retry,retries=5;
	int replies_till_retry = 2;

	req.arr[req.len-1] = 0;
	req.arr[req.len-1] = 0xFF - ByteArrayRef_Checksum(req);
	serialFramedSend(f,req);
	while (retries--) {
		b = filePacketGet(f,ret);
		if (!b.arr) {
			printf("Failed to communicate with AVR\n");
			exit(3);
		}
		if (b.len == 0) {
			retry = 1;
		} else {
			retry = callback(req,ret);
		}
		if (retry == 0) {
			return b;
		}
		if (--replies_till_retry == 0) {
			serialFramedSend(f,req);
			replies_till_retry = 2;
		}
	}
	printf("Failed to change AVR state\n");
	exit (3);
	//return BYTE_ARRAY_REF(0,0);
}

/*
   Common AVR behavior:
   1. There are only two packets: "request" which is to the AVR and "reply"
   which is from the AVR.
   2. Periodically the AVR sends out its current state in a "reply" packet.
   3. When we send a request, we must "echo" back the AVR state that we don't 
   want to change

   wait for AVR reply packet
   initialize request from reply
   process options to modify request
   if options indicated a more complicated task
   perform that task
   if options indicated a simple request to send
   send request until it is acknowledged
   if (we are displaying the current state)
   read a specific number of packets
   print out the latest state
   */
void _AVRrun(FILE *f,void *data,int argc,char **argv,
		void (*setup)(FILE *,void *),
		int (*init)(ByteArrayRef,ByteArrayRef),
		int (*opt)(FILE *,ByteArrayRef,void *,int *,int,char **),
		void (*print)(ByteArrayRef,void *),
		int (*task)(FILE *,int,ByteArrayRef,ByteArrayRef,void*),
		int (*callback)(ByteArrayRef,ByteArrayRef)
	    ) {
	byte req[256],rep[256];
	ByteArrayRef bReq = BAR(req,256),bRep = BAR(rep,256);
	int todo,loops;

	if (setup) {
		setup(f,data);
	}
	bRep = AVRreplyGet(f,bRep);
	bReq.len = init(bRep,bReq);
	todo = opt(f,bReq,data,&loops,argc,argv);
	if (todo & 0xFFFFFFFE) { // options indicated a simple request to send
		todo |= task(f,todo,bReq,bRep,data);
	}
	if (todo & 1) { // options indicated a simple request to send
		AVRsendRequest(f,bReq,bRep,callback);
	}
	if (loops) {
		while (loops--) {
			bRep = AVRreplyGet(f,bRep);
		}
		print(bRep,data);
	}
}

void AVRrun(char *ser,void *data,int argc,char **argv,
		void (*setup)(FILE *,void *),
		int (*init)(ByteArrayRef,ByteArrayRef),
		int (*opt)(FILE *,ByteArrayRef,void *,int *,int,char **),
		void (*print)(ByteArrayRef,void *),
		int (*task)(FILE *,int,ByteArrayRef,ByteArrayRef,void*),
		int (*callback)(ByteArrayRef,ByteArrayRef)
	   ) {
	FILE *f;
	struct termios saved;

	f = fileOpenOrDie(ser,"r+");
	fileSetBlocking(fileno(f),0);
	termConfigRaw(fileno(f),&saved);

	_AVRrun(f,data,argc,argv,setup,init,opt,print,task,callback);

	tcsetattr(fileno(f),TCSANOW,&saved);
	fclose(f);
}
