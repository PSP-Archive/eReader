#include <stdio.h>
#include <string.h>
#include <pspkernel.h>
#include "utils.h"

extern dword utils_dword2string(dword dw, char * dest, dword width)
{
	dest[width] = 0;
	if(dw == 0)
	{
		dest[--width] = '0';
		return width;
	}
	while(dw > 0 && width > 0)
	{
		dest[-- width] = '0' + (dw % 10);
		dw /= 10;
	}
	return width;
}

extern const char * utils_fileext(const char * filename)
{
	dword len = strlen(filename);
	const char * p = filename + len;
	while(p > filename && *p != '.' && *p != '/') p --;
	if(*p == '.')
		return p + 1;
	else
		return NULL;
}

extern void utils_del_dir(char * dir)
{
	int dl = sceIoDopen(dir);
	if(dl < 0)
		return;
	SceIoDirent sid;
	memset(&sid, 0, sizeof(SceIoDirent));
	while(sceIoDread(dl, &sid))
	{
		if(sid.d_name[0] == '.') continue; // hide file
		char compPath[260];
		sprintf(compPath, "%s/%s", dir, sid.d_name);
		if(FIO_S_ISDIR(sid.d_stat.st_mode)) // dir
		{
			utils_del_dir(compPath);
			continue;
		}
		sceIoRemove(compPath);
		memset(&sid, 0, sizeof(SceIoDirent));
	}
	sceIoDclose(dl);
	sceIoRmdir(dir);
}
