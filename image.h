#ifndef _IMAGE_C_
#define _IAMGE_C_

#include "common/datatype.h"

extern void image_zoom_bicubic(dword * src, int srcwidth, int srcheight, dword * dest, int destwidth, int destheight);
extern void image_zoom_bilinear(dword * src, int srcwidth, int srcheight, dword * dest, int destwidth, int destheight);
extern void image_rotate(dword * imgdata, dword * pwidth, dword * pheight, dword organgle, dword newangle);
extern int image_readpng(const char * filename, dword *pwidth, dword *pheight, dword ** image_data, dword * bgcolor);
extern int image_readgif(const char * filename, dword *pwidth, dword *pheight, dword ** image_data, dword * bgcolor);
extern int image_readjpg(const char * filename, dword *pwidth, dword *pheight, dword ** image_data, dword * bgcolor);
extern int image_readbmp(const char * filename, dword *pwidth, dword *pheight, dword ** image_data, dword * bgcolor);
extern int image_readtga(const char * filename, dword *pwidth, dword *pheight, dword ** image_data, dword * bgcolor);

#endif
