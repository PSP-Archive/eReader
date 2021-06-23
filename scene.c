#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pspkernel.h>
#include "display.h"
#include "win.h"
#include "ctrl.h"
#include "fs.h"
#include "image.h"
#include "usb.h"
#include "power.h"
#include "bookmark.h"
#include "conf.h"
#include "charsets.h"
#include "fat.h"
#include "mp3.h"
#include "bg.h"
#include "version.h"
#include "common/log.h"
#include "common/utils.h"
#include "scene.h"

static char appdir[256];
static dword rowsperpage;
static p_bookmark bm = NULL;
static p_fs_text fs = NULL;
static t_conf config;
static p_win_menuitem filelist = NULL;
static dword filecount = 0;
static enum {
	scene_in_dir,
	scene_in_zip,
	scene_in_chm,
} where;

static void scene_mp3bar()
{
	while(1)
	{
		disp_waitv();
		disp_duptocache();
		disp_rectangle(5, 263 - DISP_FONTSIZE * 3, 474, 267, 0xFFFFFFFF);
		disp_fillrect(6, 264 - DISP_FONTSIZE * 3, 473, 266, RGB(0x18, 0x28, 0x50));
		int bitrate, sample, len, tlen;
		char infostr[80];
		if(mp3_get_info(&bitrate, &sample, &len, &tlen))
			sprintf(infostr, "%d kbps    %d Hz    %02d:%02d / %02d:%02d", bitrate / 1000, sample, len / 60, len % 60, tlen / 60, tlen % 60);
		else
			strcpy(infostr, "");
		disp_putstring(6 + DISP_FONTSIZE, 264 - DISP_FONTSIZE * 3, 0xFFFFFFFF, (const byte *)"○播放 ×暂停 □停止  L上一首  R下一首");
		disp_putstring(6 + DISP_FONTSIZE, 265 - DISP_FONTSIZE * 2, 0xFFFFFFFF, (const byte *)infostr);
		disp_putnstring(6 + DISP_FONTSIZE, 266 - DISP_FONTSIZE, 0xFFFFFFFF, (const byte *)mp3_get_tag(), (468 - DISP_FONTSIZE * 2) * 2 / DISP_FONTSIZE);
		disp_flip();
		switch(ctrl_read())
		{
			case (PSP_CTRL_SELECT | PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER):
				if(win_msgbox("是否退出软件?", "是", "否", 0xFFFFFFFF, 0xFFFFFFFF, RGB(0x18, 0x28, 0x50)))
					scene_exit();
				break;
			case PSP_CTRL_CIRCLE:
				if(fs != NULL)
					power_set_clock(222, 111);
				mp3_resume();
				break;
			case PSP_CTRL_CROSS:
				mp3_pause();
				if(fs != NULL)
					power_set_clock(50, 25);
				break;
			case PSP_CTRL_SQUARE:
				mp3_restart();
				mp3_pause();
				if(fs != NULL)
					power_set_clock(50, 25);
				break;
			case PSP_CTRL_LTRIGGER:
				mp3_prev();
				break;
			case PSP_CTRL_RTRIGGER:
				mp3_next();
				break;
			case PSP_CTRL_START:
				disp_waitv();
				disp_fillvram(0);
				disp_flip();
				disp_fillvram(0);
				return;
		}
		sceKernelDelayThread(50000);
	}
}

static t_win_menu_op scene_imgkey_menucb(dword key, p_win_menuitem item, dword count, dword max_height, dword topindex, dword * index)
{
	switch(key)
	{
		case (PSP_CTRL_SELECT | PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER):
			if(win_msgbox("是否退出软件?", "是", "否", 0xFFFFFFFF, 0xFFFFFFFF, RGB(0x18, 0x28, 0x50)))
				scene_exit();
			return win_menu_op_force_redraw;
		case PSP_CTRL_CIRCLE:
			disp_waitv();
			disp_duptocache();
			disp_rectangle(239 - DISP_FONTSIZE * 3, 135 - DISP_FONTSIZE / 2, 240 + DISP_FONTSIZE * 3, 136 + DISP_FONTSIZE / 2, 0xFFFFFFFF);
			disp_fillrect(240 - DISP_FONTSIZE * 3, 136 - DISP_FONTSIZE / 2, 239 + DISP_FONTSIZE * 3, 135 + DISP_FONTSIZE / 2, RGB(0x8, 0x18, 0x10));
			disp_putstring(240 - DISP_FONTSIZE * 3, 136 - DISP_FONTSIZE / 2, 0xFFFFFFFF, (const byte *)"请按对应按键");
			disp_flip();
			dword key, key2;
			SceCtrlData ctl;
			ctrl_waitrelease();
			do {
				sceCtrlReadBufferPositive(&ctl, 1);
				key = (ctl.Buttons & ~PSP_CTRL_SELECT) & ~PSP_CTRL_START;
			} while((key & ~(PSP_CTRL_UP | PSP_CTRL_DOWN | PSP_CTRL_LEFT | PSP_CTRL_RIGHT))== 0);
			key2 = key;
			while((key2 & key) == key)
			{
				key = key2;
				sceCtrlReadBufferPositive(&ctl, 1);
				key2 = (ctl.Buttons & ~PSP_CTRL_SELECT) & ~PSP_CTRL_START;
			}
			int i;
			for(i = 0; i < 10; i ++)
				if(i != *index && config.imgkey[i] == key)
				{
					config.imgkey[i] = config.imgkey[*index];
					break;
				}
			config.imgkey[*index] = key;
			ctrl_waitrelease();
			return win_menu_op_redraw;
		case PSP_CTRL_TRIANGLE:
			config.txtkey[*index] = 0;
			return win_menu_op_redraw;
		default:;
	}
	return win_menu_defcb(key, item, count, max_height, topindex, index);
}

static void scene_imgkey_predraw(p_win_menuitem item, dword index, dword topindex, dword max_height)
{
	char keyname[256];
	disp_rectangle(239 - DISP_FONTSIZE * 10, 128 - 7 * DISP_FONTSIZE, 240 + DISP_FONTSIZE * 10, 143 + 5 * DISP_FONTSIZE, 0xFFFFFFFF);
	disp_fillrect(240 - DISP_FONTSIZE * 10, 129 - 7 * DISP_FONTSIZE, 239 + DISP_FONTSIZE * 10, 128 - 6 * DISP_FONTSIZE, RGB(0x10, 0x30, 0x20));
	disp_fillrect(240 - DISP_FONTSIZE * 10, 127 - 6 * DISP_FONTSIZE, 239 + DISP_FONTSIZE * 10, 142 + 5 * DISP_FONTSIZE, RGB(0x10, 0x30, 0x20));
	disp_putstring(240 - DISP_FONTSIZE * 5, 129 - 7 * DISP_FONTSIZE, 0xFFFFFFFF, (const byte *)"按键设置   △ 删除");
	disp_line(240 - DISP_FONTSIZE * 10, 129 - 6 * DISP_FONTSIZE, 239 + DISP_FONTSIZE * 10, 129 - 6 * DISP_FONTSIZE, 0xFFFFFFFF);
	dword i;
	for (i = topindex; i < topindex + max_height; i ++)
	{
		conf_get_keyname(config.imgkey[i], keyname);
		disp_putstring(238 - DISP_FONTSIZE * 5, 131 - 6 * DISP_FONTSIZE + (1 + DISP_FONTSIZE) * i, 0xFFFFFFFF, (const byte *)keyname);
	}
}

static void scene_imgkey()
{
	t_win_menuitem item[11];
	strcpy(item[0].name, "上一张图");
	strcpy(item[1].name, "下一张图");
	strcpy(item[2].name, "缩放模式");
	strcpy(item[3].name, "缩小图片");
	strcpy(item[4].name, "放大图片");
	strcpy(item[5].name, "左旋90度");
	strcpy(item[6].name, "右旋90度");
	strcpy(item[7].name, "  信息栏");
	strcpy(item[8].name, "显示信息");
	strcpy(item[9].name, "退出浏览");
	strcpy(item[10].name, "缩放引擎");
	dword i;
	for(i = 0; i < 11; i ++)
	{
		item[i].width = 8;
		item[i].icolor = RGB(0xDF, 0xDF, 0xDF);
		item[i].selicolor = RGB(0xFF, 0xFF, 0x40);
		item[i].selrcolor = RGB(0x10, 0x30, 0x20);
		item[i].selbcolor = RGB(0x20, 0x20, 0xDF);
		item[i].data = NULL;
	}
	dword index;
	while((index = win_menu(240 - DISP_FONTSIZE * 10, 130 - 6 * DISP_FONTSIZE, 8, 11, item, 11, 0, 0, RGB(0x10, 0x30, 0x20), scene_imgkey_predraw, NULL, scene_imgkey_menucb)) != INVALID);
}

static t_win_menu_op scene_txtkey_menucb(dword key, p_win_menuitem item, dword count, dword max_height, dword topindex, dword * index)
{
	switch(key)
	{
		case (PSP_CTRL_SELECT | PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER):
			if(win_msgbox("是否退出软件?", "是", "否", 0xFFFFFFFF, 0xFFFFFFFF, RGB(0x18, 0x28, 0x50)))
				scene_exit();
			return win_menu_op_force_redraw;
		case PSP_CTRL_CIRCLE:
			disp_waitv();
			disp_duptocache();
			disp_rectangle(239 - DISP_FONTSIZE * 3, 135 - DISP_FONTSIZE / 2, 240 + DISP_FONTSIZE * 3, 136 + DISP_FONTSIZE / 2, 0xFFFFFFFF);
			disp_fillrect(240 - DISP_FONTSIZE * 3, 136 - DISP_FONTSIZE / 2, 239 + DISP_FONTSIZE * 3, 135 + DISP_FONTSIZE / 2, RGB(0x8, 0x18, 0x10));
			disp_putstring(240 - DISP_FONTSIZE * 3, 136 - DISP_FONTSIZE / 2, 0xFFFFFFFF, (const byte *)"请按对应按键");
			disp_flip();
			dword key, key2;
			SceCtrlData ctl;
			do {
				sceCtrlReadBufferPositive(&ctl, 1);
			} while(ctl.Buttons != 0);
			do {
				sceCtrlReadBufferPositive(&ctl, 1);
				key = (ctl.Buttons & ~PSP_CTRL_SELECT) & ~PSP_CTRL_START;
			} while((key & ~(PSP_CTRL_UP | PSP_CTRL_DOWN | PSP_CTRL_LEFT | PSP_CTRL_RIGHT))== 0);
			key2 = key;
			while((key2 & key) == key)
			{
				key = key2;
				sceCtrlReadBufferPositive(&ctl, 1);
				key2 = (ctl.Buttons & ~PSP_CTRL_SELECT) & ~PSP_CTRL_START;
			}
			int i;
			for(i = 0; i < 12; i ++)
				if(i != *index && config.txtkey[i] == key)
				{
					config.txtkey[i] = config.txtkey[*index];
					break;
				}
			config.txtkey[*index] = key;
			do {
				sceCtrlReadBufferPositive(&ctl, 1);
			} while(ctl.Buttons != 0);
			return win_menu_op_redraw;
		case PSP_CTRL_TRIANGLE:
			config.txtkey[*index] = 0;
			return win_menu_op_redraw;
		case PSP_CTRL_SQUARE:
			ctrl_waitrelease();
			return win_menu_op_ok;
		default:;
	}
	return win_menu_defcb(key, item, count, max_height, topindex, index);
}

static void scene_txtkey_predraw(p_win_menuitem item, dword index, dword topindex, dword max_height)
{
	char keyname[256];
	disp_rectangle(239 - DISP_FONTSIZE * 10, 128 - 7 * DISP_FONTSIZE, 240 + DISP_FONTSIZE * 10, 144 + 6 * DISP_FONTSIZE, 0xFFFFFFFF);
	disp_fillrect(240 - DISP_FONTSIZE * 10, 129 - 7 * DISP_FONTSIZE, 239 + DISP_FONTSIZE * 10, 128 - 6 * DISP_FONTSIZE, RGB(0x10, 0x30, 0x20));
	disp_fillrect(240 - DISP_FONTSIZE * 10, 127 - 6 * DISP_FONTSIZE, 239 + DISP_FONTSIZE * 10, 143 + 6 * DISP_FONTSIZE, RGB(0x10, 0x30, 0x20));
	disp_putstring(240 - DISP_FONTSIZE * 5, 129 - 7 * DISP_FONTSIZE, 0xFFFFFFFF, (const byte *)"按键设置   △ 删除");
	disp_line(240 - DISP_FONTSIZE * 10, 129 - 6 * DISP_FONTSIZE, 239 + DISP_FONTSIZE * 10, 129 - 6 * DISP_FONTSIZE, 0xFFFFFFFF);
	dword i;
	for (i = topindex; i < topindex + max_height; i ++)
	{
		conf_get_keyname(config.txtkey[i], keyname);
		disp_putstring(238 - DISP_FONTSIZE * 5, 131 - 6 * DISP_FONTSIZE + (1 + DISP_FONTSIZE) * i, 0xFFFFFFFF, (const byte *)keyname);
	}
}

static void scene_txtkey()
{
	t_win_menuitem item[12];
	strcpy(item[0].name, "选项菜单");
	strcpy(item[1].name, "  上一页");
	strcpy(item[2].name, "  下一页");
	strcpy(item[3].name, " 前100行");
	strcpy(item[4].name, " 后100行");
	strcpy(item[5].name, " 前500行");
	strcpy(item[6].name, " 后500行");
	strcpy(item[7].name, "  第一页");
	strcpy(item[8].name, "最后一页");
	strcpy(item[9].name, "上一文件");
	strcpy(item[10].name, "下一文件");
	strcpy(item[11].name, "退出阅读");
	dword i;
	for(i = 0; i < 12; i ++)
	{
		item[i].width = 8;
		item[i].icolor = RGB(0xDF, 0xDF, 0xDF);
		item[i].selicolor = RGB(0xFF, 0xFF, 0x40);
		item[i].selrcolor = RGB(0x10, 0x30, 0x20);
		item[i].selbcolor = RGB(0x20, 0x20, 0xDF);
		item[i].data = NULL;
	}
	dword index;
	while((index = win_menu(240 - DISP_FONTSIZE * 10, 130 - 6 * DISP_FONTSIZE, 8, 12, item, 12, 0, 0, RGB(0x10, 0x30, 0x20), scene_txtkey_predraw, NULL, scene_txtkey_menucb)) != INVALID);
}

static t_win_menu_op scene_options_menucb(dword key, p_win_menuitem item, dword count, dword max_height, dword topindex, dword * index)
{
	dword r, g, b;
	switch(key)
	{
		case (PSP_CTRL_SELECT | PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER):
			if(win_msgbox("是否退出软件?", "是", "否", 0xFFFFFFFF, 0xFFFFFFFF, RGB(0x18, 0x28, 0x50)))
				scene_exit();
			return win_menu_op_force_redraw;
		case PSP_CTRL_LEFT:
			switch (* index)
			{
			case 0:
				r = RGB_R(config.forecolor); g = RGB_G(config.forecolor); b = RGB_B(config.forecolor);
				if(r == 0)
					r = 255;
				else
					r --;
				config.forecolor = RGB(r, g, b);
				break;
			case 1:
				r = RGB_R(config.forecolor); g = RGB_G(config.forecolor); b = RGB_B(config.forecolor);
				if(g == 0)
					g = 255;
				else
					g --;
				config.forecolor = RGB(r, g, b);
				break;
			case 2:
				r = RGB_R(config.forecolor); g = RGB_G(config.forecolor); b = RGB_B(config.forecolor);
				if(b == 0)
					b = 255;
				else
					b --;
				config.forecolor = RGB(r, g, b);
				break;
			case 3:
				r = RGB_R(config.bgcolor); g = RGB_G(config.bgcolor); b = RGB_B(config.bgcolor);
				if(r == 0)
					r = 255;
				else
					r --;
				config.bgcolor = RGB(r, g, b);
				break;
			case 4:
				r = RGB_R(config.bgcolor); g = RGB_G(config.bgcolor); b = RGB_B(config.bgcolor);
				if(g == 0)
					g = 255;
				else
					g --;
				config.bgcolor = RGB(r, g, b);
				break;
			case 5:
				r = RGB_R(config.bgcolor); g = RGB_G(config.bgcolor); b = RGB_B(config.bgcolor);
				if(b == 0)
					b = 255;
				else
					b --;
				config.bgcolor = RGB(r, g, b);
				break;
			case 6:
				if(config.rowspace == 0)
					config.rowspace = 8;
				else
					config.rowspace --;
				break;
			case 7:
				config.infobar = !config.infobar;
				break;
			case 8:
				config.rlastrow = !config.rlastrow;
				break;
			case 9:
				config.autobm = !config.autobm;
				break;
			case 10:
				config.vertread = !config.vertread;
				break;
			case 11:
				if(config.encode == 0)
					config.encode = 2;
				else
					config.encode --;
				break;
			case 12:
				config.scrollbar = !config.scrollbar;
				break;
			}
			return win_menu_op_redraw;
		case PSP_CTRL_RIGHT:
			switch (* index)
			{
			case 0:
				r = RGB_R(config.forecolor), g = RGB_G(config.forecolor), b = RGB_B(config.forecolor);
				if(r == 255)
					r = 0;
				else
					r ++;
				config.forecolor = RGB(r, g, b);
				break;
			case 1:
				r = RGB_R(config.forecolor), g = RGB_G(config.forecolor), b = RGB_B(config.forecolor);
				if(g == 255)
					g = 0;
				else
					g ++;
				config.forecolor = RGB(r, g, b);
				break;
			case 2:
				r = RGB_R(config.forecolor), g = RGB_G(config.forecolor), b = RGB_B(config.forecolor);
				if(b == 255)
					b = 0;
				else
					b ++;
				config.forecolor = RGB(r, g, b);
				break;
			case 3:
				r = RGB_R(config.bgcolor), g = RGB_G(config.bgcolor), b = RGB_B(config.bgcolor);
				if(r == 255)
					r = 0;
				else
					r ++;
				config.bgcolor = RGB(r, g, b);
				break;
			case 4:
				r = RGB_R(config.bgcolor), g = RGB_G(config.bgcolor), b = RGB_B(config.bgcolor);
				if(g == 255)
					g = 0;
				else
					g ++;
				config.bgcolor = RGB(r, g, b);
				break;
			case 5:
				r = RGB_R(config.bgcolor), g = RGB_G(config.bgcolor), b = RGB_B(config.bgcolor);
				if(b == 255)
					b = 0;
				else
					b ++;
				config.bgcolor = RGB(r, g, b);
				break;
			case 6:
				if(config.rowspace == 8)
					config.rowspace = 0;
				else
					config.rowspace ++;
				break;
			case 7:
				config.infobar = !config.infobar;
				break;
			case 8:
				config.rlastrow = !config.rlastrow;
				break;
			case 9:
				config.autobm = !config.autobm;
				break;
			case 10:
				config.vertread = !config.vertread;
				break;
			case 11:
				if(config.encode == 2)
					config.encode = 0;
				else
					config.encode ++;
				break;
			case 12:
				config.scrollbar = !config.scrollbar;
				break;
			}
			return win_menu_op_redraw;
		case PSP_CTRL_CIRCLE:
			return win_menu_op_continue;
		case PSP_CTRL_SQUARE:
			scene_txtkey();
			return win_menu_op_ok;
		default:;
	}
	return win_menu_defcb(key, item, count, max_height, topindex, index);
}

