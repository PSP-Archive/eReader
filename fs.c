#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pspuser.h>

#include "lib/unzip.h"
#include "lib/chm_lib.h"
#include "common/utils.h"
#include "html.h"
#include "display.h"
#include "win.h"
#include "charsets.h"
#include "fat.h"
#include "fs.h"

typedef struct {
	const char * ext;
	t_fs_filetype ft;
} t_fs_filetype_entry;

t_fs_filetype_entry ft_table[] =
{
	{"txt", fs_filetype_txt},
	{"log", fs_filetype_txt},
	{"ini", fs_filetype_txt},
	{"inf", fs_filetype_txt},
	{"xml", fs_filetype_txt},
	{"cpp", fs_filetype_txt},
	{"c", fs_filetype_txt},
	{"h", fs_filetype_txt},
	{"hpp", fs_filetype_txt},
	{"cc", fs_filetype_txt},
	{"cxx", fs_filetype_txt},
	{"pas", fs_filetype_txt},
	{"bas", fs_filetype_txt},
	{"rc", fs_filetype_txt},
	{"pl", fs_filetype_txt},
	{"bat", fs_filetype_txt},
	{"js", fs_filetype_txt},
	{"vbs", fs_filetype_txt},
	{"vb", fs_filetype_txt},
	{"cs", fs_filetype_txt},
	{"css", fs_filetype_txt},
	{"csv", fs_filetype_txt},
	{"php", fs_filetype_txt},
	{"php3", fs_filetype_txt},
	{"asp", fs_filetype_txt},
	{"aspx", fs_filetype_txt},
	{"html", fs_filetype_html},
	{"htm", fs_filetype_html},
	{"shtml", fs_filetype_html},
	{"png", fs_filetype_png},
	{"gif", fs_filetype_gif},
	{"jpg", fs_filetype_jpg},
	{"jpeg", fs_filetype_jpg},
	{"bmp", fs_filetype_bmp},
	{"tga", fs_filetype_tga},
	{"zip", fs_filetype_zip},
	{"chm", fs_filetype_chm},
	{NULL, fs_filetype_unknown}
};

/* -- old style
extern dword fs_dir_to_menu(const char * dir, p_win_menuitem * mitem, dword icolor, dword selicolor, dword selrcolor, dword selbcolor)
{
	SceIoDirent info;
	dword cur_count = 0;
	p_win_menuitem item = NULL;
	int fd = sceIoDopen(dir);
	if(fd < 0)
		return 0;
	memset(&info, 0, sizeof(SceIoDirent));
	while(sceIoDread(fd, &info) > 0)
	{
		if(info.d_stat.st_attr & FIO_SO_IFDIR)
		{
			if(info.d_name[0] == '.' && info.d_name[1] == 0)
				continue;
			if(cur_count % 256 == 0)
			{
				if(cur_count == 0)
					* mitem = (p_win_menuitem)malloc(sizeof(t_win_menuitem) * 256);
				else
					* mitem = (p_win_menuitem)realloc(*mitem, sizeof(t_win_menuitem) * (cur_count + 256));
				if(* mitem == NULL)
					return 0;
				item = *mitem;
			}
			item[cur_count].data = (void *)fs_filetype_dir;
			strcpy(item[cur_count].compname, info.d_name);
			item[cur_count].name[0] = '<';
			if((item[cur_count].width = strlen(info.d_name) + 2) > 40)
			{
				strncpy(&item[cur_count].name[1], info.d_name, 35);
				item[cur_count].name[36] = item[cur_count].name[37] = item[cur_count].name[38] = '.';
				item[cur_count].name[39] = '>';
				item[cur_count].name[40] = 0;
				item[cur_count].width = 40;
			}
			else
			{
				strcpy(&item[cur_count].name[1], info.d_name);
				item[cur_count].name[item[cur_count].width - 1] = '>';
				item[cur_count].name[item[cur_count].width] = 0;
			}
		}
		else
		{
			t_fs_filetype ft;
			if((ft = fs_file_get_type(info.d_name)) == fs_filetype_unknown)
				continue;
			if(cur_count % 256 == 0)
			{
				if(cur_count == 0)
					* mitem = (p_win_menuitem)malloc(sizeof(t_win_menuitem) * 256);
				else
					* mitem = (p_win_menuitem)realloc(*mitem, sizeof(t_win_menuitem) * (cur_count + 256));
				if(* mitem == NULL)
					return 0;
				item = *mitem;
			}
			item[cur_count].data = (void *)ft;
			strcpy(item[cur_count].compname, info.d_name);
			if((item[cur_count].width = strlen(info.d_name)) > 40)
			{
				strncpy(item[cur_count].name, info.d_name, 37);
				item[cur_count].name[37] = item[cur_count].name[38] = item[cur_count].name[39] = '.';
				item[cur_count].name[40] = 0;
				item[cur_count].width = 40;
			}
			else
				strcpy(item[cur_count].name, info.d_name);
		}
		charsets_sjis_conv((const byte *)item[cur_count].name, (byte *)item[cur_count].name);
		item[cur_count].icolor = icolor;
		item[cur_count].selicolor = selicolor;
		item[cur_count].selrcolor = selrcolor;
		item[cur_count].selbcolor = selbcolor;
		cur_count ++;
	}
	sceIoDclose(fd);
	return cur_count;
}
*/

