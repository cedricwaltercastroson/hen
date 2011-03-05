#include <stdio.h>

#include "pspsdk.h"
#include "pspkernel.h"
#include "pspctrl.h"
#include "pspdisplay.h"
#include "psppower.h"

#include "systemctrl.h"
#include "kubridge.h"

#define ALL_KEYS 0x0083F3F9
#define NON_HOLD_KEYS  (ALL_KEYS & (~PSP_CTRL_HOLD))

PSP_MODULE_INFO("TNVshMenu", 0, 1, 0);

extern int scePowerRequestColdReset(int);
extern void scePaf_memcpy(void *, void *, int);
extern int scePaf_sprintf(char *, const char *, ...);

static void __strcpy(char *, char *); /* 0x00000E9C */
static int main_thread(SceSize, void *); /* 0x000008A4 */
static int vsh_menu_ctrl(SceCtrlData *, int); /* 0x000000D4 */
static void parseconfig(TNConfig *); /* 0x000002F8 */
static int cpuspeed_index(int); /* 0x00000B48 */
static int busspeed_index(int); /* 0x00000B7C */
static int button_action(int, int); /* 0x00000504 */
static int normalize(int, int, int); /* 0x00000B28 */
static int draw_menu(void); /* 0x00000170 */
static int draw_string(int, int, char *); /* 0x00000C24 */
static void set_color(int, int); /* 0x00000C10 */
static int draw_init(void); /* 0x00000E04 */
static int adjust_alpha(int color); /* 0x00000BB0 */

int g_cur_buttons = 0; /* 0x00001DD4 */
int g_buttons_on = 0; /* 0x00001DD8 */

int g_running_status = 0; /* 0X00001E08 */
int g_thread_id = 0; /* 0x00001E10 */

TNConfig *g_config = NULL; /* 0x00001DF4 */

char *g_menu[16] = {0}; /* 0x00001E14 */

static int g_cpu_speeds[] = { 0, 20, 75, 100, 133, 222, 266, 300, 333 }; /* 0x000013E8 */
static int g_bus_speeds[] = { 0, 10, 37, 50, 66, 111, 133, 150, 166 }; /* 0x0000140C */

int g_cur_index = 0; /* 0x00001DC0 */
int g_config_updated = 0; /* 0x00001DBC */

int g_font_color = 0x00FFFFFF; /* 0x000015B0 */
int g_bg_color = 0xFF000000; /* 0x000015B4 */

int g_width = 0; /* 0x00001DDC */
int g_height = 0; /* 0x00001DE0 */

u32 *g_vram32 = NULL; /* 0x00001DEC */
int g_buffer_width = 0; /* 0x00001DE4 */
int g_pixel_format = 0; /* 0x00001DE8 */


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

int
module_start(SceSize args, void* argp)
{
	if (kuKernelGetModel() == 0)
		__strcpy(g_menu_items[8], "SLIM COLORS      ");

	if ((g_thread_id = 
				sceKernelCreateThread("VshMenu_Thread", main_thread, 0x10, 0x1000, 0, 0)) >= 0)
		sceKernelStartThread(g_thread_id, 0, 0);

	return 0;
}

int
module_stop(SceSize args, void* argp)
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
	static int g_00001E0C = 0;
	static int g_00001DF0 = 0;

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
			draw_menu();
			parseconfig(&config);
		}

		if (g_00001E0C == 0) {
			if (g_cur_buttons == 0)
				continue;
			if (!(g_cur_buttons & PSP_CTRL_SELECT))
				g_00001E0C = 1;
		} else if (g_00001E0C == 1) {
			if (g_00001DF0 == 0) {
				g_00001DF0 = 1;
				continue;
			}
			r = button_action(g_cur_buttons, g_buttons_on);
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
			if (!(g_cur_buttons & ALL_KEYS)) {
				g_running_status = 1;
			}
		} else {
			if (g_00001E0C - 3 < 4) {
				if (!(g_cur_buttons & ALL_KEYS))
					g_running_status = g_00001E0C - 1;
			}
		}

	} while (1);

	if (g_config_updated != 0)
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
	return sceKernelExitDeleteThread(0);
}

/* 0x000000D4 */
static int
vsh_menu_ctrl(SceCtrlData *pad_data, int count)
{
	static SceCtrlData g_pad_data = {0}; /* 0x00001DF8 */

	int i;

	scePaf_memcpy(&g_pad_data, pad_data, sizeof(SceCtrlData));
	g_buttons_on = (~g_cur_buttons) & g_pad_data.Buttons;
	g_cur_buttons = g_pad_data.Buttons;

	for (i = 0; i < count; i++)
		pad_data[i].Buttons &= ~ALL_KEYS;

	return 0;
}