static void scene_options_predraw(p_win_menuitem item, dword index, dword topindex, dword max_height)
{
	char number[5];
	disp_rectangle(239 - DISP_FONTSIZE * 6, 128 - 7 * DISP_FONTSIZE, 240 + DISP_FONTSIZE * 6, 145 + 7 * DISP_FONTSIZE, 0xFFFFFFFF);
	disp_fillrect(240 - DISP_FONTSIZE * 6, 129 - 7 * DISP_FONTSIZE, 239 + DISP_FONTSIZE * 6, 128 - 6 * DISP_FONTSIZE, RGB(0x10, 0x30, 0x20));
	disp_putstring(240 - DISP_FONTSIZE * 4, 129 - 7 * DISP_FONTSIZE, 0xFFFFFFFF, (const byte *)"选项   □ 按键设置");
	disp_line(240 - DISP_FONTSIZE * 6, 129 - 6 * DISP_FONTSIZE, 239 + DISP_FONTSIZE * 6, 129 - 6 * DISP_FONTSIZE, 0xFFFFFFFF);
	disp_fillrect(241, 130 - 6 * DISP_FONTSIZE, 239 + DISP_FONTSIZE * 6, 144 + 7 * DISP_FONTSIZE, RGB(0x10, 0x30, 0x20));
	disp_fillrect(280, 135 - 6 * DISP_FONTSIZE, 310, 165 - 6 * DISP_FONTSIZE, config.forecolor);
	disp_fillrect(280, 138 - 3 * DISP_FONTSIZE, 310, 168 - 3 * DISP_FONTSIZE, config.bgcolor);
	memset(number, ' ', 4);
	utils_dword2string(RGB_R(config.forecolor), number, 4);
	disp_putstring(242, 131 - 6 * DISP_FONTSIZE, 0xFFFFFFFF, (const byte *)number);
	memset(number, ' ', 4);
	utils_dword2string(RGB_G(config.forecolor), number, 4);
	disp_putstring(242, 132 - 5 * DISP_FONTSIZE, 0xFFFFFFFF, (const byte *)number);
	memset(number, ' ', 4);
	utils_dword2string(RGB_B(config.forecolor), number, 4);
	disp_putstring(242, 133 - 4 * DISP_FONTSIZE, 0xFFFFFFFF, (const byte *)number);
	memset(number, ' ', 4);
	utils_dword2string(RGB_R(config.bgcolor), number, 4);
	disp_putstring(242, 134 - 3 * DISP_FONTSIZE, 0xFFFFFFFF, (const byte *)number);
	memset(number, ' ', 4);
	utils_dword2string(RGB_G(config.bgcolor), number, 4);
	disp_putstring(242, 135 - 2 * DISP_FONTSIZE, 0xFFFFFFFF, (const byte *)number);
	memset(number, ' ', 4);
	utils_dword2string(RGB_B(config.bgcolor), number, 4);
	disp_putstring(242, 136 - DISP_FONTSIZE, 0xFFFFFFFF, (const byte *)number);
	memset(number, ' ', 4);
	utils_dword2string(config.rowspace, number, 4);
	disp_putstring(242, 137, 0xFFFFFFFF, (const byte *)number);
	disp_putstring(260, 138 + DISP_FONTSIZE, 0xFFFFFFFF, (const byte *)(config.infobar ? "是" : "否"));
	disp_putstring(260, 139 + 2 * DISP_FONTSIZE, 0xFFFFFFFF, (const byte *)(config.rlastrow ? "是" : "否"));
	disp_putstring(260, 140 + 3 * DISP_FONTSIZE, 0xFFFFFFFF, (const byte *)(config.autobm ? "是" : "否"));
	disp_putstring(260, 141 + 4 * DISP_FONTSIZE, 0xFFFFFFFF, (const byte *)(config.vertread ? "是" : "否"));
	switch(config.encode)
	{
		case conf_encode_big5:
			disp_putstring(260, 142 + 5 * DISP_FONTSIZE, 0xFFFFFFFF, (const byte *)"BIG5");
			break;
		case conf_encode_sjis:
			disp_putstring(260, 142 + 5 * DISP_FONTSIZE, 0xFFFFFFFF, (const byte *)"SJIS");
			break;
		default:
			disp_putstring(260, 142 + 5 * DISP_FONTSIZE, 0xFFFFFFFF, (const byte *)"GBK");
			break;
	}
	disp_putstring(260, 143 + 6 * DISP_FONTSIZE, 0xFFFFFFFF, (const byte *)(config.scrollbar ? "是" : "否"));
}

static void scene_options()
{
	t_win_menuitem item[13];
	dword i;
	strcpy(item[0].name, "字体颜色：红");
	strcpy(item[1].name, "          绿");
	strcpy(item[2].name, "          蓝");
	strcpy(item[3].name, "背景颜色：红");
	strcpy(item[4].name, "          绿");
	strcpy(item[5].name, "          蓝");
	strcpy(item[6].name, "      行间距");
	strcpy(item[7].name, "      信息栏");
	strcpy(item[8].name, "翻页保留一行");
	strcpy(item[9].name, "自动保存书签");
	strcpy(item[10].name, "    文字竖看");
	strcpy(item[11].name, "    文字编码");
	strcpy(item[12].name, "      滚动条");
	for(i = 0; i < 13; i ++)
	{
		item[i].width = 12;
		item[i].icolor = RGB(0xDF, 0xDF, 0xDF);
		item[i].selicolor = RGB(0xFF, 0xFF, 0x40);
		item[i].selrcolor = RGB(0x10, 0x30, 0x20);
		item[i].selbcolor = RGB(0x20, 0x20, 0xDF);
		item[i].data = NULL;
	}
	dword index;
	bool orgvert = config.vertread;
	dword orgrowspace = config.rowspace;
	bool orgibar = config.infobar;
	while((index = win_menu(240 - DISP_FONTSIZE * 6, 130 - 6 * DISP_FONTSIZE, 12, 13, item, 13, 0, 0, RGB(0x10, 0x30, 0x20), scene_options_predraw, NULL, scene_options_menucb)) != INVALID);
	if(orgibar != config.infobar || orgvert != config.vertread || orgrowspace != config.rowspace)
	{
		if(!config.vertread)
		{
			if(config.infobar)
				rowsperpage = (272 - DISP_FONTSIZE) / (config.rowspace + DISP_FONTSIZE);
			else
				rowsperpage = 272 / (config.rowspace + DISP_FONTSIZE);
		}
		else
		{
			if(config.infobar)
				rowsperpage = (480 - DISP_FONTSIZE) / (config.rowspace + DISP_FONTSIZE);
			else
				rowsperpage = 480 / (config.rowspace + DISP_FONTSIZE);
		}
	}
}

static t_win_menu_op scene_bookmark_menucb(dword key, p_win_menuitem item, dword count, dword max_height, dword topindex, dword * index)
{
	switch(key)
	{
		case (PSP_CTRL_SELECT | PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER):
			if(win_msgbox("是否退出软件?", "是", "否", 0xFFFFFFFF, 0xFFFFFFFF, RGB(0x18, 0x28, 0x50)))
				scene_exit();
			return win_menu_op_force_redraw;
		case PSP_CTRL_SQUARE:
			strcpy(item[*index].name, "       ");
			bm->row[(*index) + 1] = *(dword *)item[*index].data;
			utils_dword2string(bm->row[(*index) + 1], item[*index].name, 7);
			bookmark_save(bm);
			return win_menu_op_redraw;
		case PSP_CTRL_TRIANGLE:
			bm->row[(*index) + 1] = INVALID;
			strcpy(item[*index].name, "  NONE ");
			bookmark_save(bm);
			return win_menu_op_redraw;
		case PSP_CTRL_CIRCLE:
			if(bm->row[(*index) + 1] != INVALID)
			{
				*(dword *)item[*index].data = bm->row[(*index) + 1];
				return win_menu_op_ok;
			}
			else
				return win_menu_op_continue;
		default:;
	}
	return win_menu_defcb(key, item, count, max_height, topindex, index);
}

static void scene_bookmark_predraw(p_win_menuitem item, dword index, dword topindex, dword max_height)
{
	disp_rectangle(63, 66 - DISP_FONTSIZE, 416, 70 + (1 + DISP_FONTSIZE) * 9, 0xFFFFFFFF);
	disp_fillrect(64, 67 - DISP_FONTSIZE, 415, 66, RGB(0x30, 0x60, 0x30));
	disp_putstring(75, 67 - DISP_FONTSIZE, 0xFFFFFFFF, (const byte *)"书签      ○读取  ×取消  □保存  △删除");
	disp_fillrect(68 + 7 * DISP_FONTSIZE / 2, 68, 415, 69 + (1 + DISP_FONTSIZE) * 9, RGB(0x30, 0x60, 0x30));
	disp_line(64, 67, 415, 67, 0xFFFFFFFF);
	disp_line(67 + 7 * DISP_FONTSIZE / 2, 68, 67 + 7 * DISP_FONTSIZE / 2, 69 + (1 + DISP_FONTSIZE) * 9, 0xFFFFFFFF);
	++ index;
	if(bm->row[index] < fs->row_count && fs_file_get_type(fs->filename) != fs_filetype_unknown)
	{
		t_fs_text preview;
		memset(&preview, 0, sizeof(t_fs_text));
		dword row = bm->row[index];
		preview.buf = fs->rows[row >> 10][row & 0x3FF].start;
		if(fs->buf + fs->size - preview.buf < 8 * ((347 - 7 * DISP_FONTSIZE / 2) / (DISP_FONTSIZE / 2)))
			preview.size = fs->buf + fs->size - preview.buf;
		else
			preview.size = 8 * ((347 - 7 * DISP_FONTSIZE / 2) / (DISP_FONTSIZE / 2));
		fs_format_text(&preview, ((347 - 7 * DISP_FONTSIZE / 2) / (DISP_FONTSIZE / 2)));
		if(preview.rows[0] != NULL)
		{
			dword i;
			if(preview.row_count > 8)
				preview.row_count = 8;
			for(i = 0; i < preview.row_count; i ++)
				disp_putnstring(70 + 7 * DISP_FONTSIZE / 2, 72 + (2 + DISP_FONTSIZE) * i, 0xFFFFFFFF, (const byte *)preview.rows[0][i].start, preview.rows[0][i].count);
			free((void *)preview.rows[0]);
		}
	}
}

static void scene_bookmark()
{
	char archname[256];
	if(where == scene_in_zip || where == scene_in_chm)
	{
		strcpy(archname, config.shortpath);
		strcat(archname, fs->filename);
	}
	else
		strcpy(archname, fs->filename);
	bm = bookmark_open(archname);
	if(bm == NULL)
	{
		disp_waitv();
		disp_rectangle(187, 127, 292, 144, 0xFFFFFFFF);
		disp_fillrect(188, 128, 291, 143, 0);
		disp_putstring(240 - 13 * DISP_FONTSIZE / 4, 136 - (DISP_FONTSIZE / 2), 0xFFFFFFFF, (const byte *)"无法打开书签!");
		disp_flip();
		ctrl_waitany();
		return;
	}
	dword i;
	t_win_menuitem item[9];
	for(i = 0; i < 9; i ++)
	{
		if(bm->row[i + 1] != INVALID)
		{
			strcpy(item[i].name, "       ");
			utils_dword2string(bm->row[i + 1], item[i].name, 7);
		}
		else
			strcpy(item[i].name, "  NONE ");
		item[i].width = 7;
		item[i].icolor = RGB(0xDF, 0xDF, 0xDF);
		item[i].selicolor = RGB(0xFF, 0xFF, 0x40);
		item[i].selrcolor = RGB(0x10, 0x30, 0x20);
		item[i].selbcolor = RGB(0x20, 0x20, 0xDF);
		item[i].data = (dword *)&fs->crow;
	}
	dword index;
	if((index = win_menu(64, 68, 7, 9, item, 9, 0, 0, RGB(0x10, 0x30, 0x20), scene_bookmark_predraw, NULL, scene_bookmark_menucb)) != INVALID);
	bookmark_close(bm);
	bm = NULL;
}

static void scene_mountrbkey(dword * ctlkey, dword * ku, dword * kd, dword * kl, dword * kr)
{
	if(config.vertread)
	{
		memcpy(ctlkey, config.txtkey, 12 * sizeof(dword));
		dword i;
		for (i = 0; i < 12; i ++)
		{
			if((config.txtkey[i] & PSP_CTRL_LEFT) > 0)
				ctlkey[i] = (ctlkey[i] & ~PSP_CTRL_LEFT) | PSP_CTRL_UP;
			if((config.txtkey[i] & PSP_CTRL_RIGHT) > 0)
				ctlkey[i] = (ctlkey[i] & ~PSP_CTRL_RIGHT) | PSP_CTRL_DOWN;
			if((config.txtkey[i] & PSP_CTRL_UP) > 0)
				ctlkey[i] = (ctlkey[i] & ~PSP_CTRL_UP) | PSP_CTRL_RIGHT;
			if((config.txtkey[i] & PSP_CTRL_DOWN) > 0)
				ctlkey[i] = (ctlkey[i] & ~PSP_CTRL_DOWN) | PSP_CTRL_LEFT;
		}
		*ku = PSP_CTRL_RIGHT;
		*kd = PSP_CTRL_LEFT;
		*kl = PSP_CTRL_UP;
		*kr = PSP_CTRL_DOWN;
	}
	else
	{
		memcpy(ctlkey, config.txtkey, 12 * sizeof(dword));
		*ku = PSP_CTRL_UP;
		*kd = PSP_CTRL_DOWN;
		*kl = PSP_CTRL_LEFT;
		*kr = PSP_CTRL_RIGHT;
	}
}