// New style fat system custom reading
extern dword fs_dir_to_menu(const char * dir, const char * sdir, p_win_menuitem * mitem, dword icolor, dword selicolor, dword selrcolor, dword selbcolor)
{
	p_win_menuitem item = NULL;
	p_fat_info info;
	dword count = fat_readdir(dir, sdir, &info);
	if(count == INVALID)
		return 0;
	if(*mitem != NULL)
	{
		free((void *)* mitem);
		* mitem = NULL;
	}
	dword i, cur_count = 0;
	for(i = 0; i < count; i ++)
	{
		if(info[i].attr & FAT_FILEATTR_DIRECTORY)
		{
			if(cur_count % 256 == 0)
			{
				if(cur_count == 0)
					* mitem = (p_win_menuitem)malloc(sizeof(t_win_menuitem) * 256);
				else
					* mitem = (p_win_menuitem)realloc(*mitem, sizeof(t_win_menuitem) * (cur_count + 256));
				if(* mitem == NULL)
				{
					free((void *)info);
					return 0;
				}
				item = *mitem;
			}
			item[cur_count].data = (void *)fs_filetype_dir;
			strcpy(item[cur_count].shortname, info[i].filename);
			strcpy(item[cur_count].compname, info[i].longname);
			item[cur_count].name[0] = '<';
			if((item[cur_count].width = strlen(info[i].longname) + 2) > 40)
			{
				strncpy(&item[cur_count].name[1], info[i].longname, 35);
				item[cur_count].name[36] = item[cur_count].name[37] = item[cur_count].name[38] = '.';
				item[cur_count].name[39] = '>';
				item[cur_count].name[40] = 0;
				item[cur_count].width = 40;
			}
			else
			{
				strcpy(&item[cur_count].name[1], info[i].longname);
				item[cur_count].name[item[cur_count].width - 1] = '>';
				item[cur_count].name[item[cur_count].width] = 0;
			}
		}
		else
		{
			t_fs_filetype ft = fs_file_get_type(info[i].longname);
			/*
			if(ft == fs_filetype_unknown)
				continue;
			*/
			if(cur_count % 256 == 0)
			{
				if(cur_count == 0)
					* mitem = (p_win_menuitem)malloc(sizeof(t_win_menuitem) * 256);
				else
					* mitem = (p_win_menuitem)realloc(*mitem, sizeof(t_win_menuitem) * (cur_count + 256));
				if(* mitem == NULL)
				{
					free((void *)info);
					return 0;
				}
				item = *mitem;
			}
			item[cur_count].data = (void *)ft;
			strcpy(item[cur_count].shortname, info[i].filename);
			strcpy(item[cur_count].compname, info[i].longname);
			if((item[cur_count].width = strlen(info[i].longname)) > 40)
			{
				strncpy(item[cur_count].name, info[i].longname, 37);
				item[cur_count].name[37] = item[cur_count].name[38] = item[cur_count].name[39] = '.';
				item[cur_count].name[40] = 0;
				item[cur_count].width = 40;
			}
			else
				strcpy(item[cur_count].name, info[i].longname);
		}
		item[cur_count].icolor = icolor;
		item[cur_count].selicolor = selicolor;
		item[cur_count].selrcolor = selrcolor;
		item[cur_count].selbcolor = selbcolor;
		item[cur_count].data2[0] = info[i].cdate;
		item[cur_count].data2[1] = info[i].ctime;
		item[cur_count].data2[2] = info[i].mdate;
		item[cur_count].data2[3] = info[i].mtime;
		item[cur_count].data3 = info[i].filesize;
		cur_count ++;
	}
	free((void *)info);
	return cur_count;
}

