// copyright (c)2006 Technologic Systems
#ifndef __NET_Z
#define __NET_Z
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "ByteArray.h"

int netUDPopen(int port,int asServer);

BAref netUDPrx(int fd,BAref buf,struct sockaddr_in *from);
int netUDPtx(int fd,BAref buf,struct sockaddr_in *to);
void netUDPPrint(struct sockaddr_in *from);
void netAdrsFromHost(struct sockaddr_in *adrs,struct hostent *h,int port);
inline void netAdrsFromName(struct sockaddr_in *adrs,char *name,int port);


#endif
