#include <alloca.h>
#include <stdio.h>
#include "serial.h"
//#include "file.h"

// copyright (c)2006 Technologic Systems
// Author: Michael Schmidt

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

  dst.arr[j++] = 0x7E;
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

  len = src.len+serialFrameOverhead(src);
  buffer = alloca(len);
  pkt = BAR(buffer,len);
  serialFrame(src,pkt);
  //printf("Sending packet: "); fflush(stdout);
  i = write(fileno(f),pkt.arr,pkt.len);
  //  tcdrain(fileno(f));
  //printhex(pkt.arr,pkt.len);
}