/* 0x000002F8 */
static void
parseconfig(TNConfig *config)
{
	static char *g_regions[] = {
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

	static char *g_choices[] = {
		"Enable",
		"Disable"
	}; /* 0x000015A8 */

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

	if (cpuspeed_index(config->umdisocpuspeed) == 0 ||
			busspeed_index(config->umdisobusspeed) == 0) {
		scePaf_sprintf(isospeed, "Default");
	} else {
		scePaf_sprintf(isospeed, "%d/%d", config->umdisocpuspeed, config->umdisobusspeed);
	}
	g_menu[1] = isospeed;

	g_menu[2] = g_regions[config->fakeregion];
	g_menu[3] = g_choices[!config->skipgameboot];
	g_menu[4] = g_choices[config->showmac];
	g_menu[5] = g_choices[config->notnupdate];
	g_menu[6] = g_choices[!config->hidepic];
	g_menu[7] = g_choices[config->nospoofversion];
	g_menu[8] = g_choices[!config->slimcolor];
	g_menu[9] = g_choices[!config->fastscroll];
	g_menu[10] = g_choices[!config->protectflash];
}

/* 0x00000B48 */
static int
cpuspeed_index(int speed)
{
	int i;

	for (i = 0; i < 9; i++) {
		if (speed == g_cpu_speeds[i])
			return i;
	}

	return 0;
}

/* 0x00000B7C */
static int
busspeed_index(int speed)
{
	int i;

	for (i = 0; i < 9; i++) {
		if (speed == g_bus_speeds[i])
			return i;
	}

	return 0;
}

/* 0x00000504 */
static int
button_action(int cur_buttons, int buttons_on)
{
	int dir; /* up or down */
	int idx; /* current index */
	int r;

	buttons_on &= NON_HOLD_KEYS;
	dir = (!!(buttons_on & PSP_CTRL_DOWN)) - (!!(buttons_on & PSP_CTRL_UP));

	do {
		idx = normalize(g_cur_index + dir, 0, 15);
		g_cur_index = idx;
	} while (g_menu_items[idx] == NULL);

	r = -2;
	if (buttons_on & PSP_CTRL_LEFT)
		r = -1;
	if (buttons_on & PSP_CTRL_CROSS)
		r = 0;
	if (buttons_on & PSP_CTRL_RIGHT)
		r = 1;
	if (!(buttons_on & (PSP_CTRL_HOME | PSP_CTRL_SELECT))) {
		if (r == -2)
			return 1;
	} else {
		g_cur_index = 15;
		r = 0;
	}

	if (g_config_updated == 0) {
		if (r != 0)
			g_config_updated = 1;
	}

	if (g_cur_index >= 16)
		return 1;
	
#define update_cpuspeed(__spd, __off) do {\
	*__spd = g_cpu_speeds[normalize(cpuspeed_index(*(__spd)) + (__off), 0, 8)];\
} while (0)

#define update_busspeed(__spd, __off) do {\
	*__spd = g_bus_speeds[normalize(busspeed_index(*(__spd)) + (__off), 0, 8)];\
} while (0)

	switch (g_cur_index) {
	case 0:
		if (r != 0) {
			update_cpuspeed(&g_config->vshcpuspeed, r);
			update_busspeed(&g_config->vshbusspeed, r);
		}
		return 1;

	case 1:
		if (r != 0) {
			update_cpuspeed(&g_config->umdisocpuspeed, r);
			update_busspeed(&g_config->umdisobusspeed, r);
		}
		return 1;

	case 2:
		if (r != 0)
			g_config->fakeregion = normalize(g_config->fakeregion + r, 0, 13);
		return 1;

	case 3:
		if (r != 0)
			g_config->skipgameboot = normalize(g_config->skipgameboot + r, 0, 1);
		return 1;

	case 4:
		if (r != 0)
			g_config->showmac = normalize(g_config->showmac + r, 0, 1);
		return 1;

	case 5:
		if (r != 0)
			g_config->notnupdate = normalize(g_config->notnupdate + r, 0, 1);
		return 1;

	case 6:
		if (r != 0)
			g_config->hidepic = normalize(g_config->hidepic + r, 0, 1);
		return 1;

	case 7:
		if (r != 0)
			g_config->nospoofversion = normalize(g_config->nospoofversion + r, 0, 1);
		return 1;

	case 8:
		if (r != 0)
			g_config->slimcolor = normalize(g_config->slimcolor + r, 0, 1);
		return 1;

	case 9:
		if (r != 0)
			g_config->fastscroll = normalize(g_config->fastscroll + r, 0, 1);
		return 1;

	case 10:
		if (r != 0)
			g_config->protectflash = normalize(g_config->protectflash + r, 0, 1);
		return 1;

	case 11:
		return -1;

	case 12:
		return -2;

	case 13:
		return -3;

	case 14:
		return -4;

	case 15:
		return (r > 0);

	default:
		return r;
	}
}

