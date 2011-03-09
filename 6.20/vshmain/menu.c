#include <stdio.h>
#include "pspkernel.h"
#include "pspctrl.h"
#include "pspreg.h"
#include "menu.h"
#include "kgtext.h"
#include "../sysctrl/systemctrl.h"

menuitem_t MainItems[] = {
	{"CFW options", 191, SelectMain},
	{"registry hacks", 177, SelectMain},
	{"plugins", 208, SelectMain},
	{NULL},
	{"save and exit", 181, SelectMain},
	{"exit", 222, SelectMain}
};

menu_t MainMenu = {
	"Recovery menu",
	182,
	NULL,
	MainItems,
	sizeof(MainItems) / sizeof(menuitem_t),
	NULL,
	0
};

menuitem_t OptionsItems[] = {
	{"VSH CPU speed", 114, ChangeOption},
	{"game CPU speed", 114, ChangeOption},
	{"fake region", 114, ChangeOption},
	{"skip gameboot", 114, ChangeOption},
	{"hide MAC addr.", 114, ChangeOption},
	{"use VSH menu", 114, ChangeOption},
	{"hide pictures", 114, ChangeOption},
	{"USB charge", 114, ChangeOption},
	{"fast sctroll", 114, ChangeOption},
	{"protect flash0", 114, ChangeOption},
	{"USB shows name", 114, ChangeOption},
	{"main menu", 114, GoBack}
};

menu_t CFWMenu = {
	"CFW options",
	190,
	DrawOptions,
	OptionsItems,
	sizeof(OptionsItems) / sizeof(menuitem_t),
	&MainMenu,
	0
};

menuitem_t RegistryItems[] = {
	{"activate flash player", 146, EnableFlash},
	{"activate WMA player", 155, EnableWMA},
	{"set enter button ?", 159, SwapButtons},
	{"main menu", 200, GoBack}
};

menu_t RegistryMenu = {
	"Registry hacks",
	177,
	NULL,
	RegistryItems,
	sizeof(RegistryItems) / sizeof(menuitem_t),
	&MainMenu,
	0
};

menuitem_t PluginsItems[] = {
	{"flash", 217, SelectPluginMenu},
	{"ms0 VSH", 208, SelectPluginMenu},
	{"ms0 Game", 204, SelectPluginMenu},
	{"ef0 VSH", 208, SelectPluginMenu},
	{"ef0 Game", 204, SelectPluginMenu},
	{"main menu", 200, GoBack}
};

menu_t PluginsMenu = {
	"Select plugin list",
	159,
	NULL,
	PluginsItems,
	sizeof(PluginsItems) / sizeof(menuitem_t),
	&MainMenu,
	0
};

menuitem_t SelectItems[] = {
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
};

menu_t SelectMenu = {
	NULL,
	0,
	NULL,
	SelectItems,
	0,
	&PluginsMenu,
	0
};

char *regions[] = {
	"disabled     ",
	"Japan   ",
	"America",
	"Europe ",
	"Korea         ",
	"United Kingdom",
	"Mexico               ",
	"Australia/New Zealand",
	"East                 ",
	"Taiwan",
	"Russia",
	"China        ",
	"Debug Type I ",
	"Debug Type II",
};

int cpu_speeds[] = { 0, 20, 75, 100, 133, 222, 266, 300, 333 };
int bus_speeds[] = { 0, 10, 37, 50, 66, 111, 133, 150, 166 };
char *txt_speeds[] = {"default", "20/10  ", "75/37 ", "100/50", "133/66 ", "222/111", "266/133", "300/150", "333/166"};

menu_t *CurrMenu;
int save = 0;
int flashplugins = 0;
extern int model;
int button_assign;
int PSP_CTRL_ENTER = PSP_CTRL_CROSS;
int PSP_CTRL_CANCEL = PSP_CTRL_CIRCLE;
CFWconf config;
int vshcpu;
int gamecpu;

int numplugins;
int pluginschanged;
int pluginactive[9];
int flashnum[8];
char *pluginfile;