extern dword fs_zip_to_menu(const char * zipfile, p_win_menuitem * mitem, dword icolor, dword selicolor, dword selrcolor, dword selbcolor)
{
	unzFile unzf = unzOpen(zipfile);
	p_win_menuitem item = NULL;
	if(unzf == NULL)
		return 0;
	dword cur_count = 1;
	* mitem = (p_win_menuitem)malloc(sizeof(t_win_menuitem) * 256);
	if(* mitem == NULL)
	{
		unzClose(unzf);
		return 0;
	}
	item = * mitem;
	strcpy(item[0].name, "<..>");
	strcpy(item[0].compname, "..");
	item[0].data = (void *)fs_filetype_dir;
	item[0].width = 4;
	item[0].icolor = icolor;
	item[0].selicolor = selicolor;
	item[0].selrcolor = selrcolor;
	item[0].selbcolor = selbcolor;
	if(unzGoToFirstFile(unzf) != UNZ_OK)
	{
		unzClose(unzf);
		return 1;
	}
	do {
		char fname[256];
		unz_file_info file_info;
		if(unzGetCurrentFileInfo(unzf, &file_info, fname, 256, NULL, 0, NULL, 0) != UNZ_OK)
			break;
		t_fs_filetype ft = fs_file_get_type(fname);
		if(ft != fs_filetype_txt && ft != fs_filetype_html)
			continue;
		if(cur_count % 256 == 0)
		{
			* mitem = (p_win_menuitem)realloc(*mitem, sizeof(t_win_menuitem) * (cur_count + 256));
			if(* mitem == NULL)
				return 0;
			item = *mitem;
		}
		item[cur_count].data = (void *)ft;
		strcpy(item[cur_count].compname, fname);
		sprintf(item[cur_count].shortname, "%u", (unsigned int)file_info.uncompressed_size);
		if((item[cur_count].width = strlen(fname)) > 40)
		{
			strncpy(item[cur_count].name, fname, 37);
			item[cur_count].name[37] = item[cur_count].name[38] = item[cur_count].name[39] = '.';
			item[cur_count].name[40] = 0;
			item[cur_count].width = 40;
		}
		else
			strcpy(item[cur_count].name, fname);
		item[cur_count].icolor = icolor;
		item[cur_count].selicolor = selicolor;
		item[cur_count].selrcolor = selrcolor;
		item[cur_count].selbcolor = selbcolor;
		cur_count ++;
	} while(unzGoToNextFile(unzf) == UNZ_OK);
	unzClose(unzf);
	return cur_count;
}

typedef struct {
	p_win_menuitem * mitem;
	dword cur_count;
	dword icolor;
	dword selicolor;
	dword selrcolor;
	dword selbcolor;
} t_fs_chm_enum, *p_fs_chm_enum;

