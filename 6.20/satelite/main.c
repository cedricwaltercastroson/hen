#include <stdio.h>

#include "pspsdk.h"
#include "pspkernel.h"

#include "systemctrl_se.h"
#include "kubridge.h"

PSP_MODULE_INFO("TNVshMenu", 0, 1, 0);

static void sub_00000E9C(char **, char *);
static int thread_entry(SceSize, void *); /* 0x000008A4 */

int g_thread_id = 0;
char *g_menu_items[] = {""};

int
main(void)
{
	if (kuKernelGetModel() == 0)
		sub_00000E9C(g_menu_items, "SLIM COLORS      ");

	if ((g_thread_id = 
				sceKernelCreateThread("VshMenu_Thread", thread_entry, 0x10, 0x1000, 0, 0)) >= 0)
		sceKernelStartThread(g_thread_id, 0, 0);

	return 0;
}

int
module_stop(void)
{
	SceUInt timeout = 100000;

	if (sceKernelWaitThreadEnd(g_thread_id, &timeout) < 0)
		sceKernelTerminateDeleteThread(g_thread_id);
	return 0;
}
