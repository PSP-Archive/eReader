#include <string.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include "display.h"
#include "version.h"
#include "conf.h"

static char conf_filename[256];

extern void conf_set_file(const char * filename)
{
	strcpy(conf_filename, filename);
}

extern void conf_get_keyname(dword key, char * res)
{
	res[0] = 0;
	if((key & PSP_CTRL_CIRCLE) > 0)
		strcat(res, "¡ð");
	if((key & PSP_CTRL_CROSS) > 0)
	{
		if(res[0] != 0)
			strcat(res, "+¡Á");
		else
			strcat(res, "¡Á");
	}
	if((key & PSP_CTRL_SQUARE) > 0)
	{
		if(res[0] != 0)
			strcat(res, "+¡õ");
		else
			strcat(res, "¡õ");
	}
	if((key & PSP_CTRL_TRIANGLE) > 0)
	{
		if(res[0] != 0)
			strcat(res, "+¡÷");
		else
			strcat(res, "¡÷");
	}
	if((key & PSP_CTRL_LTRIGGER) > 0)
	{
		if(res[0] != 0)
			strcat(res, "+L");
		else
			strcat(res, "L");
	}
	if((key & PSP_CTRL_RTRIGGER) > 0)
	{
		if(res[0] != 0)
			strcat(res, "+R");
		else
			strcat(res, "R");
	}
	if((key & PSP_CTRL_UP) > 0)
	{
		if(res[0] != 0)
			strcat(res, "+¡ü");
		else
			strcat(res, "¡ü");
	}
	if((key & PSP_CTRL_DOWN) > 0)
	{
		if(res[0] != 0)
			strcat(res, "+¡ý");
		else
			strcat(res, "¡ý");
	}
	if((key & PSP_CTRL_LEFT) > 0)
	{
		if(res[0] != 0)
			strcat(res, "+¡û");
		else
			strcat(res, "¡û");
	}
	if((key & PSP_CTRL_RIGHT) > 0)
	{
		if(res[0] != 0)
			strcat(res, "+¡ú");
		else
			strcat(res, "¡ú");
	}
	if((key & PSP_CTRL_SELECT) > 0)
	{
		if(res[0] != 0)
			strcat(res, "+SELECT");
		else
			strcat(res, "SELECT");
	}
	if((key & PSP_CTRL_START) > 0)
	{
		if(res[0] != 0)
			strcat(res, "+START");
		else
			strcat(res, "START");
	}
}

extern bool conf_load(p_conf conf)
{
	memset(conf, 0, sizeof(t_conf));
	int fd = sceIoOpen(conf_filename, PSP_O_RDONLY, 0777);
	conf->forecolor = 0xFFFFFFFF;
	conf->bgcolor = 0;
	conf->rowspace = 2;
	conf->infobar = true;
	conf->rlastrow = false;
	conf->autobm = true;
	conf->encode = conf_encode_gbk;
	conf->fit = conf_fit_none;
	conf->imginfobar = false;
	conf->scrollbar = false;
	conf->scale = 0;
	conf->rotate = conf_rotate_0;
	conf->txtkey[0] = PSP_CTRL_SQUARE;
	conf->txtkey[1] = PSP_CTRL_LTRIGGER;
	conf->txtkey[2] = PSP_CTRL_RTRIGGER;
	conf->txtkey[3] = PSP_CTRL_UP | PSP_CTRL_CIRCLE;
	conf->txtkey[4] = PSP_CTRL_DOWN | PSP_CTRL_CIRCLE;
	conf->txtkey[5] = PSP_CTRL_LEFT | PSP_CTRL_CIRCLE;
	conf->txtkey[6] = PSP_CTRL_RIGHT | PSP_CTRL_CIRCLE;
	conf->txtkey[7] = PSP_CTRL_LTRIGGER | PSP_CTRL_CIRCLE;
	conf->txtkey[8] = PSP_CTRL_RTRIGGER | PSP_CTRL_CIRCLE;
	conf->txtkey[9] = 0;
	conf->txtkey[10] = 0;
	conf->txtkey[11] = PSP_CTRL_CROSS;
	conf->imgkey[0] = PSP_CTRL_LTRIGGER;
	conf->imgkey[1] = PSP_CTRL_RTRIGGER;
	conf->imgkey[2] = PSP_CTRL_TRIANGLE;
	conf->imgkey[3] = PSP_CTRL_UP | PSP_CTRL_CIRCLE;
	conf->imgkey[4] = PSP_CTRL_DOWN | PSP_CTRL_CIRCLE;
	conf->imgkey[5] = PSP_CTRL_LEFT | PSP_CTRL_CIRCLE;
	conf->imgkey[6] = PSP_CTRL_RIGHT | PSP_CTRL_CIRCLE;
	conf->imgkey[7] = PSP_CTRL_SQUARE;
	conf->imgkey[8] = PSP_CTRL_CIRCLE;
	conf->imgkey[9] = PSP_CTRL_CROSS;
	conf->imgkey[10] = PSP_CTRL_LTRIGGER | PSP_CTRL_CIRCLE;
	conf->bicubic = true;
	if(fd < 0)
		return false;
	sceIoRead(fd, conf, sizeof(t_conf));
	sceIoClose(fd);
	if(conf->confver < EREADER_VERSION_NUM)
	{
		if(conf->confver < 0x000060000)
		{
			conf->imgkey[10] = PSP_CTRL_LTRIGGER | PSP_CTRL_CIRCLE;
			conf->bicubic = true;
		}
		if(conf->confver < 0x000080000)
		{
			conf->forecolor = RGB_16to32(conf->forecolor);
			conf->bgcolor = RGB_16to32(conf->bgcolor);
		}
		conf->confver = EREADER_VERSION_NUM;
	}
	return true;
}

extern bool conf_save(p_conf conf)
{
	int fd = sceIoOpen(conf_filename, PSP_O_CREAT | PSP_O_RDWR, 0777);
	if(fd < 0)
		return false;
	sceIoWrite(fd, conf, sizeof(t_conf));
	sceIoClose(fd);
	return true;
}

extern const char * conf_encodename(t_conf_encode encode)
{
	switch(encode)
	{
	case conf_encode_gbk:
		return "GBK ";
	case conf_encode_big5:
		return "BIG5";
	case conf_encode_sjis:
		return "SJIS";
	default:
		return "";
	}
}
