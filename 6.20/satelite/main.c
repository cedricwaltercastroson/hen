#include <stdio.h>

#include "pspsdk.h"
#include "pspkernel.h"

#include "systemctrl.h"
#include "kubridge.h"

#define ALLKEYS 0x0083F3F9

PSP_MODULE_INFO("TNVshMenu", 0, 1, 0);

static void __strcpy(char *, char *); /* 0x00000E9C */
static int main_thread(SceSize, void *); /* 0x000008A4 */
static int vsh_menu_ctrl(SceCtrlData *, int); /* 0x000000D4 */
static void parseconfig(TNConfig *); /* 0x000002F8 */
static int cpuspeed_index(int); /* 0x00000B48 */
static int busspeed_index(int); /* 0x00000B7C */

int g_cur_buttons = 0; /* 0x00001DD4 */
int g_buttons_on = 0; /* 0x00001DD8 */

int g_running_status = 0;
int g_thread_id = 0; /* 0x00001E10 */
SceCtrlData g_pad_data = {0}; /* 0x00001DF8 */

TNConfig *g_config = NULL; /* 0x00001DF4 */

char *g_menu[16] = {0}; /* 0x00001E14 */


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
}; /* 0x00001570 */

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
			parseconfig(&config);
			g_00001DF0++;
		} else if (g_00001DF0 == 2) {
			sub_00000170();
			parseconfig(&config);
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
	static char vshspeed[8] = {0}; /* 0x00001DCC */
	static char isospeed[8] = {0}; /* 0x00001DC4 */

	int i;

	g_config = config;
	for (i = 0; i < 16; i++)
		g_menu[i] = NULL;

	if (cpuspeed_index(config->vshcpuspeed) == 0 ||
			busspeed_index(config->vshbusspeed) == 0) {
		scePaf_sprintf(vshspeed, "Default");
	} else {
		scePaf_sprintf(vshspeed, "%d/%d", config->vshcpuspeed, config->vshbusspeed);
	}
	g_menu[0] = vshspeed;

	if (cpuspeed_index(config->isocpuspeed) == 0 ||
			busspeed_index(config->isobusspeed) == 0) {
		scePaf_sprintf(isospeed, "Default");
	} else {
		scePaf_sprintf(isospeed, "%d/%d", config->isocpuspeed, config->vshisospeed);
	}
	g_menu[1] = isospeed;

	g_menu[2] = g_region[config->fakeregion];
	g_menu[3] = g_choices[!config->skipgameboot];
	g_menu[4] = g_choices[config->showmac];
	g_menu[5] = g_choices[config->notnupdate];
	g_menu[6] = g_choices[!config->hidepic];
	g_menu[7] = g_choices[config->nospoofversion];
	g_menu[8] = g_choices[!config->slimcolor];
	g_menu[9] = g_choices[!config->skipgameboot];
	g_menu[10] = g_choices[!config->protectflash];
}

/* 0x00000B48 */
static int cpuspeed_index(int speed)
{
	int cpu = { 0, 20, 75, 100, 133, 222, 266, 300, 333 };
	int i;

	for (i = 0; i < 9; i++) {
		if (speed == cpu[i])
			return i;
	}

	return 0;
}

/* 0x00000B7C */
static int busspeed_index(int speed)
{
	int bus = { 0, 10, 37, 50, 66, 111, 133, 150, 166 };
	int i;

	for (i = 0; i < 9; i++) {
		if (speed == bus[i])
			return i;
	}

	return 0;
}