static int chmEnum(struct chmFile *h, struct chmUnitInfo *ui, void *context)
{
	t_fs_filetype ft = fs_file_get_type(ui->path);
	if(ft == fs_filetype_txt || ft == fs_filetype_html)
	{
		p_win_menuitem * mitem = ((p_fs_chm_enum)context)->mitem;
		p_win_menuitem item = *mitem;
		dword cur_count = ((p_fs_chm_enum)context)->cur_count;
		if(cur_count % 256 == 0)
		{
			* mitem = (p_win_menuitem)realloc(*mitem, sizeof(t_win_menuitem) * (cur_count + 256));
			if(* mitem == NULL)
			{
				((p_fs_chm_enum)context)->cur_count = 0;
				return CHM_ENUMERATOR_FAILURE;
			}
			item = *mitem;
		}
		int size = strlen(ui->path);
		char fname[256] = "";
		if(size == 0 || ui->path[size - 1] == '/')
			return CHM_ENUMERATOR_CONTINUE;
		else
		{
			strcpy(item[cur_count].compname, ui->path);
			sprintf(item[cur_count].shortname, "%u", (unsigned int)ui->length);
			if(ui->path[0] == '/')
				strcpy(fname, ui->path + 1);
			else
				strcpy(fname, ui->path);
		}
		item[cur_count].data = (void *)ft;
		if((item[cur_count].width = strlen(fname)) > 40)
		{
			strncpy(item[cur_count].name, fname, 37);
			item[cur_count].name[37] = item[cur_count].name[38] = item[cur_count].name[39] = '.';
			item[cur_count].name[40] = 0;
			item[cur_count].width = 40;
		}
		else
			strcpy(item[cur_count].name, fname);
		item[cur_count].icolor = ((p_fs_chm_enum)context)->icolor;
		item[cur_count].selicolor = ((p_fs_chm_enum)context)->selicolor;
		item[cur_count].selrcolor = ((p_fs_chm_enum)context)->selrcolor;
		item[cur_count].selbcolor = ((p_fs_chm_enum)context)->selbcolor;
		((p_fs_chm_enum)context)->cur_count ++;
	}
	return CHM_ENUMERATOR_CONTINUE;
}

extern dword fs_chm_to_menu(const char * chmfile, p_win_menuitem * mitem, dword icolor, dword selicolor, dword selrcolor, dword selbcolor)
{
	struct chmFile * chm = chm_open(chmfile);
	p_win_menuitem item = NULL;
	if(chm == NULL)
		return 0;
	* mitem = (p_win_menuitem)malloc(sizeof(t_win_menuitem) * 256);
	if(* mitem == NULL)
	{
		chm_close(chm);
		return 0;
	}
	item = * mitem;
	strcpy(item[0].name, "<..>");
	strcpy(item[0].compname, "..");
	item[0].data = (void *)fs_filetype_dir;
	item[0].width = 4;
	item[0].icolor = icolor;
	item[0].selicolor = selicolor;
	item[0].selrcolor = selrcolor;
	item[0].selbcolor = selbcolor;

	t_fs_chm_enum cenum;
	cenum.mitem = mitem;
	cenum.cur_count = 1;
	cenum.icolor = icolor;
	cenum.selicolor = selicolor;
	cenum.selrcolor = selrcolor;
	cenum.selbcolor = selbcolor;
	chm_enumerate(chm, CHM_ENUMERATE_NORMAL | CHM_ENUMERATE_FILES, chmEnum, (void *)&cenum);

	chm_close(chm);
	return cenum.cur_count;
}

