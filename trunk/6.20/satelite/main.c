#include <stdio.h>

#include "pspsdk.h"
#include "pspkernel.h"

#include "systemctrl_se.h"
#include "kubridge.h"

PSP_MODULE_INFO("TNVshMenu", 0, 1, 0);

static void sub_00000E9C(char **, char *);
static int main_thread(SceSize, void *); /* 0x000008A4 */

int g_thread_id = 0;
char g_network_item[] = "TN NETWORK UPDATE"; /* 0x00001550 */

int
main(void)
{
	if (kuKernelGetModel() == 0)
		sub_00000E9C(&g_network_item, "SLIM COLORS      ");

	if ((g_thread_id = 
				sceKernelCreateThread("VshMenu_Thread", main_thread, 0x10, 0x1000, 0, 0)) >= 0)
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

static void
sub_00000E9C(char **dst, char *src)
{
}

/* 0x000008A4 */
static int
main_thread(SceSize args, void *argp)
{
	return 0;
}
