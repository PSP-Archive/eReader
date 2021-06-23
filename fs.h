#ifndef _FS_H_
#define _FS_H_

#include "common/datatype.h"
#include "conf.h"

typedef struct {
	char * start;
	dword count;
} t_fs_textrow, * p_fs_textrow;

typedef enum {
	fs_filetype_unknown,
	fs_filetype_txt,
	fs_filetype_html,
	fs_filetype_png,
	fs_filetype_gif,
	fs_filetype_jpg,
	fs_filetype_bmp,
	fs_filetype_tga,
	fs_filetype_dir,
	fs_filetype_zip,
	fs_filetype_chm,
} t_fs_filetype;

typedef struct {
	char filename[256];
	dword crow;
	dword size;
	char * buf;
	bool isucs;
	dword row_count;
	p_fs_textrow rows[1024];
} t_fs_text, * p_fs_text;

extern dword fs_dir_to_menu(const char * dir, const char * sdir, p_win_menuitem * mitem, dword icolor, dword selicolor, dword selrcolor, dword selbcolor);
extern dword fs_zip_to_menu(const char * zipfile, p_win_menuitem * mitem, dword icolor, dword selicolor, dword selrcolor, dword selbcolor);
extern dword fs_chm_to_menu(const char * chmfile, p_win_menuitem * mitem, dword icolor, dword selicolor, dword selrcolor, dword selbcolor);
extern bool fs_format_text(p_fs_text txt, dword rowbytes);
extern p_fs_text fs_open_binary(const char * filename, bool vert);
extern p_fs_text fs_open_text(const char * filename, t_fs_filetype ft, dword rowbytes, t_conf_encode encode);
extern p_fs_text fs_open_text_in_zip(const char * zipfile, const char * filename, t_fs_filetype ft, dword rowbytes, t_conf_encode encode);
extern p_fs_text fs_open_text_in_chm(const char * chmfile, const char * filename, t_fs_filetype ft, dword rowbytes, t_conf_encode encode);
extern void fs_close_text(p_fs_text fstext);
extern t_fs_filetype fs_file_get_type(const char * filename);
extern bool fs_is_image(t_fs_filetype ft);
extern bool fs_is_txtbook(t_fs_filetype ft);

#endif
