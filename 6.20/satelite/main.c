#include <stdio.h>

#include "pspsdk.h"
#include "pspkernel.h"

#include "systemctrl_se.h"
#include "kubridge.h"

#define ALLKEYS 0x0083F3F9

PSP_MODULE_INFO("TNVshMenu", 0, 1, 0);

static void __strcpy(char *, char *); /* 0x00000E9C */
static int main_thread(SceSize, void *); /* 0x000008A4 */
static int vsh_menu_ctrl(SceCtrlData *, int); /* 0x000000D4 */

int g_00001E08;
int g_thread_id = 0; /* 0x00001E10 */
TNConfig g_config = {0}; /* 0x00001E94 */

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
	"EXIT"
}; /* 0x00001530 */

char *g_regions[] = {
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
};

char *g_choices[] = {
	"Enable",
	"Disable"
}; /* 0x000015A8 */

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

	g_00001E08 = 1;
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
	int r;

	sceKernelChangeThreadPriority(0, 0x8);
	sctrlSEGetConfig(&g_config);
	vctrlVSHRegisterVshMenu(vsh_menu_ctrl);

	do {
		sceDisplayWaitVblankStart();

		if (g_00001DF0 == 1) {
			sub_000002F8(&g_config);
			g_00001DF0++;
		} else if (g_00001DF0 == 2) {
			sub_00000170();
			sub_000002F8(&g_config);
		}

		if (g_00001E0C == 0) {
			if (g_00001DD4 == 0)
				continue;
			if (g_00001DD4 & 0x1 == 0)
				g_00001E0C = 1;
		} else if (g_00001E0C == 1) {
			if (g_00001DF0 == 0) {
				g_00001DF0 = 1;
				continue;
			}
			r = sub_00000504(g_00001DD4, g_00001DD8);
			if (r == 0) {
				g_00001E0C = 2;
			} else if (r == -1) {
				g_00001E0C = 3;
			} else if (r == -2) {
				g_00001E0C = 4;
			} else if (r == -3) {
				g_00001E0C = 5;
			} else if (r == -4) {
				g_00001E0C = 6;
			}
		} else if (g_00001E0C == 2) {
			if (!(g_00001DD4 & ALLKEYS)) {
				g_00001E08 = 1;
			}
		} else {
			if (g_00001E0C - 3 < 4) {
				if (!(g_00001DD4 & ALLKEYS))
					g_00001E08 = g_00001E0C - 1;
			}
		}

	} while (g_00001E08 == 0);

	if (g_00001DBC != 0)
		sctrlSESetConfig(&g_config);

	switch(g_00001E08) {
	case 2:
		scePowerRequestStandby();
		break;

	case 3:
		scePowerRequestSuspend();
		break;

	case 4:
		scePowerRequestColdReset(0);
		break;

	case 5:
		sctrlKernelExitVSH(0);
		break;
	
	default:
		break;
	}

	vctrlVSHExitVSHMenu(&g_config);
	sceKernelExitDeleteThread(0);

	return 0;
}

/* 0x000000D4 */
static int
vsh_menu_ctrl(SceCtrlData *pad_data, int count)
{
	return 0;
}