static dword scene_readbook(dword selidx)
{
	dword ctlkey[12], ku, kd, kl, kr;
	dword cidx, rrow;
	char tr[8], * trow;
	bool needrf = true, needrp = true, needrb = false;
	char filename[256], archname[256];
	scene_mountrbkey(ctlkey, &ku, &kd, &kl, &kr);
	while(1)
	{
		if(needrf)
		{
			if(where == scene_in_zip || where == scene_in_chm)
				strcpy(filename, filelist[selidx].compname);
			else
			{
				strcpy(filename, config.shortpath);
				strcat(filename, filelist[selidx].shortname);
			}
			if(where == scene_in_zip || where == scene_in_chm)
			{
				strcpy(archname, config.shortpath);
				strcat(archname, filename);
			}
			else
				strcpy(archname, filename);
			rrow = bookmark_autoload(archname);
			if(fs != NULL)
			{
				fs_close_text(fs);
				fs = NULL;
			}
			power_set_clock(222, 111);
			if(where == scene_in_zip)
				fs = fs_open_text_in_zip(config.shortpath, filename, (t_fs_filetype)filelist[selidx].data, (config.vertread ? (config.scrollbar ? 534 : 544) : (config.scrollbar ? 950 : 960)) / DISP_FONTSIZE, config.encode);
			else if(where == scene_in_chm)
				fs = fs_open_text_in_chm(config.shortpath, filename, (t_fs_filetype)filelist[selidx].data, (config.vertread ? (config.scrollbar ? 534 : 544) : (config.scrollbar ? 950 : 960)) / DISP_FONTSIZE, config.encode);
			else if((t_fs_filetype)filelist[selidx].data == fs_filetype_unknown)
				fs = fs_open_binary(filename, config.vertread);
			else
				fs = fs_open_text(filename, (t_fs_filetype)filelist[selidx].data, (config.vertread ? (config.scrollbar ? 534 : 544) : (config.scrollbar ? 950 : 960)) / DISP_FONTSIZE, config.encode);
			if(fs == NULL)
				return selidx;
			if(needrb && (t_fs_filetype)filelist[selidx].data != fs_filetype_unknown)
			{
				fs->crow = 1;
				while(fs->crow < fs->row_count && (fs->rows[fs->crow >> 10] + (fs->crow & 0x3FF))->start - fs->buf <= rrow)
					fs->crow ++;
				fs->crow --;
				needrb = false;
			}
			else
				fs->crow = rrow;
			trow = &tr[utils_dword2string(fs->row_count, tr, 7)];
			if(mp3_paused())
				power_set_clock(50, 25);
			needrf = false;
		}
		if(needrp)
		{
			char ci[8] = "       ", cr[300];
			disp_waitv();
			if(!bg_display())
				disp_fillvram(config.bgcolor);
			if(!config.vertread)
			{
				for(cidx = 0; cidx < rowsperpage && fs->crow + cidx < fs->row_count; cidx ++)
				{
					p_fs_textrow tr = fs->rows[(fs->crow + cidx) >> 10] + ((fs->crow + cidx) & 0x3FF);
					disp_putnstring(0, (DISP_FONTSIZE + config.rowspace) * cidx, config.forecolor, (const byte *)tr->start, (int)tr->count);
				}
				if(config.infobar)
				{
					disp_line(0, 271 - DISP_FONTSIZE, 479, 271 - DISP_FONTSIZE, config.forecolor);
					utils_dword2string(fs->crow + 1, ci, 7);
					sprintf(cr, "%s/%s  %s  %s", ci, trow, fs->isucs ? "UCS " : conf_encodename(config.encode), filelist[selidx].compname);
					disp_putnstring(0, 272 - DISP_FONTSIZE, config.forecolor, (const byte *)cr, 960 / DISP_FONTSIZE);
				}
			}
			else
			{
				for(cidx = 0; cidx < rowsperpage && fs->crow + cidx < fs->row_count; cidx ++)
				{
					p_fs_textrow tr = fs->rows[(fs->crow + cidx) >> 10] + ((fs->crow + cidx) & 0x3FF);
					disp_putnstringvert(480 - (DISP_FONTSIZE + config.rowspace) * cidx, 0, config.forecolor, (const byte *)tr->start, (int)tr->count);
				}
				if(config.infobar)
				{
					disp_line(DISP_FONTSIZE, 0, DISP_FONTSIZE, 272, config.forecolor);
					utils_dword2string(fs->crow + 1, ci, 7);
					sprintf(cr, "%s/%s  %s  %s", ci, trow, fs->isucs ? "UCS " : conf_encodename(config.encode), filelist[selidx].compname);
					disp_putnstringvert(DISP_FONTSIZE - 1, 0, config.forecolor, (const byte *)cr, 544 / DISP_FONTSIZE);
				}
			}
			if(config.scrollbar)
			{
				if(config.vertread)
				{
					dword sleft = DISP_FONTSIZE + 1, slen = 479 - sleft, bsize = 2 + (slen - 2) * rowsperpage / fs->row_count, endp = 479 - (slen - 2) * fs->crow / fs->row_count, startp;
					if(endp - bsize < sleft)
						endp = sleft + bsize;
					startp = endp - bsize;
					disp_line(sleft, 267, 479, 267, config.forecolor);
					disp_fillrect(startp, 268, endp, 271, config.forecolor);
				}
				else
				{
					dword slen = config.infobar ? (270 - DISP_FONTSIZE) : 271, bsize = 2 + (slen - 2) * rowsperpage / fs->row_count, startp = (slen - 2) * fs->crow / fs->row_count, endp;
					if(startp + bsize > slen)
						startp = slen - bsize;
					endp = startp + bsize;
					disp_line(475, 0, 475, slen, config.forecolor);
					disp_fillrect(476, startp, 479, endp, config.forecolor);
				}
			}
			disp_flip();
			needrp = false;
		}
		dword key = ctrl_waitany();
		if(key == (PSP_CTRL_SELECT | PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER))
		{
			if(win_msgbox("是否退出软件?", "是", "否", 0xFFFFFFFF, 0xFFFFFFFF, RGB(0x18, 0x28, 0x50)))
				scene_exit();
		}
		else if(key == ctlkey[11])
		{
			power_set_clock(222, 111);
			if(config.autobm)
				bookmark_autosave(archname, fs->crow);
			fs_close_text(fs);
			fs = NULL;
			return selidx;
		}
		else if(key == ctlkey[0])
		{
			scene_bookmark();
			if (fs->crow >= fs->row_count)
				fs->crow = fs->row_count - 1;
			needrp = true;
		}
		else if(key == PSP_CTRL_START)
		{
			scene_mp3bar();
			needrp = true;
		}
		else if(key == PSP_CTRL_SELECT)
		{
			bool lastvert = config.vertread;
			bool lastscrollbar = config.scrollbar;
			t_conf_encode lastencode = config.encode;
			scene_options();
			scene_mountrbkey(ctlkey, &ku, &kd, &kl, &kr);
			if(lastvert != config.vertread || lastscrollbar != config.scrollbar)
			{
				rrow = (fs->rows[fs->crow >> 10] + (fs->crow & 0x3FF))->start - fs->buf;
				needrb = needrf = true;
			}
			else if(!fs->isucs && lastencode != config.encode)
			{
				rrow = fs->crow;
				needrf = true;
			}
			needrp = true;
		}
		else if(key == CTRL_ANALOG)
		{
			if(!config.vertread)
			{
				int x, y;
				ctrl_analog(&x, &y);
				y = (y + 1) / 31;
				if(y < 0)
				{
					if (fs->crow > -y)
						fs->crow -= -y;
					needrp = true;
				}
				else if(y > 0)
				{
					fs->crow += y;
					if (fs->crow >= fs->row_count)
						fs->crow = fs->row_count - 1;
					needrp = true;
				}
			}
			else
			{
				int x, y;
				ctrl_analog(&x, &y);
				x = (x + 1) / 31;
				if(x > 0)
				{
					if (fs->crow > x)
						fs->crow -= x;
					needrp = true;
				}
				else if(x < 0)
				{
					fs->crow += -x;
					if (fs->crow >= fs->row_count)
						fs->crow = fs->row_count - 1;
					needrp = true;
				}
			}
		}
		else if(key == ku)
		{
			if (fs->crow > 0)
				fs->crow --;
			needrp = true;
		}
		else if(key == kd)
		{
			if (fs->crow < fs->row_count - 1)
				fs->crow ++;
			needrp = true;
		}
		else if(key == ctlkey[1] || key == kl)
		{
			if (fs->crow > (config.rlastrow ? (rowsperpage - 1) : rowsperpage))
				fs->crow -= (config.rlastrow ? (rowsperpage - 1) : rowsperpage);
			else
				fs->crow = 0;
			needrp = true;
		}
		else if(key == ctlkey[2] || key == kr)
		{
			fs->crow += (config.rlastrow ? (rowsperpage - 1) : rowsperpage);
			if (fs->crow >= fs->row_count)
				fs->crow = fs->row_count - 1;
			needrp = true;
		}
		else if(key == ctlkey[5])
		{
			if (fs->crow > 500)
				fs->crow -= 500;
			else
				fs->crow = 0;
			needrp = true;
		}
		else if(key == ctlkey[6])
		{
			fs->crow += 500;
			if (fs->crow >= fs->row_count)
				fs->crow = fs->row_count - 1;
			needrp = true;
		}
		else if(key == ctlkey[3])
		{
			if (fs->crow > 100)
				fs->crow -= 100;
			else
				fs->crow = 0;
			needrp = true;
		}
		else if(key == ctlkey[4])
		{
			fs->crow += 100;
			if (fs->crow >= fs->row_count)
				fs->crow = fs->row_count - 1;
			needrp = true;
		}
		else if(key == ctlkey[7])
		{
			if(fs->crow != 0)
			{
				fs->crow = 0;
				needrp = true;
			}
		}
		else if(key == ctlkey[8])
		{
			if (fs->row_count >= rowsperpage)
				fs->crow = fs->row_count - rowsperpage;
			else
				fs->crow = 0;
			needrp = true;
		}
		else if(key == ctlkey[9])
		{
			dword orgidx = selidx;
			if(config.autobm)
				bookmark_autosave(archname, fs->crow);
			do {
				if(selidx > 0)
					selidx --;
				else
					selidx = filecount - 1;
			} while(!fs_is_txtbook((t_fs_filetype)filelist[selidx].data));
			if(selidx != orgidx)
			{
				if(config.autobm)
					bookmark_autosave(archname, fs->crow);
				needrf = needrp = true;
				needrb = false;
			}
		}
		else if(key == ctlkey[10])
		{
			dword orgidx = selidx;
			do {
				if(selidx < filecount - 1)
					selidx ++;
				else
					selidx = 0;
			} while(!fs_is_txtbook((t_fs_filetype)filelist[selidx].data));
			if(selidx != orgidx)
			{
				if(config.autobm)
					bookmark_autosave(archname, fs->crow);
				needrf = needrp = true;
				needrb = false;
			}
		}
		else
			needrp = needrb = needrf = false;
	}
	power_set_clock(222, 111);
	if(config.autobm)
		bookmark_autosave(archname, fs->crow);
	fs_close_text(fs);
	fs = NULL;
	return selidx;
}

