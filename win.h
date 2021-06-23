#ifndef _WIN_H_
#define _WIN_H_

#include "common/datatype.h"

// Menu item structure
typedef struct {
	char compname[256];
	char shortname[256];
	char name[64];		// item name
	dword width;		// display width in bytes (for align/span)
	dword icolor;		// font color
	dword selicolor;	// selected font color
	dword selrcolor;	// selected rectangle line color
	dword selbcolor;	// selected background filling color
	void * data;		// custom data for user processing
	word data2[4];
	dword data3;
} t_win_menuitem, * p_win_menuitem;

// Menu callback
typedef enum {
	win_menu_op_continue = 0,
	win_menu_op_redraw,
	win_menu_op_ok,
	win_menu_op_cancel,
	win_menu_op_force_redraw
} t_win_menu_op;
typedef t_win_menu_op (*t_win_menu_callback)(dword key, p_win_menuitem item, dword count, dword page_count, dword topindex, dword * index);
typedef void (*t_win_menu_draw)(p_win_menuitem item, dword index, dword topindex, dword max_height);

// Default callback for menu
extern t_win_menu_op win_menu_defcb(dword key, p_win_menuitem item, dword count, dword page_count, dword topindex, dword * index);

// Menu show & wait for input with callback supported
extern dword win_menu(dword x, dword y, dword max_width, dword max_height, p_win_menuitem item, dword count, dword initindex, dword linespace, dword bgcolor, t_win_menu_draw predraw, t_win_menu_draw postdraw, t_win_menu_callback cb);

// Messagebox with yes/no
extern bool win_msgbox(const char * prompt, const char * yesstr, const char * nostr, dword fontcolor, dword bordercolor, dword bgcolor);

#endif