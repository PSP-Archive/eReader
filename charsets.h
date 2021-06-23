#ifndef _CHARSETS_H_
#define _CHARSETS_HH

#include "common/datatype.h"

extern void charsets_ucs_conv(const byte *uni, byte *cjk);
extern void charsets_big5_conv(const byte *big5, byte *cjk);
extern void charsets_sjis_conv(const byte *jis, byte **cjk, dword * newsize);

#endif