static dword scene_readimage(dword selidx)
{
	dword width, height, w2 = 0, h2 = 0, paintleft = 0, painttop = 0;
	dword * imgdata = NULL, * imgshow = NULL;
	dword bgcolor;
	dword oldangle = 0;
	char filename[256];
	int curtop = 0, curleft = 0;
	bool needrf = true, needrc = true, needrp = true, showinfo = false;
	int imgh;
	power_set_clock(333, 166);
	if(config.imginfobar)
		imgh = 272 - DISP_FONTSIZE;
	else
		imgh = 272;
	while(1) {
		if(needrf)
		{
			if(imgshow != NULL && imgshow != imgdata)
			{
				free((void *)imgshow);
				imgshow = NULL;
			}
			if(imgdata != NULL)
			{
				free((void *)imgdata);
				imgdata = NULL;
			}
			strcpy(filename, config.shortpath);
			strcat(filename, filelist[selidx].shortname);
			switch((t_fs_filetype)filelist[selidx].data)
			{
			case fs_filetype_png:
				if(image_readpng(filename, &width, &height, &imgdata, &bgcolor) != 0)
				{
					power_set_clock(222, 111);
					return selidx;
				}
				break;
			case fs_filetype_gif:
				if(image_readgif(filename, &width, &height, &imgdata, &bgcolor) != 0)
				{
					power_set_clock(222, 111);
					return selidx;
				}
				break;
			case fs_filetype_jpg:
				if(image_readjpg(filename, &width, &height, &imgdata, &bgcolor) != 0)
				{
					power_set_clock(222, 111);
					return selidx;
				}
				break;
			case fs_filetype_bmp:
				if(image_readbmp(filename, &width, &height, &imgdata, &bgcolor) != 0)
				{
					power_set_clock(222, 111);
					return selidx;
				}
				break;
			case fs_filetype_tga:
				if(image_readtga(filename, &width, &height, &imgdata, &bgcolor) != 0)
				{
					power_set_clock(222, 111);
					return selidx;
				}
				break;
			default:
				power_set_clock(222, 111);
				return selidx;
			}
			needrf = false;
			oldangle = 0;
		}
		if(needrc)
		{
			image_rotate(imgdata, &width, &height, oldangle, (dword)config.rotate * 90);
			oldangle = (dword)config.rotate * 90;
			if(config.fit > 0 && (config.fit != conf_fit_custom || config.scale != 100))
			{
				if(imgshow != NULL && imgshow != imgdata)
					free((void *)imgshow);
				if(config.fit == conf_fit_custom)
				{
					w2 = width * config.scale / 100;
					h2 = height * config.scale / 100;
				}
				else if(config.fit == conf_fit_width)
				{
					config.scale = 480 / width;
					if(config.scale > 200)
						config.scale = (config.scale / 50) * 50;
					else
					{
						config.scale = (config.scale / 10) * 10;
						if(config.scale < 10)
							config.scale = 10;
					}
					w2 = 480;
					h2 = height * 480 / width;
				}
				else
				{
					config.scale = 272 / height;
					if(config.scale > 200)
						config.scale = (config.scale / 50) * 50;
					else
					{
						config.scale = (config.scale / 10) * 10;
						if(config.scale < 10)
							config.scale = 10;
					}
					h2 = imgh;
					w2 = width * 272 / height;
				}
				imgshow = (dword *)malloc(sizeof(dword) * w2 * h2);
				if(imgshow != NULL)
				{
					if(config.bicubic)
						image_zoom_bicubic(imgdata, width, height, imgshow, w2, h2);
					else
						image_zoom_bilinear(imgdata, width, height, imgshow, w2, h2);
				}
				else
				{
					imgshow = imgdata;
					w2 = width;
					h2 = height;
				}
			}
			else
			{
				config.scale = 100;
				imgshow = imgdata;
				w2 = width;
				h2 = height;
			}
			if(w2 < 480)
				paintleft = (480 - w2) / 2;
			else
				paintleft = 0;
			if(h2 < 272)
				painttop = (272 - h2) / 2;
			else
				painttop = 0;
			needrc = false;
		}
		if(needrp)
		{
			disp_waitv();
			disp_fillvram(bgcolor);
				disp_putimage(paintleft, painttop, w2, h2, curleft, curtop, imgshow);
			if(config.imginfobar || showinfo)
			{
				char infostr[64];
				switch(config.fit)
				{
				case conf_fit_custom:
					sprintf(infostr, "%dx%d  %d%%  旋转角度 %d  %s", (int)w2, (int)h2, (int)config.scale, (int)oldangle, config.bicubic ? "三次立方" : "两次线性");
					break;
				case conf_fit_width:
					sprintf(infostr, "%dx%d  适应宽度  旋转角度 %d  %s", (int)w2, (int)h2, (int)oldangle, config.bicubic ? "三次立方" : "两次线性");
					break;
				case conf_fit_height:
					sprintf(infostr, "%dx%d  适应高度  旋转角度 %d  %s", (int)w2, (int)h2, (int)oldangle, config.bicubic ? "三次立方" : "两次线性");
					break;
				default:
					sprintf(infostr, "%dx%d  原始大小  旋转角度 %d  %s", (int)w2, (int)h2, (int)oldangle, config.bicubic ? "三次立方" : "两次线性");
					break;
				}
				int ilen = strlen(infostr);
				if(config.imginfobar)
				{
					disp_fillrect(0, 272 - DISP_FONTSIZE, 479, 271, 0);
					disp_putnstring(0, 272 - DISP_FONTSIZE, 0xFFFFFFFF, (const byte *)filename, 960 / DISP_FONTSIZE - ilen - 1);
					disp_putnstring(480 - DISP_FONTSIZE / 2 * ilen, 272 - DISP_FONTSIZE, 0xFFFFFFFF, (const byte *)infostr, ilen);
				}
				else
				{
					disp_fillrect(11, 11, 10 + DISP_FONTSIZE / 2 * ilen, 10 + DISP_FONTSIZE, 0);
					disp_putnstring(11, 11, 0xFFFFFFFF, (const byte *)infostr, ilen);
				}
			}
			disp_flip();
			needrp = false;
		}
		dword key = (showinfo ? ctrl_read_cont() : ctrl_waitany());
		if(showinfo && (key & PSP_CTRL_CIRCLE) == 0)
		{
			needrp = true;
			showinfo = false;
		}
		if(key == (PSP_CTRL_SELECT | PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER))
		{
			if(win_msgbox("是否退出软件?", "是", "否", 0xFFFFFFFF, 0xFFFFFFFF, RGB(0x18, 0x28, 0x50)))
				scene_exit();
		}
		else if(key == PSP_CTRL_SELECT)
		{
			needrp = true;
			scene_imgkey();
		}
		else if(key == PSP_CTRL_START)
		{
			scene_mp3bar();
			needrp = true;
		}
		else if(key == CTRL_ANALOG)
		{
			int x, y, orgtop = curtop, orgleft = curleft;
			ctrl_analog(&x, &y);
			x = x / 31 * 4;
			y = y / 31 * 4;
			curtop += y;
			if(curtop + imgh > h2)
				curtop = (int)h2 - imgh;
			if(curtop < 0)
				curtop = 0;
			curleft += x;
			if(curleft + 480 > w2)
				curleft = (int)w2 - 480;
			if(curleft < 0)
				curleft = 0;
			needrp = (orgtop != curtop || orgleft != curleft);
		}
		else if(key == PSP_CTRL_LEFT)
		{
			if(curleft > 0)
			{
				curleft -= 8;
				if(curleft < 0)
					curleft = 0;
				needrp = true;
			}
		}
		else if(key == PSP_CTRL_UP)
		{
			if(curtop > 0)
			{
				curtop -= 8;
				if(curleft < 0)
					curleft = 0;
				needrp = true;
			}
		}
		else if(key == PSP_CTRL_RIGHT)
		{
			if(w2 > 480 && curleft < w2 - 480)
			{
				curleft += 8;
				if(curleft > w2 - 480)
					curleft = w2 - 480;
				needrp = true;
			}
		}
		else if(key == PSP_CTRL_DOWN)
		{
			if(h2 > imgh && curtop < h2 - imgh)
			{
				curtop += 8;
				if(curtop > h2 - imgh)
					curtop = h2 - imgh;
				needrp = true;
			}
		}
		else if(key == config.imgkey[0])
		{
			dword orgidx = selidx;
			do {
				if(selidx > 0)
					selidx --;
				else
					selidx = filecount - 1;
			} while(!fs_is_image((t_fs_filetype)filelist[selidx].data));
			if(selidx != orgidx)
			{
				curtop = 0; curleft = 0;
				needrf = needrc = needrp = true;
			}
		}
		else if(key == config.imgkey[1])
		{
			dword orgidx = selidx;
			do {
				if(selidx < filecount - 1)
					selidx ++;
				else
					selidx = 0;
			} while(!fs_is_image((t_fs_filetype)filelist[selidx].data));
			if(selidx != orgidx)
			{
				curtop = 0; curleft = 0;
				needrf = needrc = needrp = true;
			}
		}
		else if(key == config.imgkey[2])
		{
			config.fit ++;
			if(config.fit > 2)
				config.fit = 0;
			curtop = 0; curleft = 0;
			needrc = needrp = true;
		}
		else if(key == config.imgkey[10])
		{
			config.bicubic = !config.bicubic;
			curtop = 0; curleft = 0;
			needrc = needrp = true;
		}
		else if(key == config.imgkey[7])
		{
			config.imginfobar = !config.imginfobar;
			if(config.imginfobar)
				imgh = 272 - DISP_FONTSIZE;
			else
				imgh = 272;
			if(h2 > imgh && curtop > h2 - imgh)
				curtop = h2 - imgh;
			needrc = (config.fit == conf_fit_height);
			needrp = true;
		}
		else if(key == config.imgkey[9])
		{
			power_set_clock(222, 111);
			if(imgshow != NULL && imgshow != imgdata)
				free((void *)imgshow);
			if(imgdata != NULL)
				free((void *)imgdata);
			return selidx;
		}
		else if(key == config.imgkey[8])
		{
			if(!showinfo)
			{
				needrp = true;
				showinfo = true;
			}
		}
		else if(key == config.imgkey[5])
		{
			if(config.rotate == conf_rotate_0)
				config.rotate = conf_rotate_270;
			else
				config.rotate --;
			needrc = needrp = true;
		}
		else if(key == config.imgkey[6])
		{
			if(config.rotate == conf_rotate_270)
				config.rotate = conf_rotate_0;
			else
				config.rotate ++;
			needrc = needrp = true;
		}
		else if(key == config.imgkey[3])
		{
			if(config.scale > 200)
				config.scale -= 50;
			else if(config.scale > 10)
				config.scale -= 10;
			else
				continue;
			config.fit = conf_fit_custom;
			needrc = needrp = true;
		}
		else if(key == config.imgkey[4])
		{
			if(config.scale < 200)
				config.scale += 10;
			else if(config.scale < 1000)
				config.scale += 50;
			else
				continue;
			config.fit = conf_fit_custom;
			needrc = needrp = true;
		}
		else
			needrf = needrc = false;
	}
	power_set_clock(222, 111);
	if(imgshow != NULL && imgshow != imgdata)
		free((void *)imgshow);
	if(imgdata != NULL)
		free((void *)imgdata);
	return selidx;
}

#ifdef FONT12
#define HRR 8
#define WRR 13
#else
#define HRR 6
#define WRR 10
#endif

static t_win_menu_op scene_filelist_menucb(dword key, p_win_menuitem item, dword count, dword max_height, dword topindex, dword * index)
{
	switch(key)
	{
		case (PSP_CTRL_SELECT | PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER):
			if(win_msgbox("是否退出软件?", "是", "否", 0xFFFFFFFF, 0xFFFFFFFF, RGB(0x18, 0x28, 0x50)))
				scene_exit();
			return win_menu_op_force_redraw;
		case PSP_CTRL_CROSS:
			* index = 0;
			return win_menu_op_force_redraw;
		case PSP_CTRL_SQUARE:
			* index = 0;
			if(item[*index].compname[0] == '.')
				return win_menu_op_ok;
			return win_menu_op_force_redraw;
		case PSP_CTRL_TRIANGLE:
			if(where != scene_in_dir)
				return win_menu_op_continue;
			disp_waitv();
			disp_duptocache();
			disp_rectangle(239 - DISP_FONTSIZE * 3, 135 - DISP_FONTSIZE, 240 + DISP_FONTSIZE * 3, 136 + DISP_FONTSIZE, 0xFFFFFFFF);
			disp_fillrect(240 - DISP_FONTSIZE * 3, 136 - DISP_FONTSIZE, 239 + DISP_FONTSIZE * 3, 135 + DISP_FONTSIZE, RGB(0x28, 0x50, 0x60));
			disp_putstring(240 - DISP_FONTSIZE * 3, 136 - DISP_FONTSIZE, 0xFFFFFFFF, (const byte *)"删除该文件？");
			disp_putstring(240 - DISP_FONTSIZE * 3, 136, 0xFFFFFFFF, (const byte *)"○是  ×否");
			disp_flip();
			if(ctrl_waitmask(PSP_CTRL_CIRCLE | PSP_CTRL_CROSS) == PSP_CTRL_CIRCLE)
			{
				char fn[256];
				strcpy(fn, config.shortpath);
				strcat(fn, item[*index].shortname);
				if((t_fs_filetype)item[*index].data == fs_filetype_dir)
					utils_del_dir(fn);
				else
					sceIoRemove(fn);
			}
			return win_menu_op_cancel;
		case PSP_CTRL_SELECT:
			return win_menu_op_cancel;
		case PSP_CTRL_START:
			scene_mp3bar();
			return win_menu_op_force_redraw;
	}
	dword orgidx = * index;
	t_win_menu_op op = win_menu_defcb(key, item, count, max_height, topindex, index);
	if(((t_fs_filetype)item[*index].data == fs_filetype_dir && (t_fs_filetype)item[orgidx].data != fs_filetype_dir) || (*index != orgidx && *index >= topindex && *index < topindex + HRR * 2 && ((orgidx - topindex < HRR && *index - topindex >= HRR) || (orgidx - topindex >= HRR && *index - topindex < HRR))))
		return win_menu_op_force_redraw;
	return op;
}

