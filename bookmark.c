#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <pspkernel.h>
#include "common/utils.h"
#include "bookmark.h"

static char bmdir[256];

static void bookmark_encode(const char * filename, char * destname)
{
    register dword h, l;
	char destfilename[10];

    for (h = 5381; * filename != 0; ++filename) {
        h += h << 5;
	    h ^= *filename;
    }
	l = utils_dword2string(h, destfilename, 10);
	strcpy(destname, bmdir);
	strcat(destname, &destfilename[l]);
}

extern void bookmark_setbasedir(const char * dir)
{
	strcpy(bmdir, dir);
	strcat(bmdir, "bookmarks/");
	mkdir(bmdir, 0777);
}

extern p_bookmark bookmark_open(const char * filename)
{
	int fd;
	p_bookmark bm = (p_bookmark)calloc(1, sizeof(t_bookmark));
	if(bm == NULL)
		return NULL;
	memset(bm->row, 0xFF, 10 * sizeof(dword));
	bookmark_encode(filename, bm->filename);
	fd = sceIoOpen(bm->filename, PSP_O_RDONLY, 0777);
	if(fd < 0)
		return bm;
	sceIoRead(fd, bm->row, 10 * sizeof(dword));
	sceIoClose(fd);
	return bm;
}

extern void bookmark_save(p_bookmark bm)
{
	int fd = sceIoOpen(bm->filename, PSP_O_CREAT | PSP_O_WRONLY, 0777);
	if(fd < 0)
		return;
	sceIoWrite(fd, bm->row, 10 * sizeof(dword));
	sceIoClose(fd);
}

extern void bookmark_close(p_bookmark bm)
{
	free((void *)bm);
}

extern dword bookmark_autoload(const char * filename)
{
	int fd;
	char bmfn[256];
	dword row = 0;
	bookmark_encode(filename, bmfn);
	if((fd = sceIoOpen(bmfn, PSP_O_RDONLY, 0777)) < 0)
		return 0;
	sceIoRead(fd, &row, sizeof(dword));
	sceIoClose(fd);
	if(row == 0xFFFFFFFF)
		row = 0;
	return row;
}

extern void bookmark_autosave(const char * filename, dword row)
{
	int fd;
	char bmfn[256];
	bookmark_encode(filename, bmfn);
	if((fd = sceIoOpen(bmfn, PSP_O_RDWR, 0777)) >= 0 || (fd = sceIoOpen(bmfn, PSP_O_CREAT | PSP_O_WRONLY, 0777)) >= 0)
	{
		sceIoWrite(fd, &row, sizeof(dword));
		sceIoClose(fd);
	}
}