/* 0x00000B28 */
static int
normalize(int val, int min, int max)
{
	if (val < min) 
		return max;
	else if (val > max)
		return min;

	return val;
}

/* 0x00000170 */
static int
draw_menu(void)
{
	int i, color, x;

	if (draw_init() < 0)
		return -1;

	set_color(0x00FFFFFF, 0x8000FF00);
	draw_string(0xC0, 0x30, "TN VSH MENU");

	for (i = 0; i < 0x10; i++) {
		color = g_cur_index == i ? 0x00FF0000 : 0xC0FF8080;
		set_color(0x00FFFFFF, color);
		if (g_menu_items[i] != NULL) {
			if (i == 0xF)
				x = 0xD8;
			else if (i == 0xE)
				x = 0xC0;
			else if (i == 0xD)
				x = 0xB8;
			else if (i == 0xC)
				x = 0xB0;
			else if (i == 0xB)
				x = 0xB0;
			else
				x = 0x88;

			draw_string(x, 0x40 + 8 * i, g_menu_items[i]);

			if (g_menu[i] != NULL) {
				set_color(0x00FFFFFF, color);
				draw_string(0x118, 0x40 + 8 * i, g_menu[i]);
			}
		}
	}

	set_color(0x00FFFFFF, 0);
	return 0;
}

extern u8 msx[]; /* 0x000015B8 */

/* 0x00000C24 */
static int
draw_string(int x, int y, char *msg)
{
	int i, j, k;
	int offset;
	char code;
	u8 font;

	u32 color, fg, bg;
	u32 c1;
	u32 c2;
	u32 alpha;

	if((g_buffer_width == 0) || (g_pixel_format != 3))
		return -1;

	fg = adjust_alpha(g_font_color);
	bg = adjust_alpha(g_bg_color);

	for(i = 0; msg[i] && i < (g_width / 8); i++) {
		code = msg[i] & 0x7F;
		for(j = 0; j < 8; j++) {
			offset = (y + j) * g_buffer_width + x + i * 8;
			font = j == 7 ? 0x00 : msx[code * 8 + j];
			for(k = 0; k < 8; k++) {
				color = (font & 0x80) ? fg : bg;
				alpha = color >> 24;
				if (alpha == 0) {
					g_vram32[offset] = color;
				} else if(alpha != 0xFF) {
					c2 = g_vram32[offset];
					c1 = c2 & 0x00FF00FF;
					c2 = c2 & 0x0000FF00;
					c1 = ((c1 * alpha) >> 8) & 0x00FF00FF;
					c2 = ((c2 * alpha) >> 8) & 0x0000FF00;
					g_vram32[offset] = (color & 0x00FFFFFF) + c1 + c2;
				}

				font <<= 1;
				offset++;
			}
		}
	}

	return i;
}

/* 0x00000C10 */
static void
set_color(int font, int bg)
{
	g_font_color = font;
	g_bg_color = bg;
}

/* 0x00000E04 */
static int
draw_init(void)
{
	int mode;

	sceDisplayGetMode(&mode, &g_width, &g_height);
	sceDisplayGetFrameBuf((void **) &g_vram32, &g_buffer_width, &g_pixel_format, PSP_DISPLAY_SETBUF_NEXTFRAME);
	if (g_buffer_width == 0)
		return -1;
	if (g_pixel_format != 3)
		return -1;
	g_font_color = 0x00FFFFFF;
	g_bg_color = 0xFF000000;

	return 0;
}

/* 0x00000BB0 */
static int
adjust_alpha(int col)
{
	u32 alpha = col >> 24;
	u8 mul;
	u32 c1,c2;

	if(alpha == 0)
		return col;
	if(alpha == 0xFF)
		return col;

	c1 = col & 0x00FF00FF;
	c2 = col & 0x0000FF00;
	mul = (u8) (255 - alpha);
	c1 = ((c1 * mul) >> 8) & 0x00FF00FF;
	c2 = ((c2 * mul) >> 8) & 0x0000FF00;

	return (alpha << 24) | c1 | c2;
}
