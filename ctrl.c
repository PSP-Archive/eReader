#include "display.h"
#include "ctrl.h"
#include <pspkernel.h>

#define CTRL_REPEAT_TIME 0x40000
static unsigned int last_btn = 0;
static unsigned int last_tick = 0;

extern void ctrl_init()
{
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
}

extern void ctrl_analog(int * x, int * y)
{
	SceCtrlData ctl;
	sceCtrlReadBufferPositive(&ctl,1);
	*x = ((int)ctl.Lx) - 128;
	*y = ((int)ctl.Ly) - 128;
}

extern dword ctrl_read_cont()
{
	SceCtrlData ctl;

	sceCtrlReadBufferPositive(&ctl,1);

	if (ctl.Lx < 65 || ctl.Lx > 191 || ctl.Ly < 65 || ctl.Ly > 191)
		return CTRL_ANALOG | ctl.Buttons;
	last_btn  = ctl.Buttons;
	last_tick = ctl.TimeStamp;
	return last_btn;
}

extern dword ctrl_read()
{
	SceCtrlData ctl;

	sceCtrlReadBufferPositive(&ctl,1);

	if (ctl.Lx < 65 || ctl.Lx > 191 || ctl.Ly < 65 || ctl.Ly > 191)
		return CTRL_ANALOG;
	if (ctl.Buttons == last_btn) {
		if (ctl.TimeStamp - last_tick < CTRL_REPEAT_TIME) return 0;
		return last_btn;
	}
	last_btn  = ctl.Buttons;
	last_tick = ctl.TimeStamp;
	return last_btn;
}

extern void ctrl_waitrelease()
{
	SceCtrlData ctl;
	do {
		sceCtrlReadBufferPositive(&ctl,1);
	} while (ctl.Buttons != 0);
}

extern dword ctrl_waitany()
{
	dword key;

	while(1) {
		disp_waitv();
		key = ctrl_read();
		if (key != 0) break;
		sceKernelDelayThread(10000);
	}
	return key;
}

extern dword ctrl_waitkey(dword keymask)
{
	dword key;
	while((key = ctrl_read()) != key)
		sceKernelDelayThread(10000);
	return key;
}

extern dword ctrl_waitmask(dword keymask)
{
	dword key;
	while(((key = ctrl_read()) & keymask) == 0)
		sceKernelDelayThread(10000);
	return key;
}