extern p_fs_text fs_open_binary(const char * filename, bool vert)
{
	int fd;
	p_fs_text txt = (p_fs_text)calloc(1, sizeof(t_fs_text));
	if(txt == NULL)
		return NULL;
	if((fd = sceIoOpen(filename, PSP_O_RDONLY, 0777)) < 0)
	{
		fs_close_text(txt);
		return NULL;
	}
	strcpy(txt->filename, filename);
	txt->size = sceIoLseek32(fd, 0, PSP_SEEK_END);
	if(txt->size > 256 * 1024) txt->size = 256 * 1024;
	byte * tmpbuf;
	dword bpr = (vert ? 33 : 56);
	if((txt->buf = (char *)calloc(1, (txt->size + 15) / 16 * bpr)) == NULL || (tmpbuf = (byte *)calloc(1, txt->size)) == NULL)
	{
		sceIoClose(fd);
		fs_close_text(txt);
		return NULL;
	}
	sceIoLseek32(fd, 0, PSP_SEEK_SET);
	sceIoRead(fd, tmpbuf, txt->size);
	sceIoClose(fd);

	dword curs = 0;
	txt->row_count = (txt->size + 15) / 16;
	byte * cbuf = tmpbuf;
	dword i;
	for(i = 0; i < txt->row_count; i ++)
	{
		if((i % 1024) == 0)
		{
			curs = i >> 10;
			if((txt->rows[curs] = (p_fs_textrow)calloc(1024, sizeof(t_fs_textrow))) == NULL)
			{
				free((void *)tmpbuf);
				fs_close_text(txt);
				return NULL;
			}
		}
		txt->rows[curs][i & 0x3FF].start = &txt->buf[bpr * i];
		txt->rows[curs][i & 0x3FF].count = bpr;
		if(vert)
		{
			sprintf(&txt->buf[bpr * i], "%02X%02X%02X%02X%02X%02X%02X%02X %02X%02X%02X%02X%02X%02X%02X%02X", cbuf[0], cbuf[1], cbuf[2], cbuf[3], cbuf[4], cbuf[5], cbuf[6], cbuf[7], cbuf[8], cbuf[9], cbuf[10], cbuf[11], cbuf[12], cbuf[13], cbuf[14], cbuf[15]);
			if((i + 1) * 16 > txt->size)
			{
				dword padding = (i + 1) * 16 - txt->size;
				if(padding < 9)
					memset(&txt->buf[bpr * i + bpr - padding * 2], 0x20, padding * 2);
				else
					memset(&txt->buf[bpr * i + bpr - 1 - padding * 2], 0x20, padding * 2 + 1);
			}
		}
		else
		{
			sprintf(&txt->buf[bpr * i], "%02X%02X %02X%02X %02X%02X %02X%02X %02X%02X %02X%02X %02X%02X %02X%02X ", cbuf[0], cbuf[1], cbuf[2], cbuf[3], cbuf[4], cbuf[5], cbuf[6], cbuf[7], cbuf[8], cbuf[9], cbuf[10], cbuf[11], cbuf[12], cbuf[13], cbuf[14], cbuf[15]);
			dword j;
			for(j = 0; j < 16; j ++)
				txt->buf[bpr * i + 40 + j] = (cbuf[j] > 0x1F && cbuf[j] < 0x7F) ? cbuf[j] : '.';
			if((i + 1) * 16 > txt->size)
			{
				dword padding = (i + 1) * 16 - txt->size;
				memset(&txt->buf[bpr * i + bpr - padding], 0x20, padding);
				if((padding & 1) > 0)
					memset(&txt->buf[bpr * i + 40 - padding / 2 * 5 - 3], 0x20, padding / 2 * 5 + 3);
				else
					memset(&txt->buf[bpr * i + 40 - padding / 2 * 5], 0x20, padding / 2 * 5);
			}
		}
		cbuf += 16;
	}
	free((void *)tmpbuf);
	return txt;
}

static void fs_decode_text(p_fs_text txt, t_conf_encode encode)
{
	if(txt->size > 1 && *(word*)txt->buf == 0xFEFF)
	{
		charsets_ucs_conv((const byte *)txt->buf + 2, (byte *)txt->buf + 2);
		txt->isucs = true;
	}
	else
	{
		switch(encode)
		{
		case conf_encode_big5:
			charsets_big5_conv((const byte *)txt->buf, (byte *)txt->buf);
			break;
		case conf_encode_sjis:
			{
				char * orgbuf = txt->buf;
				charsets_sjis_conv((const byte *)orgbuf, (byte **)&txt->buf, &txt->size);
				if(txt->buf != NULL)
					free((void *)orgbuf);
				else
					txt->buf = orgbuf;
			}
			break;
		default:;
		}
		txt->isucs = false;
	}
}

extern bool fs_format_text(p_fs_text txt, dword rowbytes)
{
	static bool bytetable[256] = {
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0,  // 0x00
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x10
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x20
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x30
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x40
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x50
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x60
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x70
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x80
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0x90
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xA0
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xB0
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xC0
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xD0
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0xE0
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0   // 0xF0
	};
	char * pos = txt->buf, * posend = pos + txt->size;
	dword curs;
	txt->row_count = 0;
	for(curs = 0; curs < 1024; ++ curs)
		if(txt->rows[curs] != NULL)
		{
			free((void *)txt->rows[curs]);
			txt->rows[curs] = NULL;
		}
	curs = 0;
	while(txt->row_count < 1024 * 1024 && pos + 1 < posend)
	{
		if((txt->row_count % 1024) == 0)
		{
			curs = txt->row_count >> 10;
			if((txt->rows[curs] = (p_fs_textrow)calloc(1024, sizeof(t_fs_textrow))) == NULL)
				return false;
		}
		txt->rows[curs][txt->row_count & 0x3FF].start = pos;
		char * startp = pos, * endp = pos + rowbytes;
		if(endp > posend)
			endp = posend;
		while(pos < endp && bytetable[*(byte *)pos] == 0)
			if((*(byte *)pos) >= 0x80)
				pos += 2;
			else
				++ pos;
		if(pos > endp)
			pos -= 2;
		if(pos + 1 < posend && ((*pos >= 'A' && *pos <= 'Z') || (*pos >= 'a' && *pos <= 'z')))
		{
			char * curp = pos - 1;
			while(curp > startp)
			{
				if(*(byte *)(curp - 1) >= 0x80 || *curp == ' ' || * curp == '\t')
				{
					pos = curp + 1;
					break;
				}
				curp --;
			}
		}
		txt->rows[curs][txt->row_count & 0x3FF].count = pos - startp;
		if(bytetable[*(byte *)pos] > 0)
		{
			if(*pos == '\r' && *(pos + 1) == '\n')
				pos += 2;
			else
				++ pos;
		}
		txt->row_count ++;
	}
	return true;
}

extern p_fs_text fs_open_text(const char * filename, t_fs_filetype ft, dword rowbytes, t_conf_encode encode)
{
	int fd;
	p_fs_text txt = (p_fs_text)calloc(1, sizeof(t_fs_text));
	if(txt == NULL)
		return NULL;
	if((fd = sceIoOpen(filename, PSP_O_RDONLY, 0777)) < 0)
	{
		fs_close_text(txt);
		return NULL;
	}
	strcpy(txt->filename, filename);
	txt->size = sceIoLseek32(fd, 0, PSP_SEEK_END);
	if((txt->buf = (char *)calloc(1, txt->size + 1)) == NULL)
	{
		sceIoClose(fd);
		fs_close_text(txt);
		return NULL;
	}
	sceIoLseek32(fd, 0, PSP_SEEK_SET);
	sceIoRead(fd, txt->buf, txt->size);
	sceIoClose(fd);
	txt->buf[txt->size] = 0;
	fs_decode_text(txt, encode);
	if(ft == fs_filetype_html)
		txt->size = html_to_text(txt->buf);
	if(!fs_format_text(txt, rowbytes))
	{
		fs_close_text(txt);
		return NULL;
	}
	return txt;
}

