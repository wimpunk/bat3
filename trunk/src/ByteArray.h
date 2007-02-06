// copyright (c)2006 Technologic Systems
#ifndef __BYTE_ARRAY_Z
#define __BYTE_ARRAY_Z
#include <string.h>
#include <stdio.h>
#include "base.h"

STRUCT(ByteArray);
STRUCT(ByteArrayRef);

struct ByteArray {
  int len;
  byte arr[0];
};

struct ByteArrayRef {
  int len;
  byte *arr;
};

#define BAref ByteArrayRef

inline int ByteArrayRef_Normalize(BAref str,int offset);
inline int ByteArrayRef_Bound(BAref str,int offset,int len);

inline BAref ByteArrayRef__OverwriteIn(BAref dst,BAref src,int offd,int offs,int len);
BAref ByteArrayRef_OverwriteIn(BAref dst,BAref src,int offd,int offs,int len);
inline BAref BAR(void *ptr,int len);
inline BAref ByteArray_BAR(ByteArray *ba);
inline int ByteArrayRef_Compare(BAref str1,BAref str2);
BAref ByteArrayRef_Reverse(BAref str);
inline BAref ByteArrayRef_Fill(BAref str,byte b);
BAref ByteArrayRef_Map(BAref str,void (*f)(byte,void *),void *data);


BAref ByteArrayRef_FPrintHex(ByteArrayRef bar,FILE *f);
BAref ByteArrayRef_PrintHex(ByteArrayRef bar);
BAref ByteArrayRef_FPrint(ByteArrayRef bar,FILE *f);
BAref ByteArrayRef_Print(ByteArrayRef bar);


#endif
/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * */
