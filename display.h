#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <pspdisplay.h>
#include "common/datatype.h"

#ifdef FONT12
#define DISP_FONTSIZE 12
#else
#define DISP_FONTSIZE 16
#endif

extern dword * vram_start;

// R,G,B color to word value color
#define RGB(r,g,b) ((((dword)(b))<<16)|(((dword)(g))<<8)|((dword)(r))|0xFF000000)
#define RGBA(r,g,b,a) ((((dword)(b))<<16)|(((dword)(g))<<8)|((dword)(r))|(((dword)(a))<<24))
#define RGB_R(c) ((c) & 0xFF)
#define RGB_G(c) (((c) >> 8) & 0xFF)
#define RGB_B(c) (((c) >> 16) & 0xFF)
#define RGB_16to32(c) RGBA((((c)&0x1F)*255/31),((((c)>>5)&0x1F)*255/31),((((c)>>10)&0x1F)*255/31),((c&0x8000)?0xFF:0))

// sceDisplayWaitVblankStart function alias name, define is faster than function call (even at most time this is inline linked)
#define disp_waitv() sceDisplayWaitVblankStart()

#define disp_get_vaddr(x, y) (vram_start + (x) + ((y) << 9))

#define disp_putpixel(x,y,c) *(dword*)disp_get_vaddr((x),(y)) = (c)

extern void disp_init();

extern bool disp_load_font(const char * efont, const char * cfont);

extern void disp_free_font();

extern void disp_flip();

extern void disp_getimage(dword x, dword y, dword w, dword h, dword * buf);

extern void disp_putimage(dword x, dword y, dword w, dword h, dword startx, dword starty, dword * buf);

extern void disp_duptocache();

extern void disp_rectduptocache(dword x1, dword y1, dword x2, dword y2);

extern void disp_putnstring(word x, word y, dword color, const byte *str, int count);
#define disp_putstring(x,y,color,str) disp_putnstring((x),(y),(color),(str),0x7FFFFFFF)

extern void disp_putnstringvert(word x, word y, dword color, const byte *str, int count);
#define disp_putstringvert(x,y,color,str) disp_putnstringvert((x),(y),(color),(str),0x7FFFFFFF)

extern void disp_fillvram(dword color);

extern void disp_fillrect(dword x1, dword y1, dword x2, dword y2, dword color);

extern void disp_rectangle(dword x1, dword y1, dword x2, dword y2, dword color);

extern void disp_line(dword x1, dword y1, dword x2, dword y2, dword color);

#endif
