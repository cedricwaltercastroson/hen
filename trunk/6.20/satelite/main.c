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
static void parseconfig(TNConfig *); /* 0x000002F8 */

int g_cur_buttons = 0; /* 0x00001DD4 */
int g_buttons_on = 0; /* 0x00001DD8 */

int g_running_status = 0;
int g_thread_id = 0; /* 0x00001E10 */
SceCtrlData g_pad_data = {0}; /* 0x00001DF8 */

TNConfig *g_config = NULL; /* 0x00001DF4 */


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
	if (kuKernelGetModel() == 0)
		__strcpy(g_menu_items[8], "SLIM COLORS      ");

	if ((g_thread_id = 
				sceKernelCreateThread("VshMenu_Thread", main_thread, 0x10, 0x1000, 0, 0)) >= 0)
		sceKernelStartThread(g_thread_id, 0, 0);

	return 0;
}

int
module_stop(void)
{
	SceUInt timeout = 100000;

	g_running_status = 1;
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
	static TNConfig config = {0}; /* 0x00001E94 */
	int r;

	sceKernelChangeThreadPriority(0, 0x8);
	sctrlSEGetConfig(&config);
	vctrlVSHRegisterVshMenu(vsh_menu_ctrl);

	do {
		sceDisplayWaitVblankStart();
		if (g_running_status != 0)
			break;

		if (g_00001DF0 == 1) {
			sub_000002F8(&config);
			g_00001DF0++;
		} else if (g_00001DF0 == 2) {
			sub_00000170();
			sub_000002F8(&config);
		}

		if (g_00001E0C == 0) {
			if (g_cur_buttons == 0)
				continue;
			if (g_cur_buttons & PSP_CTRL_SELECT == 0)
				g_00001E0C = 1;
		} else if (g_00001E0C == 1) {
			if (g_00001DF0 == 0) {
				g_00001DF0 = 1;
				continue;
			}
			r = sub_00000504(g_cur_buttons, g_buttons_on);
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
			if (!(g_cur_buttons & ALLKEYS)) {
				g_running_status = 1;
			}
		} else {
			if (g_00001E0C - 3 < 4) {
				if (!(g_cur_buttons & ALLKEYS))
					g_running_status = g_00001E0C - 1;
			}
		}

	} while (1);

	if (g_00001DBC != 0)
		sctrlSESetConfig(&config);

	switch(g_running_status) {
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

	vctrlVSHExitVSHMenu(&config);
	sceKernelExitDeleteThread(0);

	return 0;
}

/* 0x000000D4 */
static int
vsh_menu_ctrl(SceCtrlData *pad_data, int count)
{
	int i;

	scePaf_memcpy(&g_pad_data, pad_data, sizeof(SceCtrlData));
	g_buttons_on = (~g_cur_buttons) & g_pad_data.Buttons;
	g_cur_buttons = g_pad_data.Buttons;

	for (i = 0; i < count; i++)
		pad_data[i].Buttons &= ~ALLKEYS;

	return 0;
}

/* 0x000002F8 */
static void
parseconfig(TNConfig *config)
{
	g_config = config;
	/* XXX */
}
