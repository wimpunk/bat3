#include "ByteArray.h"

// copyright (c)2006 Technologic Systems
// Author: Michael Schmidt

inline int ByteArrayRef_Normalize(BAref str,int offset) {
  if (offset < 0) {
    offset = str.len + offset;
    if (offset < 0) {
      offset = 0;
    }
  }
  if (offset >= str.len) {
    offset = str.len - 1;
  }
  return offset;
}

inline int ByteArrayRef_Bound(BAref str,int offset,int len) {
  if (offset + len > str.len) {
    len = str.len - offset;
  }
  return len > 0 ? len : 0;
}

inline BAref ByteArrayRef__OverwriteIn(BAref dst,BAref src,int offd,int offs,int len) {
  memmove(&dst.arr[offd],&src.arr[offs],len);
  return dst;
}

BAref ByteArrayRef_OverwriteIn(BAref dst,BAref src,int offd,int offs,int len) {
  offd = ByteArrayRef_Normalize(dst,offd);
  offs = ByteArrayRef_Normalize(src,offs);
  len = ByteArrayRef_Bound(dst,offd,len);
  len = ByteArrayRef_Bound(src,offs,len);
  return ByteArrayRef__OverwriteIn(dst,src,offd,offs,len);
}

inline BAref BAR(void *ptr,int len) {
  return (BAref){len,(byte *)ptr};
}

inline BAref ByteArray_BAR(ByteArray *ba) {
  return BAR(ba->arr,ba->len);
}

inline int ByteArrayRef_Compare(BAref str1,BAref str2) {
  int len = (str1.len < str2.len) ? str1.len : str2.len;
  int result;

  if (str1.len == str2.len) {
    return memcmp(str1.arr,str2.arr,len);
  } else {
    result = memcmp(str1.arr,str2.arr,len);
    if (result != 0) {
      return result;
    }
    if (str1.len > str2.len) {
      return 1;
    } else {
      return -1;
    }
  }
}

BAref ByteArrayRef_Reverse(BAref str) {
  int i;
  byte tmp;

  for (i=0;i<str.len/2;i++) {
    tmp = str.arr[i];
    str.arr[i] = str.arr[str.len-i-1];
    str.arr[str.len-i-1] = tmp;
  }
  return str;
}

inline BAref ByteArrayRef_Fill(BAref str,byte b) {
  memset(str.arr,b,str.len);
  return str;
}

BAref ByteArrayRef_Map(BAref str,void (*f)(byte,void *),void *data) {
  int i;

  for (i=0;i<str.len;i++) {
    f(str.arr[i],data);
  }
  return str;
}

BAref ByteArrayRef_FPrintHex(ByteArrayRef bar,FILE *f) {
  int i;

  for (i=0;i<bar.len;i++) {
    fprintf(f,"%02X ",bar.arr[i]);
  }
  return bar;
}

BAref ByteArrayRef_PrintHex(ByteArrayRef bar) {
  return ByteArrayRef_FPrintHex(bar,stdout);
}

BAref ByteArrayRef_FPrint(ByteArrayRef bar,FILE *f) {
  int i;

  for (i=0;i<bar.len;i++) {
    fprintf(f,"%c",bar.arr[i]);
  }
  return bar;
}

BAref ByteArrayRef_Print(ByteArrayRef bar) {
  return ByteArrayRef_FPrint(bar,stdout);
}
/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * */
