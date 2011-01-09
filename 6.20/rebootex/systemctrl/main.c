#include <stdio.h>
#include <string.h>

#include "pspsdk.h"
#include "pspkernel.h"
#include "psputility.h"
#include "psputilsforkernel.h"

PSP_MODULE_INFO("SystemControl", 0x1007, 1, 1);
PSP_MAIN_THREAD_ATTR(0);

void
ClearCaches(void)
{
	sceKernelIcacheInvalidateAll();
	sceKernelDcacheWritebackInvalidateAll();
}


int
module_bootstart(void)
{
	ClearCaches();
	return 0;
}