static void scene_filelist_predraw(p_win_menuitem item, dword index, dword topindex, dword max_height)
{
	disp_fillrect(0, 0, 479, DISP_FONTSIZE - 1, 0);
	disp_putstring(0, 0, 0xFFFFFFFF, (const byte *)EREADER_VERSION_STR_LONG);
	disp_line(0, DISP_FONTSIZE, 479, DISP_FONTSIZE, 0xFFFFFFFF);
	disp_rectangle(239 - WRR * DISP_FONTSIZE, 138 - (HRR + 1) * (DISP_FONTSIZE + 1), 243 + WRR * DISP_FONTSIZE, 141 + HRR * (DISP_FONTSIZE + 1), 0xFFFFFFFF);
	disp_fillrect(240 - WRR * DISP_FONTSIZE, 139 - (HRR + 1) * (DISP_FONTSIZE + 1), 242 + WRR * DISP_FONTSIZE, 137 - HRR * (DISP_FONTSIZE + 1), RGB(0x30, 0x60, 0x30));
	disp_putnstring(240 - WRR * DISP_FONTSIZE, 139 - (HRR + 1) * (DISP_FONTSIZE + 1), 0xFFFFFFFF, (const byte *)config.path, 40);
	disp_line(240 - WRR * DISP_FONTSIZE, 138 - HRR * (DISP_FONTSIZE + 1), 242 + WRR * DISP_FONTSIZE, 138 - HRR * (DISP_FONTSIZE + 1), 0xFFFFFFFF);
}

static void scene_filelist_postdraw(p_win_menuitem item, dword index, dword topindex, dword max_height)
{
	if((t_fs_filetype)item[index].data != fs_filetype_dir)
	{
		if(where == scene_in_dir)
		{
			if(index - topindex < HRR)
			{
				disp_rectangle(239 - (WRR - 2) * DISP_FONTSIZE, 135 + (HRR - 3) * (DISP_FONTSIZE + 1), 243 + (WRR - 2) * DISP_FONTSIZE, 136 + HRR * (DISP_FONTSIZE + 1), 0xFFFFFFFF);
				disp_fillrect(240 - (WRR - 2) * DISP_FONTSIZE, 136 + (HRR - 3) * (DISP_FONTSIZE + 1), 242 + (WRR - 2) * DISP_FONTSIZE, 135 + HRR * (DISP_FONTSIZE + 1), RGB(0x20, 0x20, 0x20));
				char outstr[256];
				sprintf(outstr, "文件大小: %u 字节\n", (unsigned int)item[index].data3);
				disp_putstring(242 - (WRR - 2) * DISP_FONTSIZE, 136 + (HRR - 3) * (DISP_FONTSIZE + 1), 0xFFFFFFFF, (const byte *)outstr);
				sprintf(outstr, "创建时间: %04d-%02d-%02d %02d:%02d:%02d\n", (item[index].data2[0] >> 9) + 1980, (item[index].data2[0] & 0x01FF) >> 5, item[index].data2[0] & 0x01F, item[index].data2[1] >> 11, (item[index].data2[1] & 0x07FF) >> 5, (item[index].data2[1] & 0x01F) * 2);
				disp_putstring(242 - (WRR - 2) * DISP_FONTSIZE, 136 + (HRR - 2) * (DISP_FONTSIZE + 1), 0xFFFFFFFF, (const byte *)outstr);
				sprintf(outstr, "最后修改: %04d-%02d-%02d %02d:%02d:%02d\n", (item[index].data2[2] >> 9) + 1980, (item[index].data2[2] & 0x01FF) >> 5, item[index].data2[2] & 0x01F, item[index].data2[3] >> 11, (item[index].data2[3] & 0x07FF) >> 5, (item[index].data2[3] & 0x01F) * 2);
				disp_putstring(242 - (WRR - 2) * DISP_FONTSIZE, 136 + (HRR - 1) * (DISP_FONTSIZE + 1), 0xFFFFFFFF, (const byte *)outstr);
			}
			else
			{
				disp_rectangle(239 - (WRR - 2) * DISP_FONTSIZE, 141 - HRR * (DISP_FONTSIZE + 1), 243 + (WRR - 2) * DISP_FONTSIZE, 142 - (HRR - 3) * (DISP_FONTSIZE + 1), 0xFFFFFFFF);
				disp_fillrect(240 - (WRR - 2) * DISP_FONTSIZE, 142 - HRR * (DISP_FONTSIZE + 1), 242 + (WRR - 2) * DISP_FONTSIZE, 141 - (HRR - 3) * (DISP_FONTSIZE + 1), RGB(0x20, 0x20, 0x20));
				char outstr[256];
				sprintf(outstr, "文件大小: %u 字节\n", (unsigned int)item[index].data3);
				disp_putstring(242 - (WRR - 2) * DISP_FONTSIZE, 142 - HRR * (DISP_FONTSIZE + 1), 0xFFFFFFFF, (const byte *)outstr);
				sprintf(outstr, "创建时间: %04d-%02d-%02d %02d:%02d:%02d\n", (item[index].data2[0] >> 9) + 1980, (item[index].data2[0] & 0x01FF) >> 5, item[index].data2[0] & 0x01F, item[index].data2[1] >> 11, (item[index].data2[1] & 0x07FF) >> 5, (item[index].data2[1] & 0x01F) * 2);
				disp_putstring(242 - (WRR - 2) * DISP_FONTSIZE, 142 - (HRR - 1) * (DISP_FONTSIZE + 1), 0xFFFFFFFF, (const byte *)outstr);
				sprintf(outstr, "最后修改: %04d-%02d-%02d %02d:%02d:%02d\n", (item[index].data2[2] >> 9) + 1980, (item[index].data2[2] & 0x01FF) >> 5, item[index].data2[2] & 0x01F, item[index].data2[3] >> 11, (item[index].data2[3] & 0x07FF) >> 5, (item[index].data2[3] & 0x01F) * 2);
				disp_putstring(242 - (WRR - 2) * DISP_FONTSIZE, 142 - (HRR - 2) * (DISP_FONTSIZE + 1), 0xFFFFFFFF, (const byte *)outstr);
			}
		}
		else
		{
			char outstr[256];
			sprintf(outstr, "文件大小: %s 字节\n", item[index].shortname);
			if(index - topindex < HRR)
			{
				disp_rectangle(239 - (WRR - 2) * DISP_FONTSIZE, 135 + (HRR - 1) * (DISP_FONTSIZE + 1), 243 + (WRR - 2) * DISP_FONTSIZE, 136 + HRR * (DISP_FONTSIZE + 1), 0xFFFFFFFF);
				disp_fillrect(240 - (WRR - 2) * DISP_FONTSIZE, 136 + (HRR - 1) * (DISP_FONTSIZE + 1), 242 + (WRR - 2) * DISP_FONTSIZE, 135 + HRR * (DISP_FONTSIZE + 1), RGB(0x20, 0x20, 0x20));
				disp_putstring(242 - (WRR - 2) * DISP_FONTSIZE, 136 + (HRR - 1) * (DISP_FONTSIZE + 1), 0xFFFFFFFF, (const byte *)outstr);
			}
			else
			{
				disp_rectangle(239 - (WRR - 2) * DISP_FONTSIZE, 141 - HRR * (DISP_FONTSIZE + 1), 243 + (WRR - 2) * DISP_FONTSIZE, 142 - (HRR - 1) * (DISP_FONTSIZE + 1), 0xFFFFFFFF);
				disp_fillrect(240 - (WRR - 2) * DISP_FONTSIZE, 142 - HRR * (DISP_FONTSIZE + 1), 242 + (WRR - 2) * DISP_FONTSIZE, 141 - (HRR - 1) * (DISP_FONTSIZE + 1), RGB(0x20, 0x20, 0x20));
				disp_putstring(242 - (WRR - 2) * DISP_FONTSIZE, 142 - HRR * (DISP_FONTSIZE + 1), 0xFFFFFFFF, (const byte *)outstr);
			}
		}
	}
}

