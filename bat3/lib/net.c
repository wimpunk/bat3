#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include "net.h"

// copyright (c)2006 Technologic Systems
// Author: Michael Schmidt

int netUDPopen(int port,int asServer) {
  int fd,tmp;
  struct sockaddr_in from;

  fd = socket(AF_INET,SOCK_DGRAM,0);
  if (fd == -1) { 
    return -1; 
  }
  from.sin_family = AF_INET;
  from.sin_port = htons(port);
  from.sin_addr.s_addr = htonl(INADDR_ANY);
  if (asServer) {
    tmp = bind(fd,(struct sockaddr *)&from,sizeof(from));
    if (tmp == -1) {
      close(fd);
      return -2;
    }
  }
  return fd;
}

BAref netUDPrx(int fd,BAref buf,struct sockaddr_in *from) {
  int size;
  unsigned fromlen=sizeof(*from);

  bzero((char *)from,sizeof(*from));
  from->sin_family = AF_INET;
  size = recvfrom(fd,buf.arr,buf.len,0,(struct sockaddr *)from,&fromlen);
  if (size == -1) {
    return BAR(0,0);
  }
  return BAR(buf.arr,size);
}

time_t timeOfLastTx = 0;
int lastTxRc = 0;
int netUDPtx(int fd,BAref buf,struct sockaddr_in *to) {
  int sent;
  
  time(&timeOfLastTx);
  lastTxRc = sent = sendto(fd,buf.arr,buf.len,0,
		(struct sockaddr *)to,sizeof(struct sockaddr_in));
  return sent;
}

void netUDPPrint(struct sockaddr_in *from) {
  int i,j;

  printf("%d.",(from->sin_addr.s_addr)&0xFF);
  printf("%d.",(from->sin_addr.s_addr>>8)&0xFF);
  printf("%d.",(from->sin_addr.s_addr>>16)&0xFF);
  printf("%d",(from->sin_addr.s_addr>>24)&0xFF);
  printf(" port %d\n",ntohs(from->sin_port));
}


void netAdrsFromHost(struct sockaddr_in *to,struct hostent *host,int port) {
  bzero((char *)to,sizeof(*to));
  to->sin_family = AF_INET;
  bcopy((char *)host->h_addr,
	(char *)&to->sin_addr.s_addr,
	host->h_length);
  to->sin_port = htons(port);
}

inline void netAdrsFromName(struct sockaddr_in *adrs,char *name,int port) {
  struct hostent *h = gethostbyname(name);
  netAdrsFromHost(adrs,h,port);
}
