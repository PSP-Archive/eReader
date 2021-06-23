#include <stdlib.h>
#include <string.h>
#include <pspkernel.h>
#include "display.h"

static dword * vram_base = NULL;
dword * vram_start = NULL;
static bool vram_page = 0;
static byte * cfont_buffer = NULL, * efont_buffer = NULL;

#define DISP_EFONTSIZE (DISP_FONTSIZE)
#define DISP_CFONTSIZE (DISP_FONTSIZE * DISP_FONTSIZE / 8)
#define DISP_RSPAN 0

extern void disp_init()
{
	sceDisplaySetMode(0, 480, 272);
	vram_page = 0;
	vram_base = (dword *)0x04000000;
	vram_start = (dword *)0x44088000;
	sceDisplaySetFrameBuf(vram_base, 512, PSP_DISPLAY_PIXEL_FORMAT_8888, PSP_DISPLAY_SETBUF_NEXTFRAME);
}

extern bool disp_load_font(const char * efont, const char * cfont)
{
	int size;
	int fd = sceIoOpen(efont, PSP_O_RDONLY, 0777);
	if(fd < 0)
		return false;
	size = sceIoLseek32(fd, 0, PSP_SEEK_END);
	if((efont_buffer = (byte *)calloc(1, size)) == NULL)
	{
		sceIoClose(fd);
		return false;
	}
	sceIoLseek32(fd, 0, PSP_SEEK_SET);
	sceIoRead(fd, efont_buffer, size);
	sceIoClose(fd);

	fd = sceIoOpen(cfont, PSP_O_RDONLY, 0777);
	if(fd < 0)
	{
		disp_free_font();
		return false;
	}
	size = sceIoLseek32(fd, 0, PSP_SEEK_END);
	if((cfont_buffer = (byte *)calloc(1, size)) == NULL)
	{
		disp_free_font();
		sceIoClose(fd);
		return false;
	}
	sceIoLseek32(fd, 0, PSP_SEEK_SET);
	sceIoRead(fd, cfont_buffer, size);
	sceIoClose(fd);
	return true;
}

extern void disp_free_font()
{
	if(efont_buffer != NULL)
		free((void *)efont_buffer);
	if(cfont_buffer != NULL)
		free((void *)cfont_buffer);
}

extern void disp_flip()
{
	disp_waitv();
	vram_page ^= 1;
	vram_base = (dword *)(0x04000000 + (vram_page ? 0x88000 : 0));
	vram_start = (dword *)(0x44000000 + (vram_page ? 0 : 0x88000));
	sceDisplaySetFrameBuf(vram_base, 512, PSP_DISPLAY_PIXEL_FORMAT_8888, PSP_DISPLAY_SETBUF_IMMEDIATE);
}

extern void disp_getimage(dword x, dword y, dword w, dword h, dword * buf)
{
	dword * lines = vram_base + 0x10000000, * linesend = lines + (min(272 - y, h) << 9);
	dword rw = (min(512 - x, w) << 2);
	for(;lines < linesend; lines += 512)
	{
		memcpy(buf, lines, rw);
		buf += w;
	}
}

extern void disp_putimage(dword x, dword y, dword w, dword h, dword startx, dword starty, dword * buf)
{
	dword * lines = disp_get_vaddr(x, y), * linesend = lines + (min(272 - y, h - starty) << 9);
	buf = buf + starty * w + startx;
	dword rw = (min(512 - x, w - startx) << 2);
	for(;lines < linesend; lines += 512)
	{
		memcpy(lines, buf, rw);
		buf += w;
	}
}

extern void disp_duptocache()
{
	memmove(vram_start, ((byte *)vram_base) + 0x40000000, 0x88000);
}

extern void disp_rectduptocache(dword x1, dword y1, dword x2, dword y2)
{
	dword * lines = disp_get_vaddr(x1, y1), * linesend = disp_get_vaddr(x1, y2), * lined = vram_base + 0x10000000 + x1 + (y1 << 9);
	dword w = ((x2 - x1 + 1) << 2);
	for(;lines <= linesend; lines += 512, lined += 512)
		memcpy(lines, lined, w);
}

extern void disp_putnstring(word x, word y, dword color, const byte *str, int count)
{
	dword * vaddr;
	const byte * ccur, * cend;

	while(*str != 0 && count > 0)
	{
		if(*str > 0x80)
		{
			if(x > 480 - DISP_RSPAN - DISP_FONTSIZE)
			{
				x = 0;
				y += DISP_FONTSIZE;
			}
			vaddr = disp_get_vaddr(x,y);
			ccur = cfont_buffer + (((dword)(*str - 0x81)) * 0xBF + ((dword)(*(str + 1) - 0x40))) * DISP_CFONTSIZE;
		
			for (cend = ccur + DISP_CFONTSIZE; ccur < cend; ccur ++) {
				int b;
				byte ch =*ccur;
				dword * vpoint = vaddr;
				for (b = 0x80; b > 0; b >>= 1) {
					if ((ch & b) != 0)
						* vpoint = color;
					vpoint ++;
				}
#ifdef FONT12
				ch = *(++ ccur);
				for (b = 0x80; b > 8; b >>= 1) {
					if ((ch & b) != 0)
						* vpoint = color;
					vpoint ++;
				}
				vaddr += 512;
				vpoint = vaddr;
				for (; b > 0; b >>= 1) {
					if ((ch & b) != 0)
						* vpoint = color;
					vpoint ++;
				}
#endif
				ch = *(++ ccur);
				for (b = 0x80; b > 0; b >>= 1) {
					if ((ch & b) != 0)
						* vpoint = color;
					vpoint ++;
				}
				vaddr += 512;
			}
			str += 2;
			count -= 2;
			x += DISP_FONTSIZE;
		}
		else if(*str > 0x1F)
		{
			if(x > 480 - DISP_RSPAN - DISP_FONTSIZE / 2)
			{
				x = 0;
				y += DISP_FONTSIZE;
			}
			vaddr = disp_get_vaddr(x,y);
#ifdef FONT12
			ccur = efont_buffer + (((dword)*str) - 0x20) * DISP_EFONTSIZE;
#else
			ccur = efont_buffer + ((dword)*str) * DISP_EFONTSIZE;
#endif

			for (cend = ccur + DISP_EFONTSIZE; ccur < cend; ccur ++) {
				int b;
				dword * vpoint = vaddr;
#ifdef FONT12
				for (b = 0x80; b > 2; b >>= 1) {
#else
				for (b = 0x80; b > 0; b >>= 1) {
#endif
					if (((*ccur) & b) != 0)
						* vpoint = color;
					vpoint ++;
				}
				vaddr += 512;
			}
			str ++;
			count --;
			x += DISP_FONTSIZE / 2;
		}
		else
		{
			if(x > 480 - DISP_RSPAN - DISP_FONTSIZE / 2)
			{
				x = 0;
				y += DISP_FONTSIZE;
			}
			str ++;
			count --;
			x += DISP_FONTSIZE / 2;
		}
	}
}

extern void disp_putnstringvert(word x, word y, dword color, const byte *str, int count)
{
	dword * vaddr;
	const byte * ccur, * cend;

	while(*str != 0 && count > 0)
	{
		if(*str > 0x80)
		{
			if(y > 272 - DISP_RSPAN - DISP_FONTSIZE)
			{
				y = 0;
				x -= DISP_FONTSIZE;
			}
			vaddr = disp_get_vaddr(x,y);
			ccur = cfont_buffer + (((dword)(*str - 0x81)) * 0xBF + ((dword)(*(str + 1) - 0x40))) * DISP_CFONTSIZE;
		
			for (cend = ccur + DISP_CFONTSIZE; ccur < cend; ccur ++) {
				int b;
				byte ch =*ccur;
				dword * vpoint = vaddr;
				for (b = 0x80; b > 0; b >>= 1) {
					if ((ch & b) != 0)
						* vpoint = color;
					vpoint += 512;
				}
#ifdef FONT12
				ch = *(++ ccur);
				for (b = 0x80; b > 8; b >>= 1) {
					if ((ch & b) != 0)
						* vpoint = color;
					vpoint += 512;
				}
				vaddr --;
				vpoint = vaddr;
				for (; b > 0; b >>= 1) {
					if ((ch & b) != 0)
						* vpoint = color;
					vpoint += 512;
				}
#endif
				ch = *(++ ccur);
				for (b = 0x80; b > 0; b >>= 1) {
					if ((ch & b) != 0)
						* vpoint = color;
					vpoint += 512;
				}
				vaddr --;
			}
			str += 2;
			count -= 2;
			y += DISP_FONTSIZE;
		}
		else if(*str > 0x1F)
		{
			if(y > 272 - DISP_RSPAN - DISP_FONTSIZE / 2)
			{
				y = 0;
				x -= DISP_FONTSIZE;
			}
			vaddr = disp_get_vaddr(x,y);
#ifdef FONT12
			ccur = efont_buffer + (((dword)*str) - 0x20) * DISP_EFONTSIZE;
#else
			ccur = efont_buffer + ((dword)*str) * DISP_EFONTSIZE;
#endif

			for (cend = ccur + DISP_EFONTSIZE; ccur < cend; ccur ++) {
				int b;
				dword * vpoint = vaddr;
#ifdef FONT12
				for (b = 0x80; b > 2; b >>= 1) {
#else
				for (b = 0x80; b > 0; b >>= 1) {
#endif
					if (((*ccur) & b) != 0)
						* vpoint = color;
					vpoint += 512;
				}
				vaddr --;
			}
			str ++;
			count --;
			y += DISP_FONTSIZE / 2;
		}
		else
		{
			if(y > 272 - DISP_RSPAN - DISP_FONTSIZE / 2)
			{
				y = 0;
				x -= DISP_FONTSIZE;
			}
			str ++;
			count --;
			y += DISP_FONTSIZE / 2;
		}
	}
}

extern void disp_fillvram(dword color)
{
	dword *vram, *vram_end;

	if(color == 0 || color == 0xFFFFFFFF)
		memset(vram_start, (color & 0xFF), 0x88000);
	else
		for (vram = vram_start, vram_end = vram + 0x22000; vram < vram_end; vram ++)
			* vram = color;
}

extern void disp_fillrect(dword x1, dword y1, dword x2, dword y2, dword color)
{
	dword * vsram, * vsram_end, * vram, * vram_end;
	dword wdwords;
	if(x1 > x2)
	{
		dword t = x1; x1 = x2; x2 = t;
	}
	if(y1 > y2)
	{
		dword t = y1; y1 = y2; y2 = t;
	}
	vsram = disp_get_vaddr(x1, y1);
	vsram_end = vsram + 512 * (y2 - y1);
	wdwords = (x2 - x1);
	for(;vsram <= vsram_end; vsram += 512)
		for(vram = vsram, vram_end = vram + wdwords; vram <= vram_end; vram ++)
			* vram = color;
}

extern void disp_rectangle(dword x1, dword y1, dword x2, dword y2, dword color)
{
	dword *vsram, * vram, * vram_end;
	if(x1 > x2)
	{
		dword t = x1; x1 = x2; x2 = t;
	}
	if(y1 > y2)
	{
		dword t = y1; y1 = y2; y2 = t;
	}
	vsram = disp_get_vaddr(x1, y1);
	for(vram = vsram, vram_end = vram + (x2 - x1); vram < vram_end; vram ++)
		* vram = color;
	for(vram_end = vram + 512 * (y2 - y1); vram < vram_end; vram += 512)
		* vram = color;
	for(vram = vsram, vram_end = vram + 512 * (y2 - y1); vram < vram_end; vram += 512)
		* vram = color;
	for(vram_end = vram + (x2 - x1); vram < vram_end; vram ++)
		* vram = color;
	* vram = color;
}

extern void disp_line(dword x1, dword y1, dword x2, dword y2, dword color)
{
	dword * vram;
	int dy, dx, x, y, d;
	dx = (int)x2 - (int)x1;
	dy = (int)y2 - (int)y1;
	if(dx < 0)
		dx = -dx;
	if(dy < 0)
		dy = -dy;
	d = -dx;
	x = (int)x1;
	y = (int)y1;
	if(dx >= dy)
	{
		if(y2 < y1)
		{
			dword t = x1; x1 = x2; x2 = t; t = y1; y1 = y2; y2 = t;
		}
		vram = disp_get_vaddr(x1, y1);
		if(x1 < x2)
		{
			for(x = x1; x <= x2; x ++)
			{
				if(d > 0)
				{
					y ++;
					vram += 512;
					d -= 2 * dx;
				}
				d += 2 * dy;
				* vram = color;
				vram ++;
			}
		}
		else
		{
			for(x = x1; x >= x2; x --)
			{
				if(d > 0)
				{
					y ++;
					vram += 512;
					d -= 2 * dx;
				}
				d += 2 * dy;
				* vram = color;
				vram --;
			}
		}
	}
	else
	{
		if(x2 < x1)
		{
			dword t = x1; x1 = x2; x2 = t; t = y1; y1 = y2; y2 = t;
		}
		vram = disp_get_vaddr(x1, y1);
		if(y1 < y2)
		{
			for(y = y1; y <= y2; y ++)
			{
				if(d > 0)
				{
					x ++;
					vram ++;
					d -= 2 * dy;
				}
				d += 2 * dx;
				* vram = color;
				vram += 512;
			}
		}
		else
		{
			for(y = y1; y >= y2; y --)
			{
				if(d > 0)
				{
					x ++;
					vram ++;
					d -= 2 * dy;
				}
				d += 2 * dx;
				* vram = color;
				vram -= 512;
			}
		}
	}
}
