#include <stdlib.h>
#include <string.h>
#include "display.h"
#include "image.h"
#include "bg.h"

static dword * bg_start = (dword *)0x44110000;
static bool have_bg = false;

extern void bg_load(const char * filename)
{
	dword * imgdata, * imgshow = NULL, * img_buf, * bg_buf, * bg_line, * bg_end, * bg_lineend;
	dword width, height, w2, h2, bgcolor, left, top;
	if(image_readpng(filename, &width, &height, &imgdata, &bgcolor) != 0)
		return;
	if(width > 480)
	{
		h2 = height * 480 / width;
		if(h2 > 272)
		{
			h2 = 272;
			w2 = width * 272 / height;
		}
		else
			w2 = 480;
	}
	else if(height > 272)
	{
		h2 = 272;
		w2 = width * 272 / height;
	}
	else
	{
		h2 = height;
		w2 = width;
	}
	if(width != w2 || height != h2)
	{
		imgshow = (dword *)malloc(sizeof(dword) * w2 * h2);
		if(imgshow == NULL)
		{
			free((void *)imgdata);
			return;
		}
		image_zoom_bicubic(imgdata, width, height, imgshow, w2, h2);
	}
	else
		imgshow = imgdata;

	if(w2 < 480)
		left = (480 - w2) / 2;
	else
		left = 0;
	if(h2 < 272)
		top = (272 - h2) / 2;
	else
		top = 0;

	for(bg_buf = bg_start, bg_end = bg_start + 0x88000; bg_buf < bg_end; bg_buf ++)
		* bg_buf = bgcolor;
	img_buf = imgshow;
	for(bg_buf = bg_start + top * 512 + left, bg_end = bg_buf + h2 * 512; bg_buf < bg_end; bg_buf += 512)
		for(bg_line = bg_buf, bg_lineend = bg_buf + w2; bg_line < bg_lineend; bg_line ++)
			*bg_line = *img_buf ++;

	have_bg = true;

	if(imgshow != imgdata)
		free((void *)imgshow);
	free((void *)imgdata);
}

extern bool bg_display()
{
	if(have_bg)
	{
		memcpy(vram_start, bg_start, 0x88000);
		return true;
	}
	return false;
}
