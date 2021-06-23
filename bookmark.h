#ifndef _BOOKMARK_H_
#define _BOOKMARK_H_

#include "common/datatype.h"

typedef struct {
	char filename[256];
	dword row[10];
} t_bookmark, * p_bookmark;

extern void bookmark_setbasedir(const char * dir);
extern p_bookmark bookmark_open(const char * filename);
extern void bookmark_save(p_bookmark bm);
extern void bookmark_close(p_bookmark bm);
extern dword bookmark_autoload(const char * filename);
extern void bookmark_autosave(const char * filename, dword row);

#endif
