#include <pspdebug.h>
#include <pspkernel.h>
#include <pspsdk.h>
#include <psppower.h>
#include <pspdisplay.h>
#include "mp3.h"
#include "scene.h"

PSP_MODULE_INFO("EREADER", 0x1000, 0, 1);

PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

__attribute__ ((constructor))
void loaderInit()
{
	pspKernelSetKernelPC();
	pspSdkInstallNoDeviceCheckPatch();
	pspDebugInstallErrorHandler(NULL);
}

static int power_callback(int arg1, int powerInfo, void * arg)
{
	if((powerInfo & PSP_POWER_CB_POWER_SWITCH) > 0)
	{
		mp3_powerdown();
	}
	else if((powerInfo & PSP_POWER_CB_RESUME_COMPLETE) > 0)
	{
		sceKernelDelayThread(1000000);
		mp3_powerup();
	}
	return 0;
}

static int exit_callback(int arg1, int arg2, void *arg)
{
	scene_exit();
	sceKernelExitGame();

	return 0;
}

static int CallbackThread(unsigned int args, void *argp)
{
	int cbid;
	cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	cbid = sceKernelCreateCallback("Power Callback", power_callback, NULL);
	scePowerRegisterCallback(0, cbid);

	sceKernelSleepThreadCB();

	return 0;
}


/* Sets up the callback thread and returns its thread id */ 
static int SetupCallbacks(void) 
{
	int thid = sceKernelCreateThread("Callback Thread", CallbackThread, 0x11, 0xFA0, 0, 0);
	if(thid >= 0)
	{
		sceKernelStartThread(thid, 0, 0);
	}
	return thid;
}

int main(int argc, char *argv[]) 
{
	SetupCallbacks();

	scene_init();

	sceKernelExitDeleteThread(0);

	return 0;
}
