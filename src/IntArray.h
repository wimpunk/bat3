// copyright (c)2006 Technologic Systems
#ifndef __INT_ARRAY_Z
#define __INT_ARRAY_Z
#include <string.h>
#include <stdio.h>
#include "base.h"

STRUCT(IntArrayRef);

struct IntArrayRef {
  int len;
  int *arr;
};

#define IAref IntArrayRef

inline IAref IAR(void *ptr,int len);

#endif
/*
 * $Id$
 *
 * $LastChangedDate$
 * $Rev$
 * $Author$
 *
 * */