static void scene_filelist()
{
	dword idx = 0;
	where = scene_in_dir;
	dword plen = strlen(config.path);
	if(plen > 0 && config.path[plen - 1] == '/')
	{
		filecount = fs_dir_to_menu(config.path, config.shortpath, &filelist, RGB(0xDF, 0xDF, 0xDF), RGB(0xFF, 0xFF, 0x40), RGB(0x10, 0x30, 0x20), RGB(0x20, 0x20, 0xDF));
		if(filecount == 0)
		{
			strcpy(config.path, "ms0:/");
			strcpy(config.shortpath, "ms0:/");
			filecount = fs_dir_to_menu(config.path, config.shortpath, &filelist, RGB(0xDF, 0xDF, 0xDF), RGB(0xFF, 0xFF, 0x40), RGB(0x10, 0x30, 0x20), RGB(0x20, 0x20, 0xDF));
		}
	}
	else
		switch(fs_file_get_type(config.shortpath))
		{
		case fs_filetype_zip:
		{
			where = scene_in_zip;
			filecount = fs_zip_to_menu(config.shortpath, &filelist, RGB(0xDF, 0xDF, 0xDF), RGB(0xFF, 0xFF, 0x40), RGB(0x10, 0x30, 0x20), RGB(0x20, 0x20, 0xDF));
			break;
		}
		case fs_filetype_chm:
		{
			where = scene_in_chm;
			filecount = fs_chm_to_menu(config.shortpath, &filelist, RGB(0xDF, 0xDF, 0xDF), RGB(0xFF, 0xFF, 0x40), RGB(0x10, 0x30, 0x20), RGB(0x20, 0x20, 0xDF));
			break;
		}
		default:
			strcpy(config.path, "ms0:/");
			filecount = fs_dir_to_menu(config.path, config.shortpath, &filelist, RGB(0xDF, 0xDF, 0xDF), RGB(0xFF, 0xFF, 0x40), RGB(0x10, 0x30, 0x20), RGB(0x20, 0x20, 0xDF));
			break;
		}
	usb_activate();
	while (1)
	{
		idx = win_menu(240 - WRR * DISP_FONTSIZE, 139 - HRR * (DISP_FONTSIZE + 1), WRR * 4, HRR * 2, filelist, filecount, idx, 0, RGB(0x10, 0x30, 0x20), scene_filelist_predraw, scene_filelist_postdraw, scene_filelist_menucb);
		if(idx == INVALID)
		{
			switch(where)
			{
			case scene_in_zip:
				filecount = fs_zip_to_menu(config.shortpath, &filelist, RGB(0xDF, 0xDF, 0xDF), RGB(0xFF, 0xFF, 0x40), RGB(0x10, 0x30, 0x20), RGB(0x20, 0x20, 0xDF));
				break;
			case scene_in_chm:
				filecount = fs_chm_to_menu(config.shortpath, &filelist, RGB(0xDF, 0xDF, 0xDF), RGB(0xFF, 0xFF, 0x40), RGB(0x10, 0x30, 0x20), RGB(0x20, 0x20, 0xDF));
				break;
			default:
				filecount = fs_dir_to_menu(config.path, config.shortpath, &filelist, RGB(0xDF, 0xDF, 0xDF), RGB(0xFF, 0xFF, 0x40), RGB(0x10, 0x30, 0x20), RGB(0x20, 0x20, 0xDF));
				if(filecount == 0)
				{
					strcpy(config.path, "ms0:/");
					strcpy(config.shortpath, "ms0:/");
					filecount = fs_dir_to_menu(config.path, config.shortpath, &filelist, RGB(0xDF, 0xDF, 0xDF), RGB(0xFF, 0xFF, 0x40), RGB(0x10, 0x30, 0x20), RGB(0x20, 0x20, 0xDF));
				}
				break;
			}
			idx = 0;
			continue;
		}
		switch((t_fs_filetype)filelist[idx].data)
		{
		case fs_filetype_dir:
		{
			char pdir[256];
			bool isup = false;
			pdir[0] = 0;
			if(strcmp(filelist[idx].compname, "..") == 0)
			{
				if(where == scene_in_dir)
				{
					int sl, ll;
					if((ll = strlen(config.path) - 1) >= 0)
						while(config.path[ll] == '/' && ll >= 0)
						{
							config.path[ll] = 0;
							ll --;
						}
					if((sl = strlen(config.shortpath) - 1) >= 0)
						while(config.shortpath[sl] == '/' && sl >= 0)
						{
							config.shortpath[sl] = 0;
							sl --;
						}
				}
				char * lps;
				isup = true;
				if((lps = strrchr(config.path, '/')) != NULL)
				{
					lps ++;
					strcpy(pdir, lps);
					*lps = 0;
				}
				if((lps = strrchr(config.shortpath, '/')) != NULL)
					*(lps + 1) = 0;
			}
			else if(where == scene_in_dir)
			{
				strcat(config.path, filelist[idx].compname);
				strcat(config.path, "/");
				strcat(config.shortpath, filelist[idx].shortname);
				strcat(config.shortpath, "/");
			}
			filecount = fs_dir_to_menu(config.path, config.shortpath, &filelist, RGB(0xDF, 0xDF, 0xDF), RGB(0xFF, 0xFF, 0x40), RGB(0x10, 0x30, 0x20), RGB(0x20, 0x20, 0xDF));
			if(isup)
			{
				for(idx = 0; idx < filecount; idx ++)
					if(stricmp(filelist[idx].compname, pdir) == 0)
						break;
				if(idx == filecount)
					idx = 0;
			}
			else
				idx = 0;
			where = scene_in_dir;
			break;
		}
		case fs_filetype_zip:
			where = scene_in_zip;
			strcat(config.path, filelist[idx].compname);
			strcat(config.shortpath, filelist[idx].shortname);
			idx = 0;
			filecount = fs_zip_to_menu(config.shortpath, &filelist, RGB(0xDF, 0xDF, 0xDF), RGB(0xFF, 0xFF, 0x40), RGB(0x10, 0x30, 0x20), RGB(0x20, 0x20, 0xDF));
			break;
		case fs_filetype_chm:
			where = scene_in_chm;
			strcat(config.path, filelist[idx].compname);
			strcat(config.shortpath, filelist[idx].shortname);
			idx = 0;
			filecount = fs_chm_to_menu(config.shortpath, &filelist, RGB(0xDF, 0xDF, 0xDF), RGB(0xFF, 0xFF, 0x40), RGB(0x10, 0x30, 0x20), RGB(0x20, 0x20, 0xDF));
			break;
		case fs_filetype_png:
			usb_deactivate();
			idx = scene_readimage(idx);
			usb_activate();
			break;
		case fs_filetype_gif:
			usb_deactivate();
			idx = scene_readimage(idx);
			usb_activate();
			break;
		case fs_filetype_jpg:
			usb_deactivate();
			idx = scene_readimage(idx);
			usb_activate();
			break;
		case fs_filetype_bmp:
			usb_deactivate();
			idx = scene_readimage(idx);
			usb_activate();
			break;
		case fs_filetype_tga:
			usb_deactivate();
			idx = scene_readimage(idx);
			usb_activate();
			break;
		default:
			usb_deactivate();
			idx = scene_readbook(idx);
			usb_activate();
			break;
		}
	}
	if(filelist != NULL)
	{
		free((void *)filelist);
		filelist = NULL;
		filecount = 0;
	}
	usb_deactivate();
}

extern void scene_init()
{
	char efontfile[256], cfontfile[256], conffile[256], bgfile[256];

	getcwd(appdir, 256);
	strcat(appdir, "/");
	strcpy(config.path, "ms0:/");
	strcpy(config.shortpath, "ms0:/");

#ifdef _DEBUG
	char logfile[256];
	strcpy(logfile, appdir);
	strcat(logfile, "ereader.log");
	log_open(logfile);
#endif

	ctrl_init();
	disp_init();
	usb_open();
	fat_init();

	strcpy(bgfile, appdir);
	strcat(bgfile, "bg.png");
	bg_load(bgfile);

	strcpy(conffile, appdir);
	strcat(conffile, "ereader.conf");
	conf_set_file(conffile);
	conf_load(&config);
	dword clus1 = fat_dir_clus(config.path);
	dword clus2 = fat_dir_clus(config.shortpath);
	if(clus1 == 0 || clus2 == 0 || clus1 != clus2)
	{
		strcpy(config.path, "ms0:/");
		strcpy(config.shortpath, "ms0:/");
	}

	if(!config.vertread)
	{
		if(config.infobar)
			rowsperpage = (272 - DISP_FONTSIZE) / (config.rowspace + DISP_FONTSIZE);
		else
			rowsperpage = 272 / (config.rowspace + DISP_FONTSIZE);
	}
	else
	{
		if(config.infobar)
			rowsperpage = (480 - DISP_FONTSIZE) / (config.rowspace + DISP_FONTSIZE);
		else
			rowsperpage = 480 / (config.rowspace + DISP_FONTSIZE);
	}

	strcpy(efontfile, appdir);
	strcpy(cfontfile, appdir);
#ifdef FONT12
	strcat(efontfile, "fonts/ASC12");
	strcat(cfontfile, "fonts/GBK12");
#else
	strcat(efontfile, "fonts/ASC16");
	strcat(cfontfile, "fonts/GBK16");
#endif
	disp_load_font(efontfile, cfontfile);
	mp3_init();
	mp3_list("ms0:/PSP/MUSIC/");
	mp3_start();
	bookmark_setbasedir(appdir);

	scene_filelist();
}

extern void scene_exit()
{
	if(fs != NULL && config.autobm)
	{
		char archname[256];
		if(where == scene_in_zip || where == scene_in_chm)
		{
			strcpy(archname, config.shortpath);
			strcat(archname, fs->filename);
		}
		else
			strcpy(archname, fs->filename);
		bookmark_autosave(archname, fs->crow);
	}
	conf_save(&config);
	mp3_end();
	fat_free();
	disp_free_font();
	usb_close();
#ifdef _DEBUG
	log_close();
#endif
	sceKernelExitGame();
}
