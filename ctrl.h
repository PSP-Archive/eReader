#ifndef _CTRL_H_
#define _CTRL_H_

#include <pspctrl.h>
#include "common/datatype.h"

#define CTRL_ANALOG 0x80000000

extern void ctrl_init();
extern void ctrl_analog(int * x, int * y);
extern dword ctrl_read_cont();
extern dword ctrl_read();
extern void ctrl_waitrelease();
extern dword ctrl_waitany();
extern dword ctrl_waitkey(dword keymask);
extern dword ctrl_waitmask(dword keymask);

#endif
