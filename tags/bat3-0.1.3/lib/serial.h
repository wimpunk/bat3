// copyright (c)2006 Technologic Systems
#ifndef _SERIAL_Z
#define _SERIAL_Z
#include "ByteArray.h"

inline int serialEscape(byte b); // PURE
int serialFrameOverhead(ByteArrayRef pkt); // PURE
int serialFrame(ByteArrayRef src,ByteArrayRef dst); // CLEAN
ByteArrayRef serialUnframe(ByteArrayRef src); // IMPURE
inline int serialFramedSend(FILE *f,ByteArrayRef src); // PURE-SE
#endif
