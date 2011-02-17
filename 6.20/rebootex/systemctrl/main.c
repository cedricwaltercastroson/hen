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
#include "pspiofilemgr.h"

#include "main.h"
#include "systemctrl.h"
#include "malloc.h"

PSP_MODULE_INFO("SystemControl", 0x3007, 2, 5);
PSP_MAIN_THREAD_ATTR(0);

extern int sceKernelProbeExecutableObject_Patched(void *buf, int *check);
extern int sceKernelCheckExecFile_Patched(void *buf, int *check);
extern int sceKernelLinkLibraryEntries_Patched(void *buf, u32 size);
extern int PartitionCheck_Patched(void *buf, u32 *check);
extern SceUID sceKernelCreateThread_Patched(const char *name, SceKernelThreadEntry entry, int priority, int stacksize, SceUInt attr, SceKernelThreadOptParam *opt);
extern int sceKernelStartThread_Patched(SceUID tid, SceSize len, void *p);
extern int VerifySignCheck_Patched(void *hdr, int, int);
extern int sceIoMkDir_Patched(char *dir, SceMode mode);
extern int sceIoAssign_Patched(const char *, const char *, const char *, int, void *, long);
extern int DecryptExecutable_Patched(char *buf, int size, int *compressed_size, int polling);
extern int PatchSceKernelStartModule(int, int);
extern int DecryptPrx_Patched(int a0, int a1, int a2, char *buf, int size, int *compressed_size, int polling, int t3);
extern int sceKernelStartModule_Patched(int modid, SceSize argsize, void *argp, int *modstatus, SceKernelSMOption *opt);

extern void PatchUpdatePlugin(u32);
extern void PatchGamePlugin(u32);
extern void PatchMsvideoMainPlugin(u32);
extern void PatchSceWlanDriver(u32);
extern void PatchScePowerService(u32);
extern void PatchVsh(u32);
extern void PatchSceMediaSync(u32);
extern void PatchSceUmdCacheDriver(u32);
extern void PatchSceImposeDriver(void);
extern void PatchSysconfPlugin(u32);

extern int sceCtrlReadBufferPositive_Patched(SceCtrlData *, int);
extern SceUID PatchSceUpdateDL(const char *, int, SceKernelLMOption *);

extern int LoadExecBootStart_Patched(int a0, int a1, int a2, int a3);

static unsigned int size_satelite_bin;
static unsigned char satelite_bin[];

#include "satelite_bin.inc"

/* 0x000083E8 */
TNConfig g_tnconfig = { 0 };

int g_model = 0; /* 0x00008270 */
void (*ModuleStartHandler) (void *) = NULL; /* 0x000083A4 */
int g_p2_size = 0; /* 0x00008258 */
int g_p8_size = 0; /* 0x0000825C */
unsigned int g_rebootex_size = 0; /* 0x00008264 */
void *g_rebootex_buf = NULL; /* 0x0000828C */

int *g_apitype_addr = NULL; /* 0x00008288 */
char **g_init_filename_addr = NULL; /* 0x00008284 */
int *g_keyconfig_addr = NULL; /* 0x0000839C */

int (*DecryptExecutable)(void *, unsigned int, void *, unsigned int) = NULL;  /* 0x0000827C */
void (*sceMemlmdInitializeScrambleKey)(void *, void *) = NULL; /* 0x00008280 */

int (*DecryptExecutable_HEN)(char *buf, int size, int *compressed_size, int polling) = NULL; /* 0x00008260 */
int (*DecryptPrx_HEN) (int a0, int a1, int a2, char *buf, int size, int *compressed_size, int polling, int t3) = NULL; /* 0x00008290 */
int (*DecryptPrx) (int a0, int a1, int a2, char *buf, int size, int *compressed_size, int polling, int t3) = NULL; /* 0x0000826C */

char *g_reboot_module = NULL; /* 0x000083B8 */
void *g_reboot_module_buf = NULL; /* 0x000083C8 */
int g_reboot_module_size = 0; /* 0x000083D0 */
int g_reboot_module_flags = 0; /* 0x000083CC */

int (*VshMenuCtrl) (SceCtrlData *, int) = NULL; /* 0x000083B0 */

/* 4 timestamps */
unsigned int g_timestamp_1 = 0; /* 0x000083D4 */
unsigned int g_timestamp_2 = 0; /* 0x000083DC */
unsigned int g_timestamp_3 = 0; /* 0x000083E0 */
unsigned int g_timestamp_4 = 0; /* 0x000083E4 */

SceUID g_SceModmgrStart_tid = 0; /* 0x00008298 */
SceModule2 *g_SceModmgrStart_module = NULL; /* 0x00008274 */

int (*sceDisplaySetHoldMode) (int) = NULL; /* 0x000083D8 */

int (*g_scePowerSetClockFrequency) (int, int, int) = NULL; /* 0x000083AC */
int (*g_scePowerGetCpuClockFrequency) (void) = NULL; /* 0x000083B4 */
int (*g_sceCtrlReadBufferPositive) (SceCtrlData *, int) = NULL; /* 0x000083C4 */

int (*VerifySignCheck)(void *, int, int) = NULL; /* 0x00008268 */
int (*LoadExecBootstart) (int, int, int, int) = NULL; /* 0x000083BC */

int (*ProbeExec1) (void *, u32 *) = NULL; /* 0x00008278 */
int (*ProbeExec2) (void *, u32 *) = NULL; /* 0x000083A0 */
int (*PartitionCheck) (void *, void *) = NULL; /* 0x00008294 */


/* 0x00000000 */
int
IsStaticElf(void *buf)
{
	ASM_FUNC_TAG();
	Elf32_Ehdr *hdr = buf;

	return ((hdr->e_magic == ELF_MAGIC) && (hdr->e_type) == 2);
}

/* 0x00000038 */
int
PatchExec2(void *buf, int *check)
{
	ASM_FUNC_TAG();
	int index = check[19];
	u32 addr;

	if (index < 0)
		index += 3;

	addr = (u32) (buf + index);
	if (addr + 0x77C00000 < 0x00400001) /* same as addr < 0x88800001U */
		return 0;

	check[0x58/4] = ((u32 *)buf)[index / 4] & 0xFFFF;
	return ((u32 *)buf)[index / 4];
}

/* 0x00000090 */
int
PatchExec1(void *buf, int *check)
{
	ASM_FUNC_TAG();
	int i;

	if (_lw((u32) buf) != ELF_MAGIC)
		return -1;

	i = check[2];
	if (i >= 0x120) {
		if (i != 0x120 && i != 0x141 && i != 0x142 && i != 0x143 && i != 0x140)
			return -1;

		if (check[4] == 0) {
			if (check[17] == 0)
				return -1;
			check[18] = 1;
			return 0;
		}

		check[18] = 1;
		check[17] = 1;
		PatchExec2(buf, check);

		return 0;
	} else if ((u32) i >= 0x52)
		return -1;


	if (check[17] == 0)
		return -2;

	check[18] = 1;

	return 0;
}


/* 0x00000150 */
int
ProbeExec1_Patched(void *buf, u32 *check)
{
	ASM_FUNC_TAG();
	int ret;
	u16 attr;
	u16 *modinfo;
	u16 realattr;

	ret = ProbeExec1(buf, check);

	if (((u32 *)buf)[0] != ELF_MAGIC)
		return ret;

	modinfo = ((u16 *)buf) + (check[0x4C/4] / 2);

	realattr = *modinfo;
	attr = realattr & 0x1E00;

	if (attr != 0) {
		u16 attr2 = ((u16 *)check)[0x58/2];
		attr2 &= 0x1E00;

		if (attr2 != attr)
			((u16 *)check)[0x58/2] = realattr;
	}

	if (check[0x48/4] == 0)
		check[0x48/4] = 1;

	return ret;
}

/* 0x000001E4 */
char * __attribute__ ((noinline))
GetStrTab(void *buf)
{
	ASM_FUNC_TAG();
	Elf32_Ehdr *hdr = (Elf32_Ehdr *) buf;
	int i;
	u8 *p;

	if (hdr->e_magic != ELF_MAGIC)
		return NULL;

	p = buf + hdr->e_shoff;
	for (i = 0; i < hdr->e_shnum; i++) {
		if (hdr->e_shstrndx == i) {
			Elf32_Shdr *section = (Elf32_Shdr *) p;

			if (section->sh_type == 3)
				return (char *) buf + section->sh_offset;
		}
		p += hdr->e_shentsize;
	}

	return NULL;
}

/* 0x00000280 */
int
PatchExec3(void *buf, int *check, int is_plain, int res)
{
	ASM_FUNC_TAG();
	if (!is_plain)
		return res;

	if ((u32) check[2] >= 0x52) {
		if (IsStaticElf(buf)) {
			check[8] = 3;
		}
		return res;
	}

	if (!(PatchExec2(buf, check) & 0xFF00))
		return res;

	check[17] = 1;

	return 0;
}

/* 0x0000031C
 * translate old nid to new nid
 */
u32
TranslateNid(nidtable_t *t, u32 nid)
{
	ASM_FUNC_TAG();
	int i, cnt;
	nidentry_t *nids;

	cnt = t->cnt;
	nids = t->nids;

	for (i = 0; i < cnt; i++) {
		if (nids[i].oldnid == nid)
			return nids[i].newnid;
	}

	return 0;
}

/* 0x0000037C */
int
VerifySignCheck_Patched(void *buf, int size, int polling)
{
	ASM_FUNC_TAG();
	int i;
	PSP_Header *hdr = buf;

	if (hdr->signature != 0x5053507E) /* ~PSP */
		return 0;

	for (i = 0; i < 0x58; i++) {
		if (hdr->scheck[i] != 0 &&
				hdr->reserved2[0] != 0 &&
				hdr->reserved2[1] != 0)
			return VerifySignCheck(hdr, size, polling);
	}

	return 0;
}

/* 0x00004EC0 */
#include "nidtables.inc"

/* 0x00000428 */
nidtable_t *
FindLibNidTable(const char *name)
{
	ASM_FUNC_TAG();
	/* 0x00006888 */
	static nidtable_t nidtables[] = {
		NID_TABLE(SysMemForKernel),
		NID_TABLE(KDebugForKernel),
		NID_TABLE(LoadCoreForKernel),
		NID_TABLE(ExceptionManagerForKernel),
		NID_TABLE(InterruptManagerForKernel),
		NID_TABLE(IoFileMgrForKernel),
		NID_TABLE(ModuleMgrForKernel),
		NID_TABLE(LoadExecForKernel),
		NID_TABLE(sceDdr_driver),
		NID_TABLE(sceDmacplus_driver),
		NID_TABLE(sceGpio_driver),
		NID_TABLE(sceSysreg_driver),
		NID_TABLE(sceSyscon_driver),
		NID_TABLE(sceDisplay_driver),
		NID_TABLE(sceDve_driver),
		NID_TABLE(sceGe_driver),
		NID_TABLE(sceCtrl_driver),
		NID_TABLE(sceUmd),
		NID_TABLE(sceHprm_driver),
		NID_TABLE(scePower_driver),
		NID_TABLE(sceImpose_driver),
		NID_TABLE(sceRtc_driver),
		NID_TABLE(sceReg_driver),
		NID_TABLE(memlmd),
		NID_TABLE(sceMesgLed_driver),
		NID_TABLE(sceClockgen_driver),
		NID_TABLE(sceCodec_driver),
	};

	int i;

	if (name == NULL)
		return NULL;

	for (i = 0; i < sizeof(nidtables) / sizeof(nidtable_t); i++) {
		if (!strcmp(name, nidtables[i].name))
			return &nidtables[i];
	}

	return NULL;
}

/* 0x000004B4 */
int
ProbeExec2_Patched(char *buf, u32 *check)
{
	ASM_FUNC_TAG();
	Elf32_Ehdr *hdr;
	int ret;

	ret = ProbeExec2(buf, check);
	if (*(u32 *) buf != ELF_MAGIC)
		return ret;

	hdr = (Elf32_Ehdr *) buf;
	if (hdr->e_type == 2 && (check[2] - 0x140 < 5))
		check[2] = 0x120;

	if (check[19] == 0 && IsStaticElf(buf)) {
		char *stab, *p;
		int i;

		if ((stab = GetStrTab(buf))) {
			p = buf + hdr->e_shoff;
			for (i = 0; i < hdr->e_shnum; i++) {
				Elf32_Shdr *sec = (Elf32_Shdr *) p;

				if (!strcmp(stab + sec->sh_name, ".rodata.sceModuleInfo")) {
					check[19] = sec->sh_offset;
					check[22] = 0;
				}

				p += hdr->e_shentsize;
			}
		}
	}

	return ret;
}

/* 0x00000648 */
void
PatchModuleMgr(void)
{
	ASM_FUNC_TAG();
	u32 text_addr;
	u32 fp;

	text_addr = find_text_addr_by_name("sceModuleManager");

	if (g_model == 4)
		_sw(MAKE_CALL(sceKernelProbeExecutableObject_Patched), text_addr + 0x00007C3C);

	_sw(MAKE_JMP(sceKernelCheckExecFile_Patched), text_addr + 0x00008854); /* mask sceKernelCheckExecFile */

	PartitionCheck = (void *) (text_addr + 0x00007FC0);
	g_apitype_addr = (void *) (text_addr + 0x00009990);
	g_init_filename_addr = (void *) (text_addr + 0x00009994);
	g_keyconfig_addr = (void *) (text_addr + 0x000099EC);

	_sw(0, text_addr + 0x00000760); /* sceIoIoctl in sceKernelLoadModule */
	_sw(0x24020000, text_addr + 0x000007C0); /* addiu v0, $zr, 0	*/
	_sw(0, text_addr + 0x000030B0); /* somewhere in sceKernelLoadModuleVSH */
	_sw(0, text_addr + 0x0000310C); /* somewhere in sceKernelLoadModuleVSH */
	_sw(0x10000009, text_addr + 0x00003138); /* somewhere in sceKernelLoadModuleVSH */
	_sw(0, text_addr + 0x00003444); /* somewhere in sceKernelLoadModule */
	_sw(0, text_addr + 0x0000349C); /* somewhere in sceKernelLoadModule */
	_sw(0x10000010, text_addr + 0x000034C8); /* somewhere in sceKernelLoadModule */

	fp = MAKE_CALL(PartitionCheck_Patched);
	_sw(fp, text_addr + 0x000064FC);
	_sw(fp, text_addr + 0x00006878);

	_sw(MAKE_CALL(sceKernelLinkLibraryEntries_Patched), text_addr + 0x0000842C); /* mask sceKernelLinkLibraryEntries */
	_sw(0, text_addr + 0x00004360);
	_sw(0, text_addr + 0x000043A8);
	_sw(0, text_addr + 0x000043C0);

	_sw(MAKE_JMP(sceKernelCreateThread_Patched), text_addr + 0x0000894C);
	_sw(MAKE_JMP(sceKernelStartThread_Patched), text_addr + 0x00008994);
}

/* 0x000007DC */
void
PatchInterruptMgr(void)
{
	ASM_FUNC_TAG();
	u32 text_addr;

	text_addr = find_text_addr_by_name("sceInterruptManager");
	_sw(0, text_addr + 0x00000E94);
	_sw(0, text_addr + 0x00000E98);
	_sw(0, text_addr + 0x00000DE8);
	_sw(0, text_addr + 0x00000DEC);
}

/* 0x00000814 */
void
PatchIoFileMgr(void)
{
	ASM_FUNC_TAG();
	u32 text_addr;

	text_addr = find_text_addr_by_name("sceIOFileManager");

	PatchSyscall(text_addr + 0x00001AAC, sceIoAssign_Patched);

	if (sceKernelApplicationType() == PSP_INIT_KEYCONFIG_VSH)
		PatchSyscall(text_addr + 0x00004260, sceIoMkDir_Patched);
}

/* 0x00000878 */
void
PatchMemlmd(void)
{
	ASM_FUNC_TAG();
	/* 0x00006A0C */
	static u32 model0[] = {
		0x00000F10, 0x00001158, 0x000010D8, 0x0000112C,
		0x00000E10, 0x00000E74
	};

	/* 0x00006A24 */
	static u32 model1[] = {
		0x00000FA8, 0x000011F0, 0x00001170, 0x000011C4,
		0x00000EA8, 0x00000F0C
	};

	u32 text_addr;
	u32 fp;
	u32 *table;

	text_addr = find_text_addr_by_name("sceMemlmd");

	if (g_model == 0)
		table = model0;
	else
		table = model1;

	fp = MAKE_CALL(VerifySignCheck_Patched);
	_sw(fp, text_addr + table[2]);
	_sw(fp, text_addr + table[3]);

	fp = MAKE_CALL(DecryptExecutable_Patched);
	_sw(fp, text_addr + table[4]);

	VerifySignCheck = (void *) (text_addr + table[0]);
	DecryptExecutable = (void *) (text_addr + 0x00000134);
	sceMemlmdInitializeScrambleKey = (void *) (text_addr + table[1]);

	_sw(fp, text_addr + table[5]);
}

/* 0x0000094C */
void
PatchSceMesgLed(void)
{
	ASM_FUNC_TAG();
	/* 0x000069CC */
	static u32 model0[] = {0x00001E3C, 0x00003808, 0x00003BC4, 0x00001ECC};
	/* 0x000069DC */
	static u32 model1[] = {0x00001ECC, 0x00003D10, 0x0000415C, 0x00001F5C};
	/* 0x000069EC */
	static u32 model23[] = {0x00001F5C, 0x000041F0, 0x00004684, 0x00001FEC}; /* XXX TN bug? 41F4 or 41F0 ? */
	/* 0x000069FC */
	static u32 model4[] = {0x00001FEC, 0x00004674, 0x00004B50, 0x0000207C};

	u32 text_addr, fp;
	u32 *p;

	text_addr = find_text_addr_by_name("sceMesgLed");
	switch (g_model) {
	case 0:
		p = model0;
		break;
	case 1:
		p = model1;
		break;
	case 2:
	case 3: /* fall-thru. fix for 4G */
		p = model23;
		break;
	case 4:
		p = model4;
		break;
	default: /* missing in TN HEN */
		return;
	}

	fp = MAKE_CALL(DecryptPrx_Patched);
	_sw(fp, text_addr + 0x00001908);
	_sw(fp, text_addr + p[0]);
	DecryptPrx = (void *) (text_addr + 0xE0);
	_sw(fp, text_addr + p[1]);
	_sw(fp, text_addr + p[2]);
	_sw(fp, text_addr + p[3]);
}

/* 0x00000A28 */
int
sceIoMkDir_Patched(char *dir, SceMode mode)
{
	ASM_FUNC_TAG();
	int k1 = pspSdkSetK1(0);

	if (!strcmp(dir, "ms0:/PSP/GAME")) {
		sceIoMkdir("ms0:/seplugins", mode);
	} else if (!strcmp(dir, "ef0:/PSP/GAME")) {
		sceIoMkdir("ef0:/seplugins", mode);
	}
	pspSdkSetK1(k1);

	return sceIoMkdir(dir, mode);
}

/* 0x00000ABC */
int
sceIoAssign_Patched(const char *dev1, const char *dev2, const char *dev3,
		int mode, void *unk1, long unk2)
{
	ASM_FUNC_TAG();
	int k1 = pspSdkSetK1(0);

	if (mode == IOASSIGN_RDWR) {
		if (!strcmp(dev3, "flashfat0:") &&
				g_tnconfig.protectflash != 0) {
			pspSdkSetK1(k1);
			return -1;
		}
	}
	pspSdkSetK1(k1);

	return sceIoAssign(dev1, dev2, dev3, mode, unk1, unk2);
}

/* 0x00000BA8 */
void
ClearCaches(void)
{
	ASM_FUNC_TAG();
	sceKernelIcacheInvalidateAll();
	sceKernelDcacheWritebackInvalidateAll();
}

/* 0x00000D48 */
void
PatchVLF(u32 nid)
{
	ASM_FUNC_TAG();
	u32 fp = sctrlHENFindFunction("VLF_Module", "VlfGui", nid);

	if (fp) {
		MAKE_DUMMY_FUNCTION0(fp);
	}
}

/* 0x00000D90 */
void
PatchModules(SceModule2 *mod)
{
	ASM_FUNC_TAG();
	static int g_00008244 = 0;

	u32 text_addr;

#define __panic() do { *(int *) 0 = 0; } while (1)

	text_addr = mod->text_addr;

	if (!strcmp(mod->modname, "sceLowIO_Driver")) {
		if (mallocinit() < 0) {
			__panic();
		}
	} else if (!strcmp(mod->modname, "sceUmdCache_driver")) {
		PatchSceUmdCacheDriver(text_addr);
	} else if (!strcmp(mod->modname, "sceMediaSync")) {
		PatchSceMediaSync(text_addr);
	} else if (!strcmp(mod->modname, "sceImpose_Driver")) {
		PatchSceImposeDriver();
	} else if (!strcmp(mod->modname, "sceWlan_Driver")) {
		PatchSceWlanDriver(text_addr);
	} else if (!strcmp(mod->modname, "scePower_Service")) {
		PatchScePowerService(text_addr);
	} else if (!strcmp(mod->modname, "vsh_module")) {
		PatchVsh(text_addr);
	} else if (!strcmp(mod->modname, "sysconf_plugin_module")) {
		PatchSysconfPlugin(text_addr);
	} else if (!strcmp(mod->modname, "msvideo_main_plugin_module")) {
		PatchMsvideoMainPlugin(text_addr);
	} else if (!strcmp(mod->modname, "game_plugin_module")) {
		PatchGamePlugin(text_addr);
	} else if (!g_tnconfig.notnupdate && 
			!strcmp(mod->modname, "update_plugin_module")) {
		PatchUpdatePlugin(text_addr);
	} else if (!strcmp(mod->modname, "VLF_Module")) {
		PatchVLF(0x2A245FE6);
		PatchVLF(0x7B08EAAB);
		PatchVLF(0x22050FC0);
		PatchVLF(0x158BE61A);
		PatchVLF(0xD495179F);
		ClearCaches();
	}

	if (g_00008244 == 0) {
		if (sceKernelGetSystemStatus() != 0x00020000)
			return;
		if (sceKernelApplicationType() == PSP_INIT_KEYCONFIG_GAME)
			SetSpeed(g_tnconfig.umdisocpuspeed, g_tnconfig.umdisobusspeed);
		g_00008244 = 1;
	}
}

/* 0x0000101C */
void
PatchLoadCore(void)
{
	ASM_FUNC_TAG();
	u32 text_addr;
	u32 fp;

	text_addr = find_text_addr_by_name("sceLoaderCore");

	_sw((u32) sceKernelCheckExecFile_Patched, text_addr + 0x000086B4);
	fp = MAKE_CALL(sceKernelCheckExecFile_Patched);
	_sw(fp, text_addr + 0x00001578);
	_sw(fp, text_addr + 0x000015C8);
	_sw(fp, text_addr + 0x00004A18);

	_sw(_lw(text_addr + 0x00008B58), text_addr + 0x00008B74);
	_sw(MAKE_CALL(ProbeExec1_Patched), text_addr + 0x000046A4);
	_sw(MAKE_CALL(ProbeExec2_Patched), text_addr + 0x00004878);
	_sw(0x3C090000, text_addr + 0x000040A4);
	_sw(0, text_addr + 0x00007E84);
	_sw(0, text_addr + 0x00006880);
	_sw(0, text_addr + 0x00006884);
	_sw(0, text_addr + 0x00006990);
	_sw(0, text_addr + 0x00006994);
	_sw(MAKE_CALL(PatchSceKernelStartModule), text_addr + 0x00001DB0);
	_sw(0x02E02021, text_addr + 0x00001DB4);

	ProbeExec1 = (void *) (text_addr + 0x000061D4);
	ProbeExec2 = (void *) (text_addr + 0x000060F0);

	fp = sctrlHENFindFunction("sceMemlmd", "memlmd", 0x2E208358);
	fp = MAKE_CALL(fp);

	_sw(fp, text_addr + 0x00006914);
	_sw(fp, text_addr + 0x00006944);
	_sw(fp, text_addr + 0x000069DC);

	fp = sctrlHENFindFunction("sceMemlmd", "memlmd", 0xCA560AA6);
	fp = MAKE_CALL(fp);

	_sw(fp, text_addr + 0x000041A4);
	_sw(fp, text_addr + 0x000068F0);
}

void
PatchThreadManager(void)
{
	SceModule2 *mod = sceKernelFindModuleByName("sceThreadManager");

	if (mod)
		_sw(0, mod->text_addr + 0x000175AC);
}

/* 0x0000119C */
int
module_bootstart(void)
{
	ASM_FUNC_TAG();
	SceUID partition_id;

	g_model = _lw(0x88FB0000);

	PatchLoadCore();
	PatchModuleMgr();
	PatchMemlmd();
	PatchIoFileMgr();
	PatchInterruptMgr();
	PatchThreadManager();

	ClearCaches();

	ModuleStartHandler = (void *) (0x80000000 | ((u32) PatchModules));
	g_p2_size = _lw(0x88FB0008);
	g_p8_size = _lw(0x88FB000C);
	g_rebootex_size = _lw(0x88FB0004); /* from launcher: uncompressed rebootex size */

	partition_id = sceKernelAllocPartitionMemory(1, "", 1, g_rebootex_size, 0);
	if (partition_id >= 0) {
		g_rebootex_buf = sceKernelGetBlockHeadAddr(partition_id);
		memset(g_rebootex_buf, 0, g_rebootex_size);
		memcpy(g_rebootex_buf, (void *) 0x88FC0000, g_rebootex_size);
	}

	ClearCaches();

	return 0;
}

/* 0x000012A0 */
int
sceKernelLinkLibraryEntries_Patched(void *buf, u32 size)
{
	ASM_FUNC_TAG();
	u32 ver, nid, offs;
	u32 *pnid;
	nidtable_t *nidtbl;
	const char *lib_name;
	struct SceLibraryStubTable *stub;
	int i, stubcount, ret;
	struct SceLibraryStubTable *clib, *syscon, *power;

	/* module_sdk_version */
	ver = sctrlHENFindFunction(buf, NULL, 0x11B97506);
	if (ver) {
		if (_lw(ver) == 0x06020010)
			return sceKernelLinkLibraryEntries(buf, size);
	}

	offs = 0;
	clib = syscon = power = NULL;

	while (offs < size) {
		stub = buf + offs;
		lib_name = stub->libname;
		nidtbl = FindLibNidTable(lib_name);

		if (!strcmp(lib_name, "SysclibForKernel")) {
			clib = stub;
		} else if (!strcmp(lib_name, "sceSyscon_driver")) {
			syscon = stub;
		} else if (!strcmp(lib_name, "scePower_driver")) {
			power = stub;
		}

		if (nidtbl != NULL) {
			stubcount = stub->stubcount;
			for (i = 0; i < stubcount; i++) {
				pnid = &stub->nidtable[i];
				if ((nid = TranslateNid(nidtbl, *pnid)))
					*pnid = nid;
			}
		}
		
		offs += stub->len << 2;
	}

	ret = sceKernelLinkLibraryEntries(buf, size);

	if (clib) {
		stubcount = clib->stubcount;
		for (i = 0; i < stubcount; i++) {
			nid = clib->nidtable[i];

			if (nid == 0x909C228B || nid == 0x18FE80DB) { /* setjmp and longjmp */
				u32 addr = (u32) clib->stubtable + (i * 8);

				if (nid == 0x909C228B)
					_sw(0x0A000BA0, addr);
				else
					_sw(0x0A000BAF, addr);

				_sw(0, addr + 4);
				ClearCaches();
			}
		}
	}

	if (syscon) {
		stubcount = syscon->stubcount;
		for (i = 0; i < stubcount; i++) {
			if (syscon->nidtable[i] == 0xC8439C57) { /* sceSysconPowerStandby */
				u32 addr = (u32) syscon->stubtable + (i * 8);
				u32 func = find_text_addr_by_name("sceSYSCON_Driver") + 0x2C64;

				REDIRECT_FUNCTION(addr, func);
				ClearCaches();
			}
		}
	}

	if (power) {
		stubcount = power->stubcount;
		for (i = 0; i < stubcount; i++) {
			if (power->nidtable[i] == 0x737486F2) { /* scePowerSetClockFrequency */
				u32 func = FindScePowerFunction(0x737486F2);

				if (func) {
					u32 addr = (u32) power->stubtable + (i * 8);

					REDIRECT_FUNCTION(addr, func);
					ClearCaches();
				}
			}
		}
	}

	return ret;
}

/* 0x000015E8 */
SceUID
sceKernelCreateThread_Patched(const char *name, SceKernelThreadEntry entry, int priority, int stacksize, SceUInt attr, SceKernelThreadOptParam *opt)
{
	ASM_FUNC_TAG();
	SceUID tid;

	tid = sceKernelCreateThread(name, entry, priority, stacksize, attr, opt);
	if (tid < 0)
		return tid;

	if (!strcmp(name, "SceModmgrStart")) {
		g_SceModmgrStart_tid = tid;
		g_SceModmgrStart_module = sceKernelFindModuleByAddress((u32) entry);
	}

	return tid;
}

/* 0x0000165C */
int
sceKernelStartThread_Patched(SceUID tid, SceSize len, void *p)
{
	ASM_FUNC_TAG();
	if (tid == g_SceModmgrStart_tid) {
		g_SceModmgrStart_tid = -1;
		if (ModuleStartHandler && g_SceModmgrStart_module) {
			ModuleStartHandler(g_SceModmgrStart_module);
		}
	}

	return sceKernelStartThread(tid, len, p);
}

#define is_gzip(__buf) (_lh((u32) (__buf)) == 0x8B1F)

/* 0x000016D8 */
int
DecryptExecutable_Patched(char *buf, int size, int *compressed_size, int polling)
{
	ASM_FUNC_TAG();
	int r;
	PSP_Header *hdr = (PSP_Header *) buf;

	if (DecryptExecutable_HEN) {
		if (DecryptExecutable_HEN(buf, size, compressed_size, polling) >= 0)
			return 0;
	}

	if (buf && compressed_size) {
		if (hdr->oe_tag == 0xC6BA41D3 || hdr->oe_tag == 0x55668D96) { /* M33 */
			if (is_gzip(&buf[0x150])) {
				memmove(buf, buf + 0x150, hdr->comp_size);
				*compressed_size = hdr->comp_size;
				return 0;
			}
		}
	}

	r = DecryptExecutable(buf, size, compressed_size, polling);
	if (r >= 0)
		return r;

	if (VerifySignCheck_Patched(buf, size, polling) < 0)
		return r;

	sceMemlmdInitializeScrambleKey(NULL, (void *) 0xBFC00200);
	return DecryptExecutable(buf, size, compressed_size, polling);
}

/* 0x00001838 */
int
DecryptPrx_Patched(int a0, int a1, int a2, char *buf, int size, int *compressed_size, int polling, int t3)
{
	ASM_FUNC_TAG();
	int r;
	PSP_Header *hdr = (PSP_Header*) buf;

	if (DecryptPrx_HEN) {
		if (DecryptPrx_HEN(a0, a1, a2, buf, size, compressed_size, polling, t3) >= 0)
			return 0;
	}

	if (a0 != 0 && buf && compressed_size) {
		if (hdr->oe_tag == 0x28796DAA || hdr->oe_tag == 0x7316308C
				|| hdr->oe_tag == 0x3EAD0AEE || hdr->oe_tag ==0x8555ABF2) {
			if (is_gzip(&buf[0x150])) {
				memmove(buf, buf + 0x150, hdr->comp_size);
				*compressed_size = hdr->comp_size;
				return 0;
			}
		}
	}

	r = DecryptPrx(a0, a1, a2, buf, size, compressed_size, polling, t3);
	if (r >= 0)
		return r;
	if (VerifySignCheck_Patched(buf, size, polling) < 0)
		return r;

	return DecryptPrx(a0, a1, a2, buf, size, compressed_size, polling, t3);
}

/* 0x00001A34 */
int
sceKernelCheckExecFile_Patched(void *buf, int *check)
{
	ASM_FUNC_TAG();
	int ret = PatchExec1(buf, check);
	int is_plain;

	if (ret == 0)
		return ret;

	is_plain = (((u32 *)buf)[0] == 0x464C457F);
	ret = sceKernelCheckExecFile(buf, check);
	return PatchExec3(buf, check, is_plain, ret);
}

/* 0x00001AB8 */
int
PartitionCheck_Patched(void *buf, u32 *check)
{
	ASM_FUNC_TAG();
	static u32 readbuf[64]; /* 0x0000829C */

	u16 attr;
	SceUID fd;
	u32 pos;

	fd = _lw((u32) buf + 0x18);
	pos = sceIoLseek(fd, 0, 1);
	sceIoLseek(fd, 0, 0);
	if (sceIoRead(fd, readbuf, 0x100) < 0x100)
		goto out;

	if (readbuf[0] == 0x50425000) { /* PBP */
		sceIoLseek(fd, (SceOff) readbuf[8], 0);
		sceIoRead(fd, readbuf, 0x14);
		if (readbuf[0] != ELF_MAGIC) /* encrypted module */
			goto out;

		sceIoLseek(fd, (SceOff) (readbuf[8] + check[19]), 0);
		if (!IsStaticElf(readbuf))
			check[4] = readbuf[9] - readbuf[8];
	} else if (readbuf[0] == ELF_MAGIC) {
		sceIoLseek(fd, (SceOff) check[19], 0);
	} else
		goto out;

	sceIoRead(fd, &attr, 2);
	if (IsStaticElf(readbuf))
		check[17] = 0;
	else
		check[17] = !!(attr & 0x1000);

out:
	sceIoLseek(fd, (SceOff) pos, 0);
	return PartitionCheck(buf, check);
}

/* 0x00001CBC */
int
sceKernelProbeExecutableObject_Patched(void *buf, int *check)
{
	ASM_FUNC_TAG();
	int ret;
	Elf32_Ehdr *hdr;

	ret = sceKernelProbeExecutableObject(buf, check);

	if (*(int *) buf != ELF_MAGIC)
		return ret;
	if (check[2] < 0x52)
		return ret;
	hdr = buf;
	if (hdr->e_type != 2)
		return ret;
	check[8] = 3;

	return 0;
}

/* 0x00001E1C */
int
PatchSceChkReg(char *pscode)
{
	ASM_FUNC_TAG();
	int fakeregion;

	pscode[0] = 1;
	pscode[1] = 0;

	fakeregion = g_tnconfig.fakeregion;

	if (fakeregion >= 0xC)
		fakeregion += 2;
	else
		fakeregion -= 11;

	fakeregion &= 0xFF;
	if (fakeregion == 2)
		fakeregion = 3;
	pscode[6] = 1;
	pscode[4] = 1;
	pscode[2] = fakeregion;
	pscode[7] = 0;
	pscode[3] = 0;
	pscode[5] = 0;

	return 0;
}

/* 0x00001E74 */
void
PatchSceLoadExec(u32 text_addr)
{
	ASM_FUNC_TAG();
	/* 0x00006A40 */
	static u32 model4[] = 
	{ 0x00002F28, 0x00002F74, 0x000025A4, 0x000025E8, 0x00001674, 0x000016A8 };

	/* 0x00006A58 */
	static u32 model[] = 
	{ 0x00002CD8, 0x00002D24, 0x00002350, 0x00002394, 0x00001674, 0x000016A8 };

	u32 *p;

	if(g_model == 4)
		p = model4;
	else
		p = model;

	_sw(MAKE_CALL(LoadExecBootStart_Patched), text_addr + p[0]);
	_sw(0x3C0188FC, text_addr + p[1]);
	_sw(0x1000000B, text_addr + p[2]);
	_sw(0, text_addr + p[3]);
	LoadExecBootstart = (void *) text_addr;
	_sw(0x10000008, text_addr + p[4]);
	_sw(0, text_addr + p[5]);
}

/* 0x00001F28 */
int
sceDisplaySetHoldMode_Patched(int a0)
{
	ASM_FUNC_TAG();
	if (!g_tnconfig.skipgameboot)
		return sceDisplaySetHoldMode(a0);

	return 0;
}

/* 0x00001F50 */
void
PatchUpdatePlugin(u32 text_addr)
{
	ASM_FUNC_TAG();
	int ver = sctrlHENGetVersion();

	_sw((ver >> 16) | 0x3C050000, text_addr + 0x0000819C);
	_sw((ver & 0xFFFF) | 0x34A40000, text_addr + 0x000081A4);

	ClearCaches();
}

/* 0x00001FA4 */
void
PatchGamePlugin(u32 text_addr)
{
	ASM_FUNC_TAG();
	MAKE_DUMMY_FUNCTION0(text_addr + 0x0001EB08);

	if (g_tnconfig.hidepic) {
		_sw(0x00601021, text_addr + 0x0001C098);
		_sw(0x00601021, text_addr + 0x0001C0A4);
	}

	if (g_tnconfig.skipgameboot) {
		_sw(MAKE_CALL(text_addr + 0x000181BC), text_addr + 0x00017E5C);
		_sw(0x24040002, text_addr + 0x00017E60);
	}

	ClearCaches();
}

/* 0x00002058 */
void
PatchMsvideoMainPlugin(u32 text_addr)
{
	ASM_FUNC_TAG();
	_sh(0xFE00, text_addr + 0x0003AB2C);
	_sh(0xFE00, text_addr + 0x0003ABB4);
	_sh(0xFE00, text_addr + 0x0003D3AC);
	_sh(0xFE00, text_addr + 0x0003D608);

	_sh(0xFE00, text_addr + 0x00043B98);
	_sh(0xFE00, text_addr + 0x00073A84);
	_sh(0xFE00, text_addr + 0x000880A0);

	_sh(0x4003, text_addr + 0x0003D324);
	_sh(0x4003, text_addr + 0x0003D36C);
	_sh(0x4003, text_addr + 0x00042C40);

	ClearCaches();
}

/* 0x000020F0 */
void
PatchScePowerService(u32 text_addr)
{
	ASM_FUNC_TAG();
	_sw(0, text_addr + 0x00000CC8);
	ClearCaches();
}

/* 0x000020FC */
void
PatchSceWlanDriver(u32 text_addr)
{
	ASM_FUNC_TAG();
	_sw(0, text_addr + 0x00002690);
	ClearCaches();
}

/* 0x00002108 */
void
PatchSysconfPlugin(u32 text_addr)
{
	ASM_FUNC_TAG();
	/* 0x00006A70 */
	static wchar_t g_verinfo[] = L"6.20 TN- (HEN)";
	/* 0x00006A90 */
	static wchar_t g_macinfo[] = L"00:00:00:00:00:00";

	if (g_tnconfig.nospoofversion == 0) {
		int ver = sctrlHENGetVersion();

		g_verinfo[8] = (ver & 0xF) + 0x41;
		memcpy((void *) (text_addr + 0x000298AC), g_verinfo, sizeof(g_verinfo));
		_sw(0x3C020000 | ((text_addr + 0x000298AC) >> 16), text_addr + 0x00018920);
		_sw(0x34420000 | ((text_addr + 0x000298AC) & 0xFFFF), text_addr + 0x00018924);
	}

	if (g_tnconfig.showmac == 0) {
		memcpy((void *) (text_addr + 0x0002DB90), g_macinfo, sizeof(g_macinfo));
	}

	if (g_model == 0 && g_tnconfig.slimcolor != 0) {
		_sw(_lw(text_addr + 0x00007498), text_addr + 0x00007494);
		_sw(0x24020001, text_addr + 0x00007498);
	}

	ClearCaches();
}

/* 0x00002200 */
int
LoadExecBootStart_Patched(int a0, int a1, int a2, int a3)
{
	ASM_FUNC_TAG();
	memset((void *) 0x88FB0000, 0, 0x20);

	_sw(g_p2_size, 0x88FB0008);
	_sw(g_p8_size, 0x88FB000C);
	_sw((u32) g_reboot_module, 0x88FB0010);
	_sw((u32) g_reboot_module_buf, 0x88FB0014);
	_sw(g_reboot_module_size, 0x88FB0018);
	_sw(g_reboot_module_flags, 0x88FB001C);
	_sw(g_model, 0x88FB0000);
	_sw(g_rebootex_size, 0x88FB0004);
	memcpy((void *) 0x88FC0000, (void *) g_rebootex_buf, g_rebootex_size);

	return LoadExecBootstart(a0, a1, a2, a3);
}

/* 0x00002338 */
void
PatchRegion(void)
{
	ASM_FUNC_TAG();
	u32 orig_addr = sctrlHENFindFunction("sceChkreg", "sceChkreg_driver", 0x59F8491D); /* sceChkregGetPsCode */
	if (orig_addr) {
		if (g_tnconfig.fakeregion) {
			REDIRECT_FUNCTION(orig_addr, PatchSceChkReg);
		}
	}
	ClearCaches();
}

/* 0x000023A4 */
u32 __attribute__ ((noinline))
FindScePowerFunction(u32 nid)
{
	ASM_FUNC_TAG();
	return sctrlHENFindFunction("scePower_Service", "scePower", nid);
}

/* 0x0000240C */
u32 __attribute__ ((noinline))
FindScePowerDriverFunction(u32 nid)
{
	ASM_FUNC_TAG();
	return sctrlHENFindFunction("scePower_Service", "scePower_driver", nid);
}

/* 0x00002424 */
SceUInt
UsbChargingHandler(SceUID uid, SceInt64 unused0, SceInt64 unused1, void *common)
{
	ASM_FUNC_TAG();
	static int usbcharging_enabled = 0; /* 0x00008250 */
	static int g_00008254 = 0;

	void (*_scePowerBatteryDisableUsbCharging) (int);
	void (*_scePowerBatteryEnableUsbCharging) (int);
	int (*_scePowerIsBatteryCharging) (void) = (void *) FindScePowerFunction(0x1E490401); /* scePowerIsBatteryCharging */

	if (_scePowerIsBatteryCharging())
		return 0x001E8480;

	if (usbcharging_enabled == 1) {
		if (g_00008254 != 0) {
			_scePowerBatteryDisableUsbCharging = (void *) FindScePowerDriverFunction(0x90285886); /* scePowerBatteryDisableUsbCharging */
			_scePowerBatteryDisableUsbCharging(0);
		}
		g_00008254 = (g_00008254 < 1);
		usbcharging_enabled = 0;

		return 0x004C4B40;
	}

	_scePowerBatteryEnableUsbCharging = (void *) FindScePowerDriverFunction(0x733F973B); /* scePowerBatteryEnableUsbCharging */
	_scePowerBatteryEnableUsbCharging(1);
	usbcharging_enabled = 1;

	return 0x00E4E1C0;
}

/* 0x000024E4 */
void
PatchVsh(u32 text_addr)
{
	ASM_FUNC_TAG();
	u32 text_addr2;

	if (g_tnconfig.vshcpuspeed != 0)
		g_timestamp_4 = sceKernelGetSystemTimeLow();

	_sw(0, text_addr + 0x00011A70);
	_sw(0, text_addr + 0x00011A78);
	_sw(0, text_addr + 0x00011D84);

	text_addr2 = find_text_addr_by_name("sceVshBridge_Driver");

	g_scePowerGetCpuClockFrequency = (void *) FindScePowerFunction(0xFEE03A2F); /* scePowerGetCpuClockFrequency */

	_sw(MAKE_CALL(sceCtrlReadBufferPositive_Patched), text_addr2 + 0x25C);

	g_sceCtrlReadBufferPositive = (void *) sctrlHENFindFunction("sceController_Service", "sceCtrl", 0x1F803938); /* sceCtrlReadBufferPositive */

	PatchSyscall((u32) g_sceCtrlReadBufferPositive, sceCtrlReadBufferPositive_Patched);

	_sw(MAKE_CALL(PatchSceUpdateDL), text_addr2 + 0x1564);
	_sw(MAKE_CALL(sceDisplaySetHoldMode_Patched), text_addr2 + 0x1A14);
	sceDisplaySetHoldMode = (void *) (text_addr2 + 0x5570);

	ClearCaches();
}

/* 0x000026D4 */
SceUID 
PatchSceUpdateDL(const char *path, int flags, SceKernelLMOption *option)
{
	ASM_FUNC_TAG();
	u32 ret, k1;

	if ((ret = sceKernelLoadModuleVSH(path, flags, option)) < 0)
		return ret;

	k1 = pspSdkSetK1(0);
	if(!g_tnconfig.notnupdate) {
		SceModule2 *mod = sceKernelFindModuleByName("SceUpdateDL_Library");

		if(mod) {
			if(sceKernelFindModuleByName("sceVshNpSignin_Module")) {
				if(sceKernelFindModuleByName("npsignup_plugin_module")) {
					strcpy((char *) mod->text_addr + 0x32BC, "http://total-noob.blogspot.com/updatelist.txt");
					ClearCaches();
				}
			}
		}
	}
	pspSdkSetK1(k1);

	return ret;
}

/* 0x00002780 */
void
PatchSceMediaSync(u32 text_addr)
{
	ASM_FUNC_TAG();
	SceModule2 *loadexec;
	char *fname;

	if((fname = sceKernelInitFileName())) {
		if(strstr(fname, ".PBP")) {
			_sw(0x8821, text_addr + 0x960);
			_sw(0x8821, text_addr + 0x83C);
			ClearCaches();
		}
	}

	loadexec = sceKernelFindModuleByName("sceLoadExec");
	PatchSceLoadExec(loadexec->text_addr);
	PatchSceMesgLed();
	ClearCaches();
}


/* 0x00002948 */
int
sceCtrlReadBufferPositive_Patched(SceCtrlData *pad_data, int count)
{
	ASM_FUNC_TAG();
	/* needed by fastscroll */
#if 0
	static int g_00008248;
	static int g_0000824C;
	int i;
	unsigned int *pbuttons, buttons, a1, a2;
#endif
	static SceUID g_satelite_mod_id = -1; /* 0x000083C0 */

	SceKernelLMOption opt = {
		.size = 0x14,
		.mpidtext = 5,
		.mpiddata = 5,
		.flags = 0,
		.access = 1,
	};
	int k1, ret;
	unsigned int now;
	SceUID modid;
   
	ret = g_sceCtrlReadBufferPositive(pad_data, count);

	k1 = pspSdkSetK1(0);

	if (g_timestamp_2 == 0) {
		if (g_tnconfig.vshcpuspeed != 0 &&
				g_tnconfig.vshcpuspeed != 222) {
			if (g_scePowerGetCpuClockFrequency() == 222) {
				now = sceKernelGetSystemTimeLow();
				g_timestamp_3 = now;
				if (now - g_timestamp_1 >= 10000000) {
					SetSpeed(g_tnconfig.vshcpuspeed, g_tnconfig.vshbusspeed);
				}
				g_timestamp_1 = g_timestamp_3;
			}
		}
	} else {
		if (g_tnconfig.vshcpuspeed != 0) {
			now = sceKernelGetSystemTimeLow();
			g_timestamp_3 = now;
			g_timestamp_2 = now;
			if (now - g_timestamp_4 >= 10000000) {
				SetSpeed(g_tnconfig.vshcpuspeed, g_tnconfig.vshbusspeed);
			}
			g_timestamp_1 = g_timestamp_3;
		}
	}

	if (sceKernelFindModuleByName("TNVshMenu")) {
		if (VshMenuCtrl) {
			VshMenuCtrl(pad_data, count);
		} else {
			if (g_satelite_mod_id >= 0) {
				if (sceKernelStopModule(g_satelite_mod_id, 0, 0, 0, 0) >= 0) {
					sceKernelUnloadModule(g_satelite_mod_id);
					g_satelite_mod_id = -1;
				}
			}
		}
	} else {
		/* fastscroll is not needed -_- */
#if 0
		if (g_tnconfig.fastscroll) {
			if (sceKernelFindModuleByName("music_browser_module")) {
				a2 = g_00008248;
				a1 = g_0000824C;
				pbuttons = &pad_data->Buttons;

				for (i = 0; i < count; i++) {
					buttons = *pbuttons;

					if (buttons & PSP_CTRL_UP) {
						if (a2 >= 8) {
							buttons ^= PSP_CTRL_UP;
							*pbuttons = buttons;
							a2 = 7;
						} else 
							a2++;
					} else
						a2 = 0;

					if (buttons & PSP_CTRL_DOWN) {
						if (a1 >= 8) {
							buttons ^= PSP_CTRL_DOWN;
							*pbuttons = buttons;
							a1 = 7;
						} else
							a1++;
					} else
						a1 = 0;

					if (a2 != 0 || a1 != 0) {
						*pbuttons = (buttons & 
										(PSP_CTRL_DOWN & PSP_CTRL_UP))
									^ buttons;
					}

					pbuttons += 4;
				} /* for loop */

				g_00008248 = a2;
				g_0000824C = a1;
			} /* music_browser_module */
		} /* fastscroll */
#endif

		if (sceKernelFindModuleByName("htmlviewer_plugin_module"))
			goto out;
		if (sceKernelFindModuleByName("sceVshOSK_Module"))
			goto out;
		if (sceKernelFindModuleByName("camera_plugin_module"))
			goto out;
		if (!(pad_data->Buttons & PSP_CTRL_SELECT))
			goto out;

		/* SELECT button is pressed! */
		sceKernelSetDdrMemoryProtection((void *) 0x08400000, 0x00400000, 0xF);
		modid = sceKernelLoadModuleBuffer(size_satelite_bin, satelite_bin, 0, &opt);
		if (modid >= 0) {
			g_satelite_mod_id = modid;
			sceKernelStartModule(modid, 0, 0, 0, 0);
			pad_data->Buttons &= 0xFFFFFFFE; /* clear PSP_CTRL_SELECT */
		}
	} /* non-TN-vsh */

out:
	pspSdkSetK1(k1);

	return ret;
}

/* 0x00002D38 */
void
PatchSceUmdCacheDriver(u32 text_addr)
{
	ASM_FUNC_TAG();
	int *p;

	if (sceKernelApplicationType() != PSP_INIT_KEYCONFIG_GAME)
		return;
	if (sceKernelBootFrom() != PSP_BOOT_MS)
		return;
	MAKE_DUMMY_FUNCTION1(text_addr + 0x000009C8);
	ClearCaches();

	for (p = (int *) 0xBC000040; p != (int *) 0xBC000080; p++)
		*p = -1;
}

/* 0x00002DBC */
void
PatchSceImposeDriver(void)
{
	ASM_FUNC_TAG();
	SceUID timer;
	SceModule2 *mod;
	u32 text_addr;

	sctrlSEGetConfig(&g_tnconfig);
	PatchRegion();
	if (g_model == 0)
		goto out;
	if (g_tnconfig.slimcolor == 0)
		goto out;
	timer = sceKernelCreateVTimer("", NULL);
	if (timer < 0)
		goto out;
	sceKernelStartVTimer(timer);
	sceKernelSetVTimerHandlerWide(timer, 5000000LL, UsbChargingHandler, 0);

	if ((mod = sceKernelFindModuleByName("sceUSB_Driver"))) {
		text_addr = mod->text_addr;
		MAKE_DUMMY_FUNCTION0(text_addr + 0x00008FE8);
		MAKE_DUMMY_FUNCTION0(text_addr + 0x00008FF0);
	}

out:
	ClearCaches();
}

/* un-used routine */
#if 0
void
sub_00002EB0(int a0, int a1, int a2)
{
	ASM_FUNC_TAG();
}
#endif

/* 0x00003938 */
int
PatchSceKernelStartModule(int text_addr, int a1)
{
	ASM_FUNC_TAG();
	//static char g_00008428[0x24];

	int (*func) (int, u32) = (void *) text_addr;

	//memset(g_00008428, 0, 0x24);
	_sw(MAKE_JMP(sceKernelStartModule_Patched), text_addr + 0x00000278);
	//_sw(a1, (u32) g_00008428 + 4);
	ClearCaches();

	return func(4, a1);
}

/* 0x000039BC */
void
StartPlugin(char *path)
{
	ASM_FUNC_TAG();
	SceModule2 *mod;
	SceUID uid;
	int i, nsegment;
	char *s, *end, *p;

	if ((uid = sceKernelLoadModule(path, 0, 0)) < 0)
		return;

	/* for PSP Go */
	if (g_model == 4) {
		if (!strncmp(path, "ef0:/", 5)) {
			mod = sceKernelFindModuleByUID(uid);
			p = (char *) mod;
			nsegment = mod->nsegment;
			end = (char *) (mod->segmentaddr[0]);

			for (i = 0; i < nsegment; i++) {
				p += 4;
				end += _lw((u32) p + 0x8C);
			}

			for (s = (char *) (mod->segmentaddr[0]); s < end; s += 4) {
				if (!strncmp(s, "ms0", 3)) {
					s[0] = 'e';
					s[1] = 'f';
				} else if (!strncmp(s, "fatms", 5)) {
					/* TN still changes s[0] and s[1], which seems to be buggy */
					s[3] = 'e';
					s[4] = 'f';
				}
			}
			ClearCaches();
		}
	}

	sceKernelStartModule(uid, strlen(path) + 1, path, 0, 0);
}

/* 0x00003B7C */
void
StrTrim(char *buf)
{
	ASM_FUNC_TAG();
	char *s = buf + strlen(buf) - 1;

	while (s >= buf && (*s == ' ' || *s == '\t')) {
		*s = '\0';
		s--;
	}
}

/* 0x00003BD4 */
int
ParsePluginsConfig(char *buf, int len, char *path, int *active)
{
	ASM_FUNC_TAG();
	int i, j;
	char c, *p, *s;

	p = path;
	for (i = 0, j = 0; i < len; i++) {
		c = buf[i];
		if (c >= ' ' || c == '\t') {
			*p = c;
			p++;
			j++;
		} else {
			if (j != 0)
				break;
		}
	}
	StrTrim(path);
	*active = 0;

	if (i > 0) {
		if ((p = strpbrk(path, " \t"))) {
			s = p;
			p++;
			while (*p < 0)
				p++;
			*active = strcmp(p, "1") < 1;
			*s = 0;
		}
	}

	return i;
}

/* 0x00003CB8 */
int
sceKernelStartModule_Patched(int modid, SceSize argsize, void *argp, int *modstatus, SceKernelSMOption *opt)
{
	ASM_FUNC_TAG();
	char *buf;
	char plugin_path[0x40];
	int fd, apptype, fpl, active, len, ret;

	if (!sceKernelFindModuleByUID(modid))
		goto out;

	if (!sceKernelFindModuleByName("sceMediaSync"))
		goto out;

	switch ((apptype = sceKernelApplicationType())) {
	case PSP_INIT_KEYCONFIG_VSH:
		fd = sceIoOpen("ms0:/seplugins/vsh.txt", 1, 0);
		break;

	case PSP_INIT_KEYCONFIG_GAME:
		fd = sceIoOpen("ms0:/seplugins/game.txt", 1, 0);
		break;

	default:
		goto out;
	}

	if (fd < 0) {
		fd = sceIoOpen(apptype == PSP_INIT_KEYCONFIG_VSH ? 
				"ef0:/seplugins/vsh.txt" : "ef0:/seplugins/game.txt", 
				1, 0);
		if (fd < 0)
			goto out;
	}

	if ((fpl = sceKernelCreateFpl("", 1, 0, 0x400, 1, NULL)) < 0)
		goto close_fd_out;

	sceKernelAllocateFpl(fpl, (void **) &buf, NULL);
	len = sceIoRead(fd, buf, 0x400);

	do {
		memset(plugin_path, 0, 0x40);
		active = 0;
		ret = ParsePluginsConfig(buf, len, plugin_path, &active);
		if (ret > 0) {
			len -= ret;
			if (active)
				StartPlugin(plugin_path);
		} else
			break;
	} while (1);

	if (buf) {
		sceKernelFreeFpl(fpl, buf);
		sceKernelDeleteFpl(fpl);
	}

close_fd_out:
	sceIoClose(fd);

out:
	return sceKernelStartModule(modid, argsize, argp, modstatus, opt);
}

/* 0x00003FA0 pspSdkSetK1 */
