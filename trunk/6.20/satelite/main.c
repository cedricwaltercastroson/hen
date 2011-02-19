#include <stdio.h>

#include "pspsdk.h"
#include "pspkernel.h"

#include "systemctrl_se.h"
#include "kubridge.h"

PSP_MODULE_INFO("TNVshMenu", 0, 1, 0);

static void __strcpy(char *, char *); /* 0x00000E9C */
static int main_thread(SceSize, void *); /* 0x000008A4 */

int g_thread_id = 0;

char *g_menu_items[] = {
	"CPU CLOCK XMB    ",
	"CPU CLOCK GAME   ",
	"FAKE REGION      ",
	"SKIP GAMEBOOT    ",
	"HIDE MAC ADDRESS ",
	"TN NETWORK UPDATE",
	"HIDE PIC0 & PIC1 ",
	"SPOOF VERSION    ",
	"USB CHARGE       ",
	"FAST SCROLL MUSIC",
	"PROTECT FLASH0   ",
	"SHUTDOWN DEVICE",
	"SUSPEND DEVICE",
	"RESET DEVICE",
	"RESTART VSH",
	"EXIT",
	"Disabled",
	"Japan",
	"America",
	"Europe",
	"Korea",
	"United Kingdom",
	"Mexico",
	"Australia/New Zealand",
	"East",
	"Taiwan",
	"Russia",
	"China",
	"Debug Type I",
	"Debug Type II",
	"Enabled",
}; /* 0x00001530 */

int
main(void)
{
	if (kuKernelGetModel() == 0) {
		__strcpy(g_menu_items[8], "SLIM COLORS      ");
	}

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

/* 0x00000E9C */
static void
__strcpy(char *dst, char *src)
{
	while ((*dst++ = *src++));
}

/* 0x000008A4 */
static int
main_thread(SceSize args, void *argp)
{
	return 0;
}
