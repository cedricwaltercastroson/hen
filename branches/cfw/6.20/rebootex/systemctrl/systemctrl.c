#include <string.h>

#include "pspsdk.h"
#include "pspkernel.h"
#include "psputility.h"
#include "psputilsforkernel.h"
#include "pspinit.h"
#include "pspctrl.h"
#include "psploadexec_kernel.h"
#include "pspmodulemgr_kernel.h"
#include "pspthreadman_kernel.h"
#include "pspsysmem_kernel.h"

#include "main.h"
#include "systemctrl_priv.h"
#include "malloc.h"

/* 0x000003F4 */
/* SystemCtrlForKernel_826668E9 */
void
PatchSyscall(u32 fp, void *func)
{
	ASM_FUNC_TAG();
	u32 sr;
	u32 *vectors;
	int i;

	__asm__ ("cfc0 $v0, $12;" : "=r"(sr));
	vectors = (u32 *) _lw(sr);
	for (i = 0; i < 0x4000; i++) {
		if (vectors[i] == fp)
			vectors[i] = (u32) func;
	}
}

/* 0x00000364 */
/* SystemCtrlForUser_1C90BECB SystemCtrlForKernel_1C90BECB */
void *
sctrlHENSetStartModuleHandler(void *handler)
{
	ASM_FUNC_TAG();
	void *prev = ModuleStartHandler;

	ModuleStartHandler = (void *) ((u32) handler | 0x80000000);

	return prev;
}

/* 0x000003D4 */
void *
SystemCtrlForKernel_AC0E84D1(void *func)
{
	ASM_FUNC_TAG();
	void *prev = DecryptExecutable_HEN;

	DecryptExecutable_HEN = func;

	return prev;
}

/* 0x000003E4 */
void *
SystemCtrlForKernel_1F3037FB(void *func)
{
	ASM_FUNC_TAG();
	void *prev = DecryptPrx_HEN;

	DecryptPrx_HEN = func;

	return prev;
}

/* 0x000030F4 */
/* SystemCtrlForUser_8E426F09 SystemCtrlForKernel_8E426F09 */
int
sctrlSEGetConfigEx(void *buf, SceSize size)
{
	ASM_FUNC_TAG();
	int ret = -1;
	int k1;
	SceUID fd;

	k1 = pspSdkSetK1(0);
	memset (buf, 0, size);
	if ((fd = sceIoOpen("flashfat1:/config.tn", PSP_O_RDONLY, 0)) > 0) {
		ret = sceIoRead(fd, buf, size);
		sceIoClose(fd);
	}
	pspSdkSetK1(k1);

    return ret;
}

/* 0x0000319C */
/* SystemCtrlForUser_16C3B7EE SystemCtrlForKernel_16C3B7EE */
int
sctrlSEGetConfig(void *buf)
{
	ASM_FUNC_TAG();
	return sctrlSEGetConfigEx(buf, sizeof(TNConfig));
}

/* 0x000031A4 */
/* SystemCtrlForUser_AD4D5EA5 SystemCtrlForKernel_AD4D5EA5 */
int
sctrlSESetConfigEx(TNConfig *config, int size)
{
	ASM_FUNC_TAG();
	int ret = 0;
	u32 k1;
	SceUID fd;

	k1 = pspSdkSetK1(0);
	if((fd = sceIoOpen("flashfat1:/config.tn", 0x602, 0777)) < 0) {
		pspSdkSetK1(k1);
		return -1;
	}

	config->magic = 0x47434E54;
	if (sceIoWrite(fd, config, size) < size)
		ret = -1;

	sceIoClose(fd);
	pspSdkSetK1(k1);

	return ret;
}

/* 0x0000325C */
/* SystemCtrlForUser_1DDDAD0C SystemCtrlForKernel_1DDDAD0C */
int
sctrlSESetConfig(TNConfig *config)
{
	ASM_FUNC_TAG();
	return sctrlSESetConfigEx(config, sizeof(TNConfig));
}


/* 0x000034A8 */
/* SystemCtrlForUser_D339E2E9 SystemCtrlForKernel_D339E2E9 */
int
sctrlHENIsSE(void)
{
	ASM_FUNC_TAG();
	return 1;
}

/* 0x000034B0 */
/* SystemCtrlForUser_2E2935EF SystemCtrlForKernel_2E2935EF */
int
sctrlHENIsDevhook(void)
{
	ASM_FUNC_TAG();
	return 0x0; /* not dummy routine */
}

/* 0x000034B8 */
/* SystemCtrlForUser_1090A2E1 SystemCtrlForKernel_1090A2E1 */
int
sctrlHENGetVersion(void)
{
	ASM_FUNC_TAG();
	return 0x00001000;
}

