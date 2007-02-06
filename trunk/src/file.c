#include <stdio.h>
#include <sys/select.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "file.h"
#include "ByteArray.h"
#include "IntArray.h"

// copyright (c)2006 Technologic Systems
// Author: Michael Schmidt

void fileFlushInput(FILE *f) {
  char buf[256];

  while (fread(buf,1,256,f));
}

int filesWaitRead(IAref fh,struct timeval *tv) { // PURE-SE
  fd_set readset;
  int i,max=-1;

  FD_ZERO(&readset);
  for (i=0;i<fh.len;i++) {
    FD_SET(fh.arr[i],&readset);
    if (fh.arr[i] > max) {
      max = fh.arr[i];
    }
  }
  if (max > -1) {
    return select(max+1,&readset,0,0,tv);
  }
  return 0;
  /*
  for (i=0;i<n;i++) {
    if (FD_ISSET(fh.arr[i],&readset)) {
      return i;
    }
  }
  */
}

FILE *fileOpenOrDie(char *file,char *mode) { // PURE
  FILE *f;

  f = fopen(file,"r+");
  if (!f) {
    fprintf(stderr,"Error opening %s: %m\n",file);
    exit(1);
  }
  return f;
}

inline int fileSetBlocking(int fd,int on) { // PURE-SE
  return fcntl(fd, F_SETFL, on ? 0 : (FNDELAY | O_NONBLOCK)) != -1;
}

int fprinthex(FILE *f,byte *str,int n) {
  int i;

  for (i=0;i<n;i++) {
    fprintf(f,"%02X ",str[i]);
  }
  fprintf(f,"\n");
}

int printhex(byte *str,int n) {
  return fprinthex(stdout,str,n);
}


int dirMap(char *dirname,dirFunc f,void *v) {
  DIR *dir;
  struct dirent *entry;
  char full[1024];
  struct stat stats;
  int n=0;
  dir = opendir(dirname);

  do {
    entry = readdir(dir);
    if (entry) {
      sprintf(full,"%s/%s",dirname,entry->d_name);
      stat(full,&stats);
      if (S_ISREG(stats.st_mode)) {
	n++;
	if (!f(entry->d_name,v)) {
	  return n;
	}
      }
    }
  } while (entry != NULL);
  return n;
}

/*
  Get a frame from a byte stream.  The frame starts with 0x7E and ends
  with 0x7F.  Certain characters are escaped; we un-escape them.

  Pass a ByteArrayRef with len initialized to zero.  If it has a positive 
  length on return, then the ref is to a valid packet.  Otherwise, pass 
  the ref back with the next byte.

  The len field of the ref has the following meaning:

  fr.len > 0               Complete packet contains fr.len bytes
  fr.len == 0              Haven't found start of frame yet
  -256 <= fr.len <= -1     Got -fr.len bytes so far
  fr.len == -500           Found start of frame
  -1256 <= fr.len <=-1000  Found escape with -(fr.len + 1000) bytes in packet
 */
static ByteArrayRef ByteArray_PacketGet(ByteArrayRef fr,byte b) {
  if (fr.len <= -1000) { // unescape byte
    fr.len = fr.len+1000;
    fr.arr[-(fr.len--)] = b ^ 0x20;
  } else if (fr.len == -500) { // first byte of packet
    if (b == 0x7D) { // escape character
      fr.len = -1000;
    } else if (b == 0x7E) { // another start of packet
      // do nothing
    } else if (b == 0x7F) { // end of packet - already!?
      fr.len = 0;
    } else {
      fr.len = -1;
      fr.arr[0] = b;
    }
  } else if (fr.len < 0) {
      if (b == 0x7D) { // escape character
	fr.len = fr.len-1000;
      } else if (b == 0x7E) { // framing error
	fr.len = 0;
      } else if (b == 0x7F) { // end of packet
	fr.len = -fr.len;
      } else {
	fr.arr[-(fr.len--)] = b;
      }
  } else { // waiting for start of frame
    if (b == 0x7E) {
      fr.len = -500;
    }
  }
  return fr;
}

int ByteArrayRef_Checksum(ByteArrayRef pkt) { // PURE
  int i,sum=0;

  for (i=0;i<pkt.len;i++) {
    sum += pkt.arr[i];
  }
  return sum;
}

/*
  Generic packet getting
 */

ByteArrayRef filePacketGet(FILE *f,ByteArrayRef fr) {
  int i,h;
  struct timeval tv;
  int ch,tries = 5;
  ByteArrayRef b = BAR(fr.arr,0);

  h = fileno(f);
  while (tries) {
    // First, wait up to timeout for data to be available from stream
    tv.tv_sec = 0;
    tv.tv_usec = 250000;
    if (filesWaitRead(IAR(&h,1),&tv) == 0) {
      tries--;
      break;
    }
    // now read a byte at a time until no more data is available
    // simplest way to do this is simply make sure file is open non-blocking
    while (1) {
      ch = fgetc(f);
      if (ch == -1) {
	break;
      }
      b = ByteArray_PacketGet(b,(byte)ch);
      //printf("<%02X->%d>",ch,b.len);
      if (b.len > 0) {
	if ((byte)ByteArrayRef_Checksum(b) == 0xFF) {
	  //printf("Rx: "); fflush(stdout);
	  //printhex(b.arr,b.len);
	  return b;
	} else {
	  //printf("Checksum Failure\n\n\n");
	  //printhex(b.arr,b.len);
	  //printf("\nGot %02X Checksum\n",(byte)ByteArrayRef_Checksum(b));
	  b = BAR(fr.arr,0); // checksum failed, try again
	}
      }
    }
  }
  return BAR(0,0);
}
/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * */
