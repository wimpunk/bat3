// copyright (c)2006 Technologic Systems
#ifndef __FILE_Z
#define __FILE_Z

#include "base.h"
#include "IntArray.h"
#include "ByteArray.h"

struct timeval;
void fileFlushInput(FILE *f);
FILE *fileOpenOrDie(char *file,char *mode);
int filesWaitRead(IAref fh,struct timeval *tv);
inline int fileSetBlocking(int fd,int on);
int fprinthex(FILE *f,byte *str,int n);
int printhex(byte *str,int n);
int ByteArrayRef_Checksum(ByteArrayRef pkt);

typedef int (*dirFunc)(char *,void *);
int dirMap(char *dirname,dirFunc f,void *);
ByteArrayRef filePacketGet(FILE *f,ByteArrayRef fr);
#endif
/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * */
