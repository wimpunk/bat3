#include "IntArray.h"

// copyright (c)2006 Technologic Systems
// Author: Michael Schmidt

inline IAref IAR(void *ptr,int len) {
  return (IAref){len,(int *)ptr};
}