extern p_fs_text fs_open_text_in_zip(const char * zipfile, const char * filename, t_fs_filetype ft, dword rowbytes, t_conf_encode encode)
{
	p_fs_text txt = (p_fs_text)calloc(1, sizeof(t_fs_text));
	if(txt == NULL)
		return NULL;
	unzFile unzf = unzOpen(zipfile);
	if(unzf == NULL)
	{
		fs_close_text(txt);
		return NULL;
	}
	if(unzLocateFile(unzf, filename, 0) != UNZ_OK || unzOpenCurrentFile(unzf) != UNZ_OK)
	{
		fs_close_text(txt);
		unzClose(unzf);
		return NULL;
	}
	strcpy(txt->filename, filename);
	unz_file_info info;
	if(unzGetCurrentFileInfo(unzf, &info, NULL, 0, NULL, 0, NULL, 0) != UNZ_OK)
	{
		fs_close_text(txt);
		unzCloseCurrentFile(unzf);
		unzClose(unzf);
		return NULL;
	}
	txt->size = info.uncompressed_size;
	if((txt->buf = (char *)calloc(1, txt->size)) == NULL)
	{
		fs_close_text(txt);
		unzCloseCurrentFile(unzf);
		unzClose(unzf);
		return NULL;
	}
	txt->size = unzReadCurrentFile(unzf, txt->buf, txt->size);
	unzCloseCurrentFile(unzf);
	unzClose(unzf);
	fs_decode_text(txt, encode);
	if(ft == fs_filetype_html)
		txt->size = html_to_text(txt->buf);
	if(!fs_format_text(txt, rowbytes))
	{
		fs_close_text(txt);
		return NULL;
	}
	return txt;
}

extern p_fs_text fs_open_text_in_chm(const char * chmfile, const char * filename, t_fs_filetype ft, dword rowbytes, t_conf_encode encode)
{
	p_fs_text txt = (p_fs_text)calloc(1, sizeof(t_fs_text));
	if(txt == NULL)
		return NULL;
	struct chmFile * chm = chm_open(chmfile);
	if(chm == NULL)
	{
		fs_close_text(txt);
		return NULL;
	}
	struct chmUnitInfo ui;
    if (chm_resolve_object(chm, filename, &ui) != CHM_RESOLVE_SUCCESS)
	{
		fs_close_text(txt);
		chm_close(chm);
		return NULL;
	}
	strcpy(txt->filename, filename);
	txt->size = ui.length;
	if((txt->buf = (char *)calloc(1, txt->size)) == NULL)
	{
		fs_close_text(txt);
		chm_close(chm);
		return NULL;
	}
	txt->size = chm_retrieve_object(chm, &ui, (byte *)txt->buf, 0, txt->size);
	chm_close(chm);
	fs_decode_text(txt, encode);
	if(ft == fs_filetype_html)
		txt->size = html_to_text(txt->buf);
	if(!fs_format_text(txt, rowbytes))
	{
		fs_close_text(txt);
		return NULL;
	}
	return txt;
}

extern void fs_close_text(p_fs_text fstext)
{
	if(fstext != NULL)
	{
		dword i;
		if(fstext->buf != NULL)
			free((void *)fstext->buf);
		for(i = 0; i < 1024; ++ i)
			if(fstext->rows[i] != NULL)
				free((void *)fstext->rows[i]);
	}
}

extern t_fs_filetype fs_file_get_type(const char * filename)
{
	const char * ext = utils_fileext(filename);
	if(ext == NULL)
		return fs_filetype_unknown;
	t_fs_filetype_entry * entry = ft_table;
	while(entry->ext != NULL)
	{
		if(stricmp(ext, entry->ext) == 0)
			return entry->ft;
		entry ++;
	}
	return fs_filetype_unknown;
}

extern bool fs_is_image(t_fs_filetype ft)
{
	return ft == fs_filetype_jpg || ft == fs_filetype_gif || ft == fs_filetype_png || ft == fs_filetype_tga || ft == fs_filetype_bmp;
}

extern bool fs_is_txtbook(t_fs_filetype ft)
{
	return ft == fs_filetype_txt || ft == fs_filetype_html;
}