void SetMenu(menu_t *new)
{
	CurrMenu = new;
	kgt_clear(0);
	if(CurrMenu) {
		kgt_write(CurrMenu->name, CurrMenu->nx, 8, 0xFF00FF, 0);
		if(CurrMenu == &RegistryMenu) {
			if(button_assign) CurrMenu->items[2].text[17] = 'O';
			else CurrMenu->items[2].text[17] = 'X';
		}
	}
}

void ColorWait(int times)
{
	static int ccolor = 0x808080;
	static int colway = 0x040404;
	int i;

	ccolor += colway;
	if(ccolor > 0x00FFFFFF) {
		ccolor = 0x00FFFFFF;
		colway = -0x04 -0x0400 -0x040000;
	}
	if(ccolor < 0x00808080) colway = 0x00040404;
	for(i = 0; i < times; i++) {
		if(CurrMenu->draw != DrawPlugins || CurrMenu->curitem == CurrMenu->numitems - 1)
			kgt_write(CurrMenu->items[CurrMenu->curitem].text, CurrMenu->items[CurrMenu->curitem].x, 48 + CurrMenu->curitem * 16, ccolor, 0);
		sceKernelDelayThread(10*1000);
	}
}

void RecoveryMenu()
{
	int flags = 1;
	int i, f;
	SceCtrlData pad;

	vshcpu = 0;
	gamecpu = 0;

	i = 0;
	f = sceIoOpen("flash1:/kgCFW.cfg", PSP_O_RDONLY, 0777);
	if(f >= 0) {
		i = 1;
		i = sceIoRead(f, &config, sizeof(CFWconf));
		if(i != sizeof(CFWconf) || config.magic != CF_MAGIC || config.version != CF_VERSION)
			i = 0;
		sceIoClose(f);
	}
	if(!i) {
		memset(&config, 0, sizeof(CFWconf));
		config.magic = CF_MAGIC;
		config.version = CF_VERSION;
		config.flags = CF_USEVSHMENU | CF_FLASHPROTECT;
	} else {
		for(i = 0; i < 8; i++) {
			if(config.vshcpuspeed == cpu_speeds[i]) vshcpu = i;
			if(config.gamecpuspeed == cpu_speeds[i]) gamecpu = i;
		}
	}
	if(!model) OptionsItems[7].text = "slim colors";
	if(model != 4) {
		memcpy(&PluginsItems[3], &PluginsItems[5], sizeof(menuitem_t));
		PluginsMenu.numitems -= 2;
	}

	if(sceIoUnassign("flash0:") >= 0) {
		sceIoAssign("flash0:", "lflash0:0,0", "flashfat0:", IOASSIGN_RDWR, NULL, 0);
	}

	ReadRegistry("/CONFIG/SYSTEM/XMB", "button_assign", &button_assign, 4);
	if(!button_assign) {
		PSP_CTRL_ENTER = PSP_CTRL_CIRCLE;
		PSP_CTRL_CANCEL = PSP_CTRL_CROSS;
	}

	kgt_clear(0);
	SetMenu(&MainMenu);
	while(CurrMenu) {
		sceCtrlPeekBufferPositive(&pad, 1);
		if(pad.Buttons & PSP_CTRL_UP) {
			CurrMenu->curitem--;
			if(CurrMenu->curitem < 0) CurrMenu->curitem = CurrMenu->numitems - 1;
			if(!CurrMenu->items[CurrMenu->curitem].text) CurrMenu->curitem--;
			flags |= 1;
		}
		if(pad.Buttons & PSP_CTRL_DOWN) {
			CurrMenu->curitem++;
			if(CurrMenu->curitem > CurrMenu->numitems - 1) CurrMenu->curitem = 0;
			if(!CurrMenu->items[CurrMenu->curitem].text) CurrMenu->curitem++;
			flags |= 1;
		}
		if(pad.Buttons & PSP_CTRL_ENTER && !flags) {
			CurrMenu->items[CurrMenu->curitem].func(flashplugins ^ 1);
			flags |= 3;
		}
		if(pad.Buttons & PSP_CTRL_CANCEL && !flags) {
			if(CurrMenu->prev) {
				GoBack(0);
				flags |= 1;
			}
			flags |= 2;
		}
		if(CurrMenu == &CFWMenu || flashplugins) {
			if(pad.Buttons & PSP_CTRL_LEFT && !flags) {
				CurrMenu->items[CurrMenu->curitem].func(-1);
				flags |= 5;
			}
			if(pad.Buttons & PSP_CTRL_RIGHT && !flags) {
				CurrMenu->items[CurrMenu->curitem].func(1);
				flags |= 9;
			}
		}
		if(flashplugins) {
			if(pad.Buttons & PSP_CTRL_SQUARE && !flags) {
				CurrMenu->items[CurrMenu->curitem].func(1234);
				flags |= 17;
			}
		}
		if(flags & 4 && !(pad.Buttons & PSP_CTRL_LEFT)) flags &= 0xFFFB;
		if(flags & 8 && !(pad.Buttons & PSP_CTRL_RIGHT)) flags &= 0xFFF7;
		if(flags & 16 && !(pad.Buttons & PSP_CTRL_SQUARE)) flags &= 0xFFEF;
		if(flags & 2 && !(pad.Buttons & (PSP_CTRL_CROSS|PSP_CTRL_CIRCLE))) flags &= 0xFFFD;
		if(flags & 1) {
			flags &= 0xFFFE;
			if(!CurrMenu) break;
			if(CurrMenu->draw) CurrMenu->draw();
			if(CurrMenu->draw != DrawPlugins)
			for(i = 0; i < CurrMenu->numitems; i++) {
				if(CurrMenu->curitem == i) continue;
				if(CurrMenu->items[i].text) kgt_write(CurrMenu->items[i].text, CurrMenu->items[i].x, 48 + i * 16, 0x404040, 0);
			}
			ColorWait(13);
		}
		ColorWait(1);
	}
	if(!save) return;
	if(sceIoUnassign("flash1:") < 0) {
		kgt_clear(0x0000FF);
		return;
	}
	if(sceIoAssign("flash1:", "lflash0:0,1", "flashfat1:", IOASSIGN_RDWR, NULL, 0) < 0) {
		kgt_clear(0x0000FF);
		return;
	}
	f = sceIoOpen("flash1:/kgCFW.cfg", PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
	if(f < 0) {
		kgt_clear(0x000080);
		return;
	}
	sceIoWrite(f, &config, sizeof(CFWconf));
	sceIoClose(f);
	sceKernelDelayThread(1000*1000);
}

void SelectMain(int way)
{
	switch(CurrMenu->curitem) {
		case 0:
			SetMenu(&CFWMenu);
		break;
		case 1:
			SetMenu(&RegistryMenu);
		break;
		case 2:
			SetMenu(&PluginsMenu);
		break;
		case 4:
			save = 1;
		default:
			SetMenu(NULL);
		break;
	}
}

void GoBack(int way)
{
	int i, f;
	char active[] = " ?\r\n";

	if(way == 1234) return;

	if(CurrMenu == &SelectMenu) {
		if(!flashplugins && numplugins && pluginschanged) {
			f = sceIoOpen(pluginfile, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0);
			if(f >= 0) {
				for(i = 0; i < numplugins; i++) {
					sceIoWrite(f, SelectItems[i].text, strlen(SelectItems[i].text));
					if(pluginactive[i]) active[1] = '1';
					else active[1] = '0';
					sceIoWrite(f, active, 4);
				}
				sceIoClose(f);
			}
			pluginfile = NULL;
		}
		flashplugins = 0;
	}
	SetMenu(CurrMenu->prev);
}

void DrawOptions()
{
	int i;

	kgt_write(txt_speeds[vshcpu], 250, 48, CurrMenu->curitem == 0 ? 0x00FFFF : 0x008080, 0);
	kgt_write(txt_speeds[gamecpu], 250, 64, CurrMenu->curitem == 1 ? 0x00FFFF : 0x008080, 0);
	kgt_write(regions[config.fakeregion], 250, 80, CurrMenu->curitem == 2 ? 0x00FFFF : 0x008080, 0);
	for(i = 0; i < 8; i++) {
		if(config.flags & (1 << i))
			kgt_write("enabled ", 250, 96 + i * 16, CurrMenu->curitem == i + 3 ? 0x00FF00 : 0x008000, 0);
		else
			kgt_write("disabled", 250, 96 + i * 16, CurrMenu->curitem == i + 3 ? 0x0000FF : 0x000080, 0);
	}
}

void ChangeOption(int way)
{
	switch(CurrMenu->curitem) {
		case 0:
			vshcpu += way;
			if(vshcpu > 8) vshcpu = 0;
			if(vshcpu < 0) vshcpu = 8;
			config.vshcpuspeed = cpu_speeds[vshcpu];
			config.vshbusspeed = bus_speeds[vshcpu];
		break;
		case 1:
			gamecpu += way;
			if(gamecpu > 8) gamecpu = 0;
			if(gamecpu < 0) gamecpu = 8;
			config.gamecpuspeed = cpu_speeds[gamecpu];
			config.gamebusspeed = bus_speeds[gamecpu];
		break;
		case 2:
			config.fakeregion += way;
			if(config.fakeregion > 13) config.fakeregion = 0;
			if(config.fakeregion < 0) config.fakeregion = 13;
		break;
		default:
			config.flags ^= 1 << (CurrMenu->curitem - 3);
		break;
	}
}

void EnableFlash(int way)
{
	int i;
	static int flashon = 0;

	if(flashon) {
		kgt_write("flash is active", 0, 256, 0x00FFFF, 0);
		return;
	}
	if(ReadRegistry("/CONFIG/BROWSER", "flash_activated", &i, 4)) {
		flashon = 1;
		if(!i) {
			i = 1;
			WriteRegistry("/CONFIG/BROWSER", "flash_activated", &i, 4);
			WriteRegistry("/CONFIG/BROWSER", "flash_play", &i, 4);
			kgt_write("flash activated", 0, 256, 0x00FF00, 0);
		} else {
			kgt_write("flash is active", 0, 256, 0x00FFFF, 0);
		}
	}
}

void EnableWMA(int way)
{
	int i;
	static int wmaon = 0;

	if(wmaon) {
		kgt_write("WMA is active  ", 0, 256, 0x00FFFF, 0);
		return;
	}
	if(ReadRegistry("/CONFIG/MUSIC", "wma_play", &i, 4)) {
		wmaon = 1;
		if(!i) {
			i = 1;
			WriteRegistry("/CONFIG/MUSIC", "wma_play", &i, 4);
			kgt_write("WMA activated  ", 0, 256, 0x00FF00, 0);
		} else {
			kgt_write("WMA is active  ", 0, 256, 0x00FFFF, 0);
		}
	}
}

void SwapButtons(int way)
{
	button_assign ^= 1;
	WriteRegistry("/CONFIG/SYSTEM/XMB", "button_assign", &button_assign, 4);
	if(button_assign) {
		CurrMenu->items[2].text[17] = 'O';
		kgt_write("X is now enter     ", 0, 256, 0x00FFFF, 0);
		PSP_CTRL_ENTER = PSP_CTRL_CROSS;
		PSP_CTRL_CANCEL = PSP_CTRL_CIRCLE;
	} else {
		CurrMenu->items[2].text[17] = 'X';
		kgt_write("O is now enter     ", 0, 256, 0x00FFFF, 0);
		PSP_CTRL_ENTER = PSP_CTRL_CIRCLE;
		PSP_CTRL_CANCEL = PSP_CTRL_CROSS;
	}
}

void ParsePlugins(char *path, char *menu, int nx)
{
	int i, f, pl, len, line;
	char *buff, *name, *end, *ptr;

	f = sceIoOpen(path, PSP_O_RDONLY, 0777);
	if(f < 0) {
		pluginfile = NULL;
		kgt_write("plugin file not found", 0, 256, 0x0000FF, 0);
		return;
	}
	pluginfile = path;
	SelectMenu.name = menu;
	SelectMenu.nx = nx;
	SelectMenu.draw = DrawPlugins;
	buff = (void*)0x08900000;
	len = sceIoRead(f, buff, 0x400); // same size as systemctrl
	sceIoClose(f);
	memset(&pluginactive, 0, sizeof(pluginactive));
	pl = 0;
	name = ptr = buff;
	end = buff + len;
	line = 0;
	while(ptr < end && pl < 9) {
		if(!line && (*ptr == ' ' || *ptr == '\t')) {
			*ptr = 0;
			SelectItems[pl].text = name;
			SelectItems[pl].x = 240 - (strlen(name) * 9) / 2;
			SelectItems[pl].func = TogglePlugin;
			pl++;
			while(ptr < end) {
				if(*ptr != ' ' && *ptr != '\t') break;
				ptr++;
			}
			ptr++;
			if(ptr >= end) break;
			if(*ptr == '\r' || *ptr == '\n') continue;
			if(*ptr == '1') pluginactive[pl-1] = 1;
			line = 1;
		}
		if(*ptr == '\r' || *ptr == '\n') {
			line = 0;
			name = ptr + 1;
		}
		ptr++;
	}
	numplugins = pl;
	SelectItems[pl].text = "go back";
	SelectItems[pl].x = 208;
	SelectItems[pl].func = GoBack;
	SelectMenu.numitems = pl + 1;
	SetMenu(&SelectMenu);
	pluginschanged = 0;
}

void SelectPluginMenu(int way)
{
	int i, pl;

	SelectMenu.curitem = 0;
	switch(CurrMenu->curitem) {
		case 0:
			flashplugins = 1;
			SelectMenu.name = "Flash plugins";
			SelectMenu.nx = 181;
			SelectMenu.draw = DrawFlash;
			pl = 0;
			for(i = 0; i < 8; i++) {
				if(config.plugins[i].name[0]) {
					SelectItems[pl].text = config.plugins[i].name;
					SelectItems[pl].x = 240 - (strlen(config.plugins[i].name) * 9) / 2;
					SelectItems[pl].func = FlashPlugin;
					flashnum[pl] = i;
					pl++;
				}
			}
			SelectItems[pl].text = "update list";
			SelectItems[pl].x = 190;
			SelectItems[pl].func = FlashUpdate;
			pl++;
			SelectItems[pl].text = "go back";
			SelectItems[pl].x = 208;
			SelectItems[pl].func = GoBack;
			SelectMenu.numitems = pl + 1;
			SetMenu(&SelectMenu);
		break;
		case 1:
			ParsePlugins("ms0:/seplugins/vsh.txt", "ms0 VSH plugins", 172);
		break;
		case 2:
			ParsePlugins("ms0:/seplugins/game.txt", "ms0 Game plugins", 168);
		break;
		case 3:
			ParsePlugins("ef0:/seplugins/vsh.txt", "ef0 VSH plugins", 172);
		break;
		case 4:
			ParsePlugins("ef0:/seplugins/game.txt", "ef0 Game plugins", 168);
		break;
	}
}

void DrawPlugins()
{
	int i, color;

	for(i = 0; i < CurrMenu->numitems - 1; i++) {
		if(pluginactive[i]) color = CurrMenu->curitem == i ? 0x00FF00 : 0x008000;
		else color = CurrMenu->curitem == i ? 0x0000FF : 0x000080;
		kgt_write(CurrMenu->items[i].text, CurrMenu->items[i].x, 48 + i * 16, color, 0);
	}
	if(CurrMenu->curitem != i)
		kgt_write(CurrMenu->items[i].text, CurrMenu->items[i].x, 48 + i * 16, 0x404040, 0);
}

void DrawFlash()
{
	int i;

	kgt_write("VSH", 120, 32, 0x808080, 0);
	kgt_write("Game", 324, 32, 0x808080, 0);
	for(i = 0; i < CurrMenu->numitems - 2; i++) {
		if(config.plugins[flashnum[i]].flags & PF_VSH)
			kgt_write("enabled ", 75, 48 + i * 16, CurrMenu->curitem == i ? 0x00FF00 : 0x008000, 0);
		else
			kgt_write("disabled", 75, 48 + i * 16, CurrMenu->curitem == i ? 0x0000FF : 0x000080, 0);
		if(config.plugins[flashnum[i]].flags & PF_GAME)
			kgt_write(" enabled", 333, 48 + i * 16, CurrMenu->curitem == i ? 0x00FF00 : 0x008000, 0);
		else
			kgt_write("disabled", 333, 48 + i * 16, CurrMenu->curitem == i ? 0x0000FF : 0x000080, 0);
	}
}

void TogglePlugin(int way)
{
	pluginschanged = 1;
	pluginactive[CurrMenu->curitem] ^= 1;
}

void FlashPlugin(int way)
{
	char name[48] = "flash0:/plugins/";

	if(way == 0) return;
	if(way == 1) way = PF_GAME;
	if(way == -1) way = PF_VSH;

	if(way == 1234) {
		memcpy(name + 16, SelectItems[CurrMenu->curitem].text, 32);
		if(sceIoRemove(name) >= 0) {
			memset(config.plugins[flashnum[CurrMenu->curitem]].name, 0, 32);
			config.plugins[flashnum[CurrMenu->curitem]].flags = 0;
			SelectPluginMenu(0); // flashplugins
		}
		return;
	}
	config.plugins[flashnum[CurrMenu->curitem]].flags ^= way;
}

void FlashUpdate(int way)
{
	int dir;
	SceIoDirent dirent;
	int pl, len;

	if(way == 1234) return;

	memset(&dirent,0,sizeof(SceIoDirent));
	memset(config.plugins, 0, sizeof(flash_plugin) * 8);

	dir = sceIoDopen("flash0:/plugins");
	if(dir >= 0) {
		pl = 0;
		while(sceIoDread(dir, &dirent) > 0) {
			if((FIO_S_IFREG & (dirent.d_stat.st_mode & FIO_S_IFMT)) != 0) {
				len = strlen(dirent.d_name);
				if(len < 32) {
					memcpy(config.plugins[pl].name, dirent.d_name, len);
					pl++;
					if(pl == 8) break;
				}
			}
		}
	}
	SelectPluginMenu(0); // flashplugins
}

int ReadRegistry(const char *dir, const char *name, void *val, int len)
{
	struct RegParam reg = {1, "/system", 7, 1, 1};
	REGHANDLE rh, rc;

	if(sceRegOpenRegistry(&reg, 1, &rh) < 0) return 0;
	if(sceRegOpenCategory(rh, dir, 1, &rc) < 0) {
		sceRegCloseRegistry(rh);
		return 0;
	}
	if(sceRegGetKeyValueByName(rc, name, val, len) < 0) {
		sceRegCloseCategory(rc);
		sceRegCloseRegistry(rh);
		return 0;
	}
	sceRegCloseCategory(rc);
	sceRegCloseRegistry(rh);
	return 1;
}

int WriteRegistry(const char *dir, const char *name, void *val, int len)
{
	struct RegParam reg = {1, "/system", 7, 1, 1};
	REGHANDLE rh, rc;

	if(sceRegOpenRegistry(&reg, 1, &rh) < 0) return 0;
	if(sceRegOpenCategory(rh, dir, 1, &rc) < 0) {
		sceRegCloseRegistry(rh);
		return 0;
	}
	if(sceRegSetKeyValue(rc, name, val, len) < 0) {
		sceRegCloseCategory(rc);
		sceRegCloseRegistry(rh);
		return 0;
	}
	sceRegFlushCategory(rc);
	sceRegCloseCategory(rc);
	sceRegFlushRegistry(rh);
	sceRegCloseRegistry(rh);
	return 1;
}