/* 0x000034C0 */
/* SystemCtrlForUser_B47C9D77 SystemCtrlForKernel_B47C9D77 */
int
sctrlSEGetVersion(void)
{
	ASM_FUNC_TAG();
	return 0x00020000;
}

/* 0x000034C8 */
/* SystemCtrlForUser_D8FF9B99 SystemCtrlForKernel_D8FF9B99 */
int
sctrlKernelSetDevkitVersion(int ver)
{
	ASM_FUNC_TAG();
	u32 k1;
	int ret;

	k1 = pspSdkSetK1(0);
	ret = sceKernelDevkitVersion();
	_sh(ver >> 16, 0x88011AAC);
	_sh(ver & 0xFFFF, 0x88011AB4);
	pspSdkSetK1(k1);

	return ret;
}

/* 0x0000353C */
/* SystemCtrlForUser_EB74FE45 SystemCtrlForKernel_EB74FE45 */
int
sctrlKernelSetUserLevel(int level)
{
	ASM_FUNC_TAG();
	int ret, k1;
	u32 text_addr, *thstruct;

	k1 = pspSdkSetK1(0);
	ret = sceKernelGetUserLevel();
	text_addr = find_text_addr_by_name("sceThreadManager");
	thstruct = (void *) _lw(text_addr + 0x00019E80);
	thstruct[5] = (level ^ 8) << 28;
	pspSdkSetK1(k1);

	return ret;
}

/* 0x000035B8 */
/* SystemCtrlForUser_2D10FB28 SystemCtrlForKernel_2D10FB28 */
int
sctrlKernelLoadExecVSHWithApitype(int apitype, const char *file, struct SceKernelLoadExecVSHParam *param)
{
	ASM_FUNC_TAG();
	int k1, ret;
	u32 text_addr;
	int (*_LoadExecVSH)(int, const char *, void *, int);

	k1 = pspSdkSetK1(0);
	text_addr = find_text_addr_by_name("sceLoadExec");
	if (g_model == 4)
		_LoadExecVSH = (void *) (text_addr + 0x2558);
	else
		_LoadExecVSH = (void *) (text_addr + 0x2304);

	ret = _LoadExecVSH(apitype, file, param, 0x00010000);
	pspSdkSetK1(k1);

	return ret;
}

/* 0x00003650 */
/* SystemCtrlForUser_78E46415 SystemCtrlForKernel_78E46415 */
PspIoDrv *
sctrlHENFindDriver(char* drvname)
{
	ASM_FUNC_TAG();
	u32 text_addr, k1, ret;
	u32 (*iomgr_find_driver)(char *);
	PspIoDrv *drv = NULL;

	k1 = pspSdkSetK1(0);
	text_addr = find_text_addr_by_name("sceIOFileManager");

	iomgr_find_driver = (void *) (text_addr + 0x2A38);
	if ((ret = iomgr_find_driver(drvname)))
		drv = (void *) _lw(ret + 4);
	pspSdkSetK1(k1);

	return drv;
}

/* 0x000036C4 */
/* SystemCtrlForUser_577AF198 SystemCtrlForKernel_577AF198 */
int
sctrlKernelLoadExecVSHDisc(const char *file, struct SceKernelLoadExecVSHParam *param)
{
	ASM_FUNC_TAG();
	int ret;
	int k1 = pspSdkSetK1(0);

	ret = sceKernelLoadExecVSHDisc(file, param);
	pspSdkSetK1(k1);

	return ret;
}

/* 0x00003720 */
/* SystemCtrlForUser_94FE5E4B SystemCtrlForKernel_94FE5E4B */
int
sctrlKernelLoadExecVSHDiscUpdater(const char *file, struct SceKernelLoadExecVSHParam *param)
{
	ASM_FUNC_TAG();
	int ret;
	int k1 = pspSdkSetK1(0);

	ret = sceKernelLoadExecVSHDiscUpdater(file, param);
	pspSdkSetK1(k1);

	return ret;
}

/* 0x0000377C */
/* SystemCtrlForUser_75643FCA SystemCtrlForKernel_75643FCA */
int
sctrlKernelLoadExecVSHMs1(const char *file, struct SceKernelLoadExecVSHParam *param)
{
	ASM_FUNC_TAG();
	int ret;
	int k1 = pspSdkSetK1(0);

	ret = sceKernelLoadExecVSHMs1(file, param);
	pspSdkSetK1(k1);

	return ret;
}

/* 0x000037D8 */
/* SystemCtrlForUser_ABA7F1B0 SystemCtrlForKernel_ABA7F1B0 */
int
sctrlKernelLoadExecVSHMs2(const char *file, struct SceKernelLoadExecVSHParam *param)
{
	ASM_FUNC_TAG();
	int ret;
	int k1 = pspSdkSetK1(0);

	ret = sceKernelLoadExecVSHMs2(file, param);
	pspSdkSetK1(k1);

	return ret;
}

/* 0x00003834 */
/* SystemCtrlForUser_7B369596 SystemCtrlForKernel_7B369596 */
int
sctrlKernelLoadExecVSHMs3(const char *file, struct SceKernelLoadExecVSHParam *param)
{
	ASM_FUNC_TAG();
	int ret;
	int k1 = pspSdkSetK1(0);

	ret = sceKernelLoadExecVSHMs3(file, param);
	pspSdkSetK1(k1);

	return ret;
}

/* 0x00003890 */
/* SystemCtrlForUser_D690750F SystemCtrlForKernel_D690750F */
int
sctrlKernelLoadExecVSHMs4(const char *file, struct SceKernelLoadExecVSHParam *param)
{
	ASM_FUNC_TAG();
	int ret;
	int k1 = pspSdkSetK1(0);

	ret = sceKernelLoadExecVSHMs4(file, param);
	pspSdkSetK1(k1);

	return ret;
}

/* 0x000038EC */
/* SystemCtrlForUser_2794CCF4 SystemCtrlForKernel_2794CCF4 */
int
sctrlKernelExitVSH(struct SceKernelLoadExecVSHParam *param)
{
	ASM_FUNC_TAG();
	int ret;
	int k1 = pspSdkSetK1(0);

	ret = sceKernelExitVSHVSH(param);
	pspSdkSetK1(k1);

	return ret;
}

/* 0x00000BC4
 * SystemCtrlForKernel_159AF5CC
 */
u32
sctrlHENFindFunction(char *module_name, char *lib_name, u32 nid)
{
	ASM_FUNC_TAG();
	SceModule2 *mod;
	nidtable_t *nidtbl;
	int i, ent_sz;
	void *ent_top;
	u32 tmp;

	if (!(mod = sceKernelFindModuleByName(module_name))) {
		if (!(mod = sceKernelFindModuleByAddress((u32) module_name)))
			return 0;
	}

	if ((nidtbl = FindLibNidTable(lib_name))) {
		if ((tmp = TranslateNid(nidtbl, nid)))
			nid = tmp;
	}

	ent_sz = mod->ent_size;
	ent_top = mod->ent_top;
	i = 0;

	while (i < ent_sz) {
		struct SceLibraryEntryTable *entry;
		int j, total, stubcnt, vstubcnt;
		unsigned int *entry_table;

		entry = (void *) (ent_top + i);

		if (entry->libname && !strcmp(entry->libname, lib_name)) {
			if (entry->stubcount > 0) {
				stubcnt = entry->stubcount;
				vstubcnt = entry->vstubcount;
				total = stubcnt + vstubcnt;
				entry_table = entry->entrytable;
				for (j = 0; j < stubcnt; j++) {
					if (entry_table[j] == nid)
						return entry_table[j + total];
				}
			}
		} else if (entry->vstubcount != 0) {
			stubcnt = entry->stubcount;
			vstubcnt = entry->vstubcount;
			total = stubcnt + vstubcnt;
			entry_table = entry->entrytable;
			entry_table += stubcnt;

			for (j = 0; j < vstubcnt; j++) {
				if (entry_table[j] == nid)
					return entry_table[j + total];
			}
		}

		i += entry->len * 4;
	}

	return 0;
}

/* 0x00001D50 */
/* SystemCtrlForKernel_CE0A654E */
void
sctrlHENLoadModuleOnReboot(char *module_after, void *buf, int size, int flags)
{
	ASM_FUNC_TAG();
	g_reboot_module = module_after;
	g_reboot_module_buf = buf;
	g_reboot_module_size = size;
	g_reboot_module_flags = flags;
}

/* 0x00001D74 */
void
SystemCtrlForKernel_B86E36D1(void)
{
	ASM_FUNC_TAG();
	int *(*func)(int);
	int *ret, *p;

	if (g_p2_size == 0)
		return;
	
	if (g_p2_size + g_p8_size >= 0x35)
		return;

	func = (void *) 0x88003E2C;

	ret = func(2);
	p = (int *) ret[4];
	ret[2] = g_p2_size << 20;
	p[5] = (g_p2_size << 21) | 0xFC;

	ret = func(8);
	p = (int *) ret[4];

	ret[1] = (g_p2_size << 20) + 0x88800000;
	ret[2] = g_p8_size << 20;
	p[5] = (g_p8_size << 21) | 0xFC;
}

/* 0x00002324 */
/* SystemCtrlForKernel_2F157BAF */
void
SetConfig(TNConfig *config)
{
	ASM_FUNC_TAG();
	memcpy(&g_tnconfig, config, sizeof(TNConfig));
}

