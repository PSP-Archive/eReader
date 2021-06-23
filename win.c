#include <string.h>
#include "display.h"
#include "ctrl.h"
#include "win.h"

extern t_win_menu_op win_menu_defcb(dword key, p_win_menuitem item, dword count, dword max_height, dword topindex, dword * index)
{
	switch(key)
	{
	case PSP_CTRL_UP:
		if(*index == 0)
			*index = count - 1;
		else
			(*index) --;
		return win_menu_op_redraw;
	case PSP_CTRL_DOWN:
		if(*index == count - 1)
			*index = 0;
		else
			(*index) ++;
		return win_menu_op_redraw;
	case PSP_CTRL_LEFT:
		if(*index < max_height - 1 )
			*index = 0;
		else
			*index -= max_height - 1;
		return win_menu_op_redraw;
	case PSP_CTRL_RIGHT:
		if(*index + (max_height - 1) >= count)
			*index = count - 1;
		else
			*index += max_height - 1;
		return win_menu_op_redraw;
	case PSP_CTRL_LTRIGGER:
		*index = 0;
		return win_menu_op_redraw;
	case PSP_CTRL_RTRIGGER:
		*index = count - 1;
		return win_menu_op_redraw;
	case PSP_CTRL_CIRCLE:
		ctrl_waitrelease();
		return win_menu_op_ok;
	case PSP_CTRL_CROSS:
		ctrl_waitrelease();
		return win_menu_op_cancel;
	}
	return win_menu_op_continue;
}

extern dword win_menu(dword x, dword y, dword max_width, dword max_height, p_win_menuitem item, dword count, dword initindex, dword linespace, dword bgcolor, t_win_menu_draw predraw, t_win_menu_draw postdraw, t_win_menu_callback cb)
{
	dword i, index = initindex, topindex, botindex, lastsel = index;
	bool needrp = true;
	if(cb == NULL)
		cb = win_menu_defcb;
	disp_duptocache();
	topindex = (index >= max_height) ? (index - max_height + 1) : 0;
	botindex = (topindex + max_height > count) ? (count - 1) : (topindex + max_height - 1);
	while(1)
	{
		t_win_menu_op op;
		disp_waitv();
		if(predraw != NULL)
			predraw(item, index, topindex, max_height);
		if(needrp)
		{
			disp_fillrect(x, y, x + 2 + max_width * (DISP_FONTSIZE / 2), y + 2 + DISP_FONTSIZE + (max_height - 1) * (1 + DISP_FONTSIZE + linespace), bgcolor);
			for(i = topindex; i <= botindex; i ++)
				if(i != index)
					disp_putstring(x + 1, y + 1 + (i - topindex) * (DISP_FONTSIZE + 1 + linespace), item[i].icolor, (const byte *)item[i].name);
			if(max_height < count)
			{
				dword sbh = 2 + DISP_FONTSIZE + (max_height - 1) * (1 + DISP_FONTSIZE + linespace);
				disp_line(x - 1 + max_width * (DISP_FONTSIZE / 2), y, x - 1 + max_width * (DISP_FONTSIZE / 2), y + 2 + DISP_FONTSIZE + (max_height - 1) * (1 + DISP_FONTSIZE + linespace), 0xFFFFFFFF);
				disp_fillrect(x + max_width * (DISP_FONTSIZE / 2), y + sbh * topindex / count, x + 2 + max_width * (DISP_FONTSIZE / 2), y + sbh * (botindex + 1) / count, 0xFFFFFFFF);
			}
			needrp = false;
		}
		else
		{
			disp_rectduptocache(x, y, x + 2 + max_width * (DISP_FONTSIZE / 2), y + 2 + DISP_FONTSIZE + (max_height - 1) * (DISP_FONTSIZE + 1 + linespace));
			if(item[lastsel].selrcolor != bgcolor || item[lastsel].selbcolor != bgcolor)
				disp_fillrect(x, y + (lastsel - topindex) * (DISP_FONTSIZE + 1 + linespace), x + 1 + DISP_FONTSIZE / 2 * item[lastsel].width, y + DISP_FONTSIZE + 1 + (lastsel - topindex) * (DISP_FONTSIZE + 1 + linespace), bgcolor);
			disp_putstring(x + 1, y + 1 + (lastsel - topindex) * (DISP_FONTSIZE + 1 + linespace), item[lastsel].icolor, (const byte *)item[lastsel].name);
		}
		if(item[index].selrcolor != bgcolor)
			disp_rectangle(x, y + (index - topindex) * (DISP_FONTSIZE + 1 + linespace), x + 1 + DISP_FONTSIZE / 2 * item[index].width, y + DISP_FONTSIZE + 1 + (index - topindex) * (DISP_FONTSIZE + 1 + linespace), item[index].selrcolor);
		if(item[index].selbcolor != bgcolor)
			disp_fillrect(x + 1, y + 1 + (index - topindex) * (DISP_FONTSIZE + 1 + linespace), x + DISP_FONTSIZE / 2 * item[index].width, y + DISP_FONTSIZE + (index - topindex) * (DISP_FONTSIZE + 1 + linespace), item[index].selbcolor);
		disp_putstring(x + 1, y + 1 + (index - topindex) * (DISP_FONTSIZE + 1 + linespace), item[index].selicolor, (const byte *)item[index].name);
		if(postdraw != NULL)
			postdraw(item, index, topindex, max_height);
		disp_flip();
		lastsel = index;
		while((op = cb(ctrl_waitany(), item, count, max_height, topindex, &index)) == win_menu_op_continue);
		switch(op)
		{
		case win_menu_op_ok:
			return index;
		case win_menu_op_cancel:
			return INVALID;
		case win_menu_op_force_redraw:
			needrp = true;
		default:;
		}
		if(index > botindex)
		{
			topindex = index - max_height + 1;
			botindex = index;
			needrp = true;
		}
		else if(index < topindex)
		{
			topindex = index;
			botindex = topindex + max_height - 1;
			needrp = true;
		}
	}
	return INVALID;
}

extern bool win_msgbox(const char * prompt, const char * yesstr, const char * nostr, dword fontcolor, dword bordercolor, dword bgcolor)
{
	dword width = strlen(prompt) * (DISP_FONTSIZE / 4); dword yeswidth = strlen(yesstr) * (DISP_FONTSIZE / 2);
	disp_waitv();
	disp_duptocache();
	disp_rectangle(219 - width, 99, 260 + width, 173, bordercolor);
	disp_fillrect(220 - width, 100, 259 + width, 172, bgcolor);
	disp_putstring(240 - width, 115, fontcolor, (const byte *) prompt);
	disp_putstring(214 - yeswidth, 141, fontcolor, (const byte *) "¡ð");
	disp_putstring(230 - yeswidth, 141, fontcolor, (const byte *) yesstr);
	disp_putstring(250, 141, fontcolor, (const byte *) "¡Á");
	disp_putstring(266, 141, fontcolor, (const byte *) nostr);
	disp_flip();
	
	return (ctrl_waitmask(PSP_CTRL_CIRCLE | PSP_CTRL_CROSS) == PSP_CTRL_CIRCLE);
}
