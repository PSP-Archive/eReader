#include "power.h"
#include <psppower.h>
			
extern void power_set_clock(dword cpu, dword bus)
{
	scePowerSetCpuClockFrequency(cpu);
	scePowerSetBusClockFrequency(bus);
}