/* 0x000023BC */
/* SystemCtrlForKernel_CC9ADCF8 */
int
sctrlHENSetSpeed(int cpuspd, int busspd)
{
	ASM_FUNC_TAG();
	g_scePowerSetClockFrequency = (void *) FindScePowerFunction(0x545A7F3C); /* scePowerSetClockFrequency */
	return g_scePowerSetClockFrequency(cpuspd, cpuspd, busspd);
}

/* 0x00002664 */
/* SystemCtrlForUser_745286D1 SystemCtrlForKernel_745286D1 */
int
sctrlHENSetMemory(int p2, int p8)
{
	ASM_FUNC_TAG();
	int k1;

	if (!p2 || ((p2 + p8) >= 0x35))
		return 0x80000107;

	k1 = pspSdkSetK1(0);
	g_p2_size = p2;
	g_p8_size = p8;
	pspSdkSetK1(k1);

	return 0;
}

/* 0x000027EC */
/* SystemCtrlForKernel_98012538 */
void
SetSpeed(int cpuspd, int busspd)
{
	ASM_FUNC_TAG();
	u32 fp;

	if (cpuspd == 0 || busspd == 0) {
		cpuspd = 222;
		busspd = 111;
	}

	switch (cpuspd) {
	case 0x14:
	case 0x4B:
	case 0x64:
	case 0x85:
	case 0xDE:
	case 0x10A:
	case 0x12C:
	case 0x14D:
		fp = FindScePowerFunction(0x737486F2);
		g_scePowerSetClockFrequency = (void *) fp;
		g_scePowerSetClockFrequency(cpuspd, cpuspd, busspd);

		if (sceKernelApplicationType() == PSP_INIT_KEYCONFIG_VSH)
			return;

		MAKE_DUMMY_FUNCTION0(fp);

		fp = FindScePowerFunction(0x545A7F3C);
		MAKE_DUMMY_FUNCTION0(fp);

		/* scePowerSetBusClockFrequency */
		fp = FindScePowerFunction(0xB8D7B3FB);
		MAKE_DUMMY_FUNCTION0(fp);

		/* scePowerSetCpuClockFrequency */
		fp = FindScePowerFunction(0x843FBF43);
		MAKE_DUMMY_FUNCTION0(fp);

		fp = FindScePowerFunction(0xEBD177D6);
		MAKE_DUMMY_FUNCTION0(fp);

		ClearCaches();
		break;
	
	default:
		break;
	}
}

/* 0x00003EA8 */
/* SystemCtrlForUser_CB76B778 SystemCtrlForKernel_CB76B778 */
int
sctrlKernelSetInitKeyConfig(int key)
{
	ASM_FUNC_TAG();
	int ret;
	int k1 = pspSdkSetK1(0);

	ret = sceKernelApplicationType();
	*g_keyconfig_addr = key;
	pspSdkSetK1(k1);

	return ret;
}

/* 0x00003F04 */
/* SystemCtrlForUser_128112C3 SystemCtrlForKernel_128112C3 */
int
sctrlKernelSetInitFileName(char *file)
{
	ASM_FUNC_TAG();
	int k1 = pspSdkSetK1(0);

	*g_init_filename_addr = file;
	pspSdkSetK1(k1);

	return 0;
}

/* 0x00003F44 */
/* SystemCtrlForUser_8D5BE1F0 SystemCtrlForKernel_8D5BE1F0 */
int
sctrlKernelSetInitApitype(int apitype)
{
	ASM_FUNC_TAG();
	int ret;
	int k1 = pspSdkSetK1(0);

	ret = sceKernelInitApitype();
	*g_apitype_addr = apitype;
	pspSdkSetK1(k1);

	return ret;
}

/* 0x00002620 */
/* VshCtrlLib_FD26DA72 */
int
vctrlVSHRegisterVshMenu(void *ctrl)
{
	ASM_FUNC_TAG();
	int k1 = pspSdkSetK1(0);

	VshMenuCtrl = (void *) ((u32) ctrl | 0x80000000);
	pspSdkSetK1(k1);

	return 0;
}

/* 0x00002C90 */
/* VshCtrlLib_CD6B3913 */
int
vctrlVSHExitVSHMenu(TNConfig *conf)
{
	ASM_FUNC_TAG();
	int k1 = pspSdkSetK1(0);
	int cpuspeed = g_tnconfig.vshcpuspeed;
	int busspeed = g_tnconfig.vshbusspeed;

	VshMenuCtrl = NULL;
	memcpy(&g_tnconfig, conf, sizeof(TNConfig));

	if (g_timestamp_2 == 0) {
		if (cpuspeed != g_tnconfig.vshcpuspeed) {
			SetSpeed(cpuspeed, busspeed);
			g_timestamp_1 = sceKernelGetSystemTimeLow();
		}
	}

	pspSdkSetK1(k1);

	return 0;
}

