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

#include "systemctrl.h"

typedef struct {
	u32 oldnid;
	u32 newnid;
} nidentry_t;

typedef struct {
	char *name;
	nidentry_t *nids;
	int cnt;
} nidtable_t;

PSP_MODULE_INFO("SystemControl", 0x3007, 2, 5);
PSP_MAIN_THREAD_ATTR(0);

#define MAKE_CALL(__f) \
	(((((unsigned int)__f) >> 2) & 0x03FFFFFF) | 0x0C000000)

#define MAKE_JMP(__f) \
	(((((u32)__f) & 0x0FFFFFFC) >> 2) | 0x08000000)

#define find_text_addr_by_name(__name) \
	(u32) _lw((u32) (sceKernelFindModuleByName(__name)) + 108)

extern int sceKernelProbeExecutableObject_Patched(void *buf, int *check);
extern int sceKernelCheckExecFile_Patched(void *buf, int *check);
extern int sceKernelLinkLibraryEntries_Patched(void *buf, u32 size);
extern int PartitionCheck_Patched(void *buf, int *check);
extern SceUID sceKernelCreateThread_Patched(const char *name, SceKernelThreadEntry entry, int priority, int stacksize, SceUInt attr, SceKernelThreadOptParam *opt);
extern int sceKernelStartThread_Patched(SceUID tid, SceSize len, void *p);
extern int VerifySignCheck_Patched(void *hdr, int, int);
extern int sceIoMkDir_Patched(char *dir, SceMode mode);
extern int sceIoAssign_Patched(const char *, const char *, const char *, int, void *, long);
extern int DecryptExecutable_Patched(char *buf, int size, int *compressed_size, int polling);
extern int PatchSceKernelStartModule(int, int);
extern int DecryptPrx_Patched(int a0, int a1, int a2, char *buf, int size, int *compressed_size, int polling, int t3);
extern int sctrlHENGetVersion(void);
extern int sceKernelStartModule_Patched(int modid, SceSize argsize, void *argp, int *modstatus, SceKernelSMOption *opt);
extern void SetSpeed(int, int);
extern int mallocinit(void);

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

extern int sctrlSEGetConfig(void *);
extern int sceCtrlReadBufferPositive_Patched(SceCtrlData *, int);
extern SceUID PatchSceUpdateDL(const char *, int, SceKernelLMOption *);

extern u32 FindScePowerFunction(u32 nid);
extern int LoadExecBootStart_Patched(int a0, int a1, int a2, int a3);

static unsigned int size_satelite_bin;
static unsigned char satelite_bin[];

#include "satelite_bin.inc"

/* 0x000083E8 */
TNConfig g_tnconfig;

int g_model; /* 0x00008270 */
void (*ModuleStartHandler) (void *); /* 0x000083A4 */
int g_p2_size; /* 0x00008258 */
int g_p8_size; /* 0x0000825C */
unsigned int g_rebootex_size; /* 0x00008264 */
void *g_rebootex_buf; /* 0x0000828C */

int *g_apitype_addr; /* 0x00008288 */
char **g_init_filename_addr; /* 0x00008284 */
int *g_keyconfig_addr; /* 0x0000839C */

int (*DecryptExecutable)(void *, unsigned int, void *, unsigned int);  /* 0x0000827C */
void (*sceMemlmdInitializeScrambleKey)(void *, void *); /* 0x00008280 */

int (*DecryptExecutable_HEN)(char *buf, int size, int *compressed_size, int polling); /* 0x00008260 */
int (*DecryptPrx_HEN) (int a0, int a1, int a2, char *buf, int size, int *compressed_size, int polling, int t3); /* 0x00008290 */
int (*DecryptPrx) (int a0, int a1, int a2, char *buf, int size, int *compressed_size, int polling, int t3); /* 0x0000826C */

char *g_reboot_module; /* 0x000083B8 */
void *g_reboot_module_buf; /* 0x000083C8 */
int g_reboot_module_size; /* 0x000083D0 */
int g_reboot_module_flags; /* 0x000083CC */

SceUID g_satelite_mod_id; /* 0x000083C0 */

int (*VshMenuCtrl) (SceCtrlData *, int); /* 0x000083B0 */

/* 4 timestamps */
unsigned int g_timestamp_1; /* 0x000083D4 */
unsigned int g_timestamp_2; /* 0x000083DC */
unsigned int g_timestamp_3; /* 0x000083E0 */
unsigned int g_timestamp_4; /* 0x000083E4 */

SceUID g_SceModmgrStart_tid; /* 0x00008298 */
SceModule2 *g_SceModmgrStart_module; /* 0x00008274 */

int (*sceDisplaySetHoldMode) (int); /* 0x000083D8 */

int (*g_scePowerSetClockFrequency) (int, int, int); /* 0x000083AC */
int (*g_scePowerGetCpuClockFrequency) (void); /* 0x000083B4 */
int (*g_sceCtrlReadBufferPositive) (SceCtrlData *, int); /* 0x000083C4 */

int (*VerifySignCheck)(void *, int, int); /* 0x00008268 */
int (*LoadExecBootstart) (int, int, int, int); /* 0x000083BC */

int (*ProbeExec1) (void *, int *); /* 0x00008278 */
int (*ProbeExec2) (void *, int *); /* 0x000083A0 */
int (*PartitionCheck) (void *, void *); /* 0x00008294 */

SceUID g_heapid; /* 0x00008240 */


/* 0x00000000 */
int
IsStaticElf(void *buf)
{
	Elf32_Ehdr *hdr = buf;

	if (hdr->e_magic != ELF_MAGIC)
		return 0;
	return hdr->e_type == 2;
}

/* 0x00000038 */
int
PatchExec2(void *buf, int *check)
{
	int index = check[19];
	u32 addr;

	if (index < 0)
		index += 3;

	addr = (u32) buf + index;
	if (addr + 0x77C00000 < 0x00400001) /* same as addr < 0x88800001U */
		return 0;

	addr = index / 4;
	addr <<= 2;
	addr += (u32) buf;

	check[22] = _lh(addr);

	return _lw(addr);
}

/* 0x00000090 */
int
PatchExec1(void *buf, int *check)
{
	int i;

	if (_lw((u32) buf) != ELF_MAGIC)
		return -1;

	i = check[2];
	if (i >= 0x120) {
		switch (i) {
		case 0x120:
		case 0x141:
		case 0x142:
		case 0x143:
		case 0x140:
			break;

		default:
			return -1;
		}

		if (check[4] == 0) {
			if (check[17] == 0)
				return -1;
			check[18] = 1;
			return 0;
		}

		check[18] = 1;
		check[17] = 1;
		PatchExec2(buf, check);
	} else {
		if (i >= 0x52)
			return -1;
		if (check[17] == 0)
			return -2;
		check[18] = 1;
	}

	return 0;
}


/* 0x00000150 */
int
ProbeExec1_Patched(void *buf, int *check)
{
	int r;
	u16 attr, realattr;
	u16 *check2;

	r = ProbeExec1(buf, check);
	if (_lw((u32) buf) != ELF_MAGIC)
		return r;

	realattr = *(u16 *) (((check[19] >> 1) << 1) + (u32) buf);
	attr = realattr & 0x1E00;

	if (attr != 0) {
		check2 = (u16 *) check;
		if (attr != (check2[44] & 0x1E00))
			check2[44] = realattr;
	}

	if (check[18] == 0)
		check[18] = 1;

	return r;
}

/* 0x000001E4 */
char *
GetStrTab(void *buf)
{
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
	if (!is_plain)
		return res;

	if (check[2] >= 0x52) {
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

/* 0x00000364 */
/* SystemCtrlForUser_1C90BECB SystemCtrlForKernel_1C90BECB */
void *
sctrlHENSetStartModuleHandler(void *handler)
{
	void *prev = ModuleStartHandler;

	ModuleStartHandler = (void *) ((u32) handler | 0x80000000);

	return prev;
}

/* 0x0000037C */
int
VerifySignCheck_Patched(void *buf, int size, int polling)
{
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

/* 0x000003D4 */
void *
SystemCtrlForKernel_AC0E84D1(void *func)
{
	void *prev = DecryptExecutable_HEN;

	DecryptExecutable_HEN = func;

	return prev;
}

/* 0x000003E4 */
void *
SystemCtrlForKernel_1F3037FB(void *func)
{
	void *prev = DecryptPrx_HEN;

	DecryptPrx_HEN = func;

	return prev;
}

/* 0x000003F4 */
/* SystemCtrlForKernel_826668E9 */
void
PatchSyscall(u32 fp, u32 neufp)
{
	u32 sr;
	u32 *vectors, *end;
	u32 addr;

	__asm__ ("cfc0 $v0, $12;" : "=r"(sr));
	vectors = (u32 *) _lw(sr);
	end = vectors + 0x10000;

	do {
		addr = vectors[4];
		if (addr == fp)
			_sw(neufp, vectors[4]);
		vectors++;
	} while (vectors != end);
}

/* 0x00004EC0 */
#include "nidtables.inc"

/* 0x00000428 */
nidtable_t *
FindLibNidTable(const char *name)
{
	/* 0x00006888 */
#define NID_TABLE(__n) {#__n, __n##_##table, sizeof(__n##_##table)/sizeof(nidentry_t)}

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
ProbeExec2_Patched(char *buf, int *check)
{
	Elf32_Ehdr *hdr;
	int ret;

	ret = ProbeExec2(buf, check);
	if (*(u32 *) buf != ELF_MAGIC)
		return ret;

	hdr = (Elf32_Ehdr *) buf;
	if (hdr->e_type == 2 && (check[2] + 0xFEC0 < 5))
		check[2] = 0x120;

	if (check[19] != 0)
		return ret;

	if (hdr->e_type == 2) {
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
	u32 text_addr;

	text_addr = find_text_addr_by_name("sceIOFileManager");

	PatchSyscall(text_addr + 0x00001AAC, (u32) sceIoAssign_Patched);

#ifndef sceKernelApplicationType
#define sceKernelApplicationType InitForKernel_7233B5BC
#endif
	/* InitForKernel_7233B5BC() == PSP_INIT_KEYCONFIG_VSH */
	if (sceKernelApplicationType() == PSP_INIT_KEYCONFIG_VSH)
		PatchSyscall(text_addr + 0x00004260, (u32) sceIoMkDir_Patched);
}

/* 0x00000878 */
void
PatchMemlmd(void)
{
	/* 0x00006A0C */
	static u32 model0[] = {
		0x00000F10, 0x00001158, 0x000010D8, 0x0000112C,
		0x00000E10, 0x00000E74
	};

	/* 0x00006A24 */
	static u32 model1[] = {
		0x00000FA8, 0x000011F0, 0x00001170, 0x000011C4,
		0x00000EA8, 0x000011F0
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
	/* 0x000069CC */
	static u32 model0[] = {0x00001E3C, 0x00003808, 0x00003B4C, 0x00001ECC};
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
	sceKernelIcacheInvalidateAll();
	sceKernelDcacheWritebackInvalidateAll();
}

/* 0x00000BC4
 * SystemCtrlForKernel_159AF5CC
 */
u32
sctrlHENFindFunction(char *module_name, char *lib_name, u32 nid)
{
	SceModule2 *mod;
	nidtable_t *nidtbl;
	int i, j, ent_sz, stub_cnt;
	void *ent_top;
	struct SceLibraryEntryTable *entry;
	u32 *entry_table;
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
		entry = (void *) (i + ent_top);

		if (entry->libname && !strcmp(entry->libname, lib_name)) {
			if (entry->stubcount > 0) {
				stub_cnt =  entry->stubcount;
				entry_table = entry->entrytable;

				for (j = 0; j < stub_cnt; j++) {
					if (entry_table[j] == nid)
						return entry_table[j + stub_cnt + entry->vstubcount];
				}
			}
		}

		i += entry->len * 4;
	}

	return 0;
}

/* 0x00000D48 */
void
PatchVLF(u32 nid)
{
	u32 fp = sctrlHENFindFunction("VLF_Module", "VlfGui", nid);

	if (fp) {
		_sw(0x03E00008, fp);
		_sw(0x00001021, fp + 4);
	}
}

/* 0x00000D90 */
void
PatchModules(SceModule2 *mod)
{
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
	} else if (g_tnconfig.notnupdate == 0 && 
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
		g_00008244 = 0x00010000;
	}
}

/* 0x0000101C */
void
PatchLoadCore(void)
{
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

/* 0x0000119C */
int
module_bootstart(void)
{
	SceUID partition_id;

	g_model = _lw(0x88FB0000);

	PatchLoadCore();
	PatchModuleMgr();
	PatchMemlmd();
	PatchIoFileMgr();
	PatchInterruptMgr();

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
	u32 ver, nid, offs;
	u32 *pnid;
	nidtable_t *nidtbl;
	const char *lib_name;
	struct SceLibraryEntryTable *entry;
	int i, stubcount, ret;
	struct SceLibraryEntryTable *clib, *syscon, *power;

	/* module_sdk_version */
	ver = sctrlHENFindFunction(buf, NULL, 0x11B97506);
	if (ver) {
		if (*(int *) ver == 0x06020010) { /* 620 */
			return sceKernelLinkLibraryEntries(buf, size);
		}
	}

	offs = 0;
	clib = syscon = power = NULL;

	while (offs < size) {
		entry = buf + offs;
		lib_name = entry->libname;
		nidtbl = FindLibNidTable(lib_name);

		if (!strcmp(lib_name, "SysclibForKernel")) {
			clib = entry;
		} else if (!strcmp(lib_name, "sceSyscon_driver")) {
			syscon = entry;
		} else if (!strcmp(lib_name, "scePower_driver")) {
			power = entry;
		}

		if (nidtbl != NULL) {
			stubcount = entry->stubcount;
			for (i = 0; i < stubcount; i++) {
				pnid = entry->entrytable + (i << 2);
				if ((nid = TranslateNid(nidtbl, *pnid)))
					*pnid = nid;
			}
		}
		
		offs += entry->len << 2;
	}

	ret = sceKernelLinkLibraryEntries(buf, size);

	if (clib) {
		stubcount = clib->stubcount;
		for (i = 0; i < stubcount; i++) {
			nid = _lw((u32) (clib->entrytable + (i << 2)));

			if (nid == 0x909C228B || nid == 0x18FE80DB) { /* setjmp and longjmp */
				u32 addr = (u32) (clib + 1) + (i << 3);

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
			nid = _lw((u32) (syscon->entrytable + (i << 2)));

			if (nid == 0xC8439C57) { /* sceSysconPowerStandby */
				u32 addr = (u32) (syscon + 1) + (i << 3);
				u32 func = find_text_addr_by_name("sceSYSCON_Driver") + 0x2C64;

				_sw((((func >> 2) & 0x03FFFFFF) | 0x08000000), addr);
				_sw(0, addr + 4);
				ClearCaches();
			}
		}
	}

	if (power) {
		stubcount = syscon->stubcount;
		for (i = 0; i < stubcount; i++) {
			nid = _lw((u32) (power->entrytable + (i << 2)));

			if (nid == 0x737486F2) { /* scePowerSetClockFrequency */
				u32 func = FindScePowerFunction(0x737486F2);

				if (func) {
					u32 addr = (u32) (power + 1) + (i << 3);

					_sw((((func >> 2) & 0x03FFFFFF) | 0x08000000), addr);
					_sw(0, addr + 4);
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
	if (tid == g_SceModmgrStart_tid) {
		g_SceModmgrStart_tid = -1;
		if (ModuleStartHandler && g_SceModmgrStart_module) {
			ModuleStartHandler(g_SceModmgrStart_module);
		}
	}

	return sceKernelStartThread(tid, len, p);
}

/* 0x000016D8 */
int
DecryptExecutable_Patched(char *buf, int size, int *compressed_size, int polling)
{
	int r;
	PSP_Header* hdr = (PSP_Header *) buf;

	if (DecryptExecutable_HEN) {
		if (DecryptExecutable_HEN(buf, size, compressed_size, polling) >= 0)
			return 0;
	}

	if (hdr && compressed_size) {
		if (hdr->oe_tag == 0xC6BA41D3 || hdr->oe_tag == 0x55668D96) { /* M33 */
			if (buf[0x150] == 0x1F && buf[0x151] == 0x8B) { /* gzip */
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
	int r;
	PSP_Header* hdr = (PSP_Header*) buf;

	if (DecryptPrx_HEN) {
		if (DecryptPrx_HEN(a0, a1, a2, buf, size, compressed_size, polling, t3) >= 0)
			return 0;
	}

	if (a0 != 0 && buf && compressed_size) {
		switch (hdr->oe_tag) {
		case 0x28796DAA:
		case 0x7316308C:
		case 0x3EAD0AEE:
		case 0x8555ABF2:
			break;
		default:
			goto decrypt;
		}

		if (buf[0x150] == 0x1F && buf[0x151] == 0x8B) { /* gzip */
			memmove(buf, buf + 0x150, hdr->comp_size);
			*compressed_size = hdr->comp_size;
			return 0;
		}
	}

decrypt:
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
	int ret = PatchExec1(buf, check);

	if (ret == 0)
		return ret;

	ret = sceKernelCheckExecFile(buf, check);
	return PatchExec3(buf, check, ((*(int *) buf) + 0xB9B3BA81) < 1, ret);
}

/* 0x00001AB8 */
int
PartitionCheck_Patched(void *buf, int *check)
{
	static int readbuf[64]; /* 0x0000829C */

	u16 attr;
	SceUID fd;
	SceOff pos;

	fd = _lw((u32) buf + 0x18);
	pos = sceIoLseek(fd, check, 0, 0, 1);
	sceIoLseek(fd, check, 0, 0, 0);
	if (sceIoRead(fd, readbuf, 0x100) < 0x100)
		goto out;

	if (readbuf[0] == 0x50425000) { /* PBP */
		sceIoLseek(fd, readbuf, readbuf[8], 0, 0);
		sceIoRead(fd, readbuf, 0x14);
		if (readbuf[0] != ELF_MAGIC) /* encrypted module */
			goto out;

		sceIoLseek(fd, readbuf, readbuf[8] + check[19], 0, 0);
		if (!IsStaticElf(readbuf))
			check[4] = readbuf[8] - readbuf[7];
	} else if (readbuf[0] == ELF_MAGIC) {
		sceIoLseek(fd, readbuf, check[19], 0, 0);
	} else
		goto out;

	sceIoRead(fd, &attr, 2);
	if (IsStaticElf(readbuf))
		check[17] = 0;
	else
		check[17] = !!(attr & 0x1000);

out:
	sceIoLseek(fd, readbuf, pos, 0, 0);
	return PartitionCheck(buf, check);
}

/* 0x00001CBC */
int
sceKernelProbeExecutableObject_Patched(void *buf, int *check)
{
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

/* 0x00001D50 */
/* SystemCtrlForKernel_CE0A654E */
void
sctrlHENLoadModuleOnReboot(char *module_after, void *buf, int size, int flags)
{
	g_reboot_module = module_after;
	g_reboot_module_buf = buf;
	g_reboot_module_size = size;
	g_reboot_module_flags = flags;
}

/* 0x00001D74 */
void
SystemCtrlForKernel_B86E36D1(void)
{
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

/* 0x00001E1C */
int
PatchSceChkReg(char *a0)
{
	int fakeregion, a1;

	a0[0] = 1;
	a0[1] = 0;

	fakeregion = g_tnconfig.fakeregion;

	if (fakeregion >= 0xC)
		fakeregion += 2;
	else
		fakeregion += 0xFFF5;

	a1 = fakeregion & 0xFF;
	fakeregion = a1 ^ 2;
	if (fakeregion == 0)
		a1 = 3;
	a0[6] = 1;
	a0[4] = 1;
	a0[2] = a1;
	a0[7] = 0;
	a0[3] = 0;
	a0[5] = 0;

	return 0;
}

/* 0x00001E74 */
void
PatchSceLoadExec(u32 text_addr)
{
	/* 0x00006A40 */
	static u32 model[] = 
	{ 0x00002F28, 0x00002F74, 0x000025A4, 0x000025E8, 0x00001674, 0x000016A8 };

	/* 0x00006A58 */
	static u32 model4[] = 
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
	if (!g_tnconfig.skipgameboot)
		return sceDisplaySetHoldMode(a0);

	return 0;
}

/* 0x00001F50 */
void
PatchUpdatePlugin(u32 text_addr)
{
	int ver = sctrlHENGetVersion();

	_sw((ver >> 16) | 0x3C050000, text_addr + 0x0000819C);
	_sw((ver & 0xFFFF) | 0x34A40000, text_addr + 0x000081A4);

	ClearCaches();
}

/* 0x00001FA4 */
void
PatchGamePlugin(u32 text_addr)
{
	_sw(0x03E00008, text_addr + 0x0001EB08);
	_sw(0x00001021, text_addr + 0x0001EB0C);

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
	_sw(0, text_addr + 0x00000CC8);
	ClearCaches();
}

/* 0x000020FC */
void
PatchSceWlanDriver(u32 text_addr)
{
	_sw(0, text_addr + 0x00002690);
	ClearCaches();
}

/* 0x00002108 */
void
PatchSysconfPlugin(u32 text_addr)
{
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

/* 0x00002324 */
/* SystemCtrlForKernel_2F157BAF */
void
SetConfig(TNConfig *config)
{
	memcpy(&g_tnconfig, config, sizeof(TNConfig));
}

/* 0x00002338 */
void
PatchRegion(void)
{
	u32 orig_addr = sctrlHENFindFunction("sceChkreg", "sceChkreg_driver", 0x59F8491D); /* sceChkregGetPsCode */
	if (orig_addr) {
		if (g_tnconfig.fakeregion) {
			_sw(MAKE_JMP(PatchSceChkReg), orig_addr);
			_sw(0, orig_addr + 4);
		}
	}
	ClearCaches();
}

/* 0x000023A4 */
u32
FindScePowerFunction(u32 nid)
{
	return sctrlHENFindFunction("scePower_Service", "scePower", nid);
}

/* 0x000023BC */
/* SystemCtrlForKernel_CC9ADCF8 */
int
sctrlHENSetSpeed(int cpuspd, int busspd)
{
	g_scePowerSetClockFrequency = (void *) FindScePowerFunction(0x545A7F3C); /* scePowerSetClockFrequency */
	return g_scePowerSetClockFrequency(cpuspd, cpuspd, busspd);
}

/* 0x0000240C */
u32
FindScePowerDriverFunction(u32 nid)
{
	return sctrlHENFindFunction("scePower_Service", "scePower_driver", nid);
}

/* 0x00002424 */
int
UsbChargingHandler(void)
{
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

	PatchSyscall((u32) g_sceCtrlReadBufferPositive, (u32) sceCtrlReadBufferPositive_Patched);

	_sw(MAKE_CALL(PatchSceUpdateDL), text_addr2 + 0x1564);
	_sw(MAKE_CALL(sceDisplaySetHoldMode_Patched), text_addr2 + 0x1A14);
	sceDisplaySetHoldMode = (void *) (text_addr2 + 0x5570);

	ClearCaches();
}

/* 0x00002620 */
/* VshCtrlLib_FD26DA72 */
int
vctrlVSHRegisterVshMenu(void *ctrl)
{
	int k1 = pspSdkSetK1(0);

	VshMenuCtrl = (void *) ((u32) ctrl | 0x80000000);
	pspSdkSetK1(k1);

	return 0;
}

/* 0x00002664 */
/* SystemCtrlForUser_745286D1 SystemCtrlForKernel_745286D1 */
int
sctrlHENSetMemory(int p2, int p8)
{
	int k1;

	if (!p2 || ((p2 + p8) >= 0x35))
		return 0x80000107;

	k1 = pspSdkSetK1(0);
	g_p2_size = p2;
	g_p8_size = p8;
	pspSdkSetK1(k1);

	return 0;
}

/* 0x000026D4 */
SceUID 
PatchSceUpdateDL(const char *path, int flags, SceKernelLMOption *option)
{
	u32 ret, k1;

	if ((ret = sceKernelLoadModuleVSH(path, flags, option)) < 0)
		return ret;

	k1 = pspSdkSetK1(0);
	if(g_tnconfig.notnupdate) {
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

/* 0x000027EC */
/* SystemCtrlForKernel_98012538 */
void
SetSpeed(int cpuspd, int busspd)
{
	u32 fp;

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

		_sw(0x03E00008, fp);
		_sw(0x00001021, fp + 4);

		fp = FindScePowerFunction(0x545A7F3C);
		_sw(0x03E00008, fp);
		_sw(0x00001021, fp + 4);

		/* scePowerSetBusClockFrequency */
		fp = FindScePowerFunction(0xB8D7B3FB);
		_sw(0x03E00008, fp);
		_sw(0x00001021, fp + 4);

		/* scePowerSetCpuClockFrequency */
		fp = FindScePowerFunction(0x843FBF43);
		_sw(0x03E00008, fp);
		_sw(0x00001021, fp + 4);

		fp = FindScePowerFunction(0xEBD177D6);
		_sw(0x03E00008, fp);
		_sw(0x00001021, fp + 4);

		ClearCaches();
		break;
	
	default:
		break;
	}
}

/* 0x00002948 */
int
sceCtrlReadBufferPositive_Patched(SceCtrlData *pad_data, int count)
{
	static int g_00008248;
	static int g_0000824C;

	SceKernelLMOption opt = {
		.size = 0x14,
		.mpidtext = 5,
		.mpiddata = 5,
		.flags = 0,
		.access = 1,
	};
	int k1, ret, i;
	unsigned int now, *pbuttons, buttons, a1, a2;
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
				}
			}
		}
	} else {
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
			pad_data->Buttons &= 0xFFFE; /* clear PSP_CTRL_SELECT */
		}
	} /* non-TN-vsh */

out:
	pspSdkSetK1(k1);

	return ret;
}

/* 0x00002C90 */
/* VshCtrlLib_CD6B3913 */
int
vctrlVSHExitVSHMenu(TNConfig *conf)
{
	int k1 = pspSdkSetK1(0);
	int cpuspeed = g_tnconfig.vshcpuspeed;

	VshMenuCtrl = NULL;
	memcpy(&g_tnconfig, conf, sizeof(TNConfig));

	if (g_timestamp_2 == 0) {
		if (cpuspeed != g_tnconfig.vshcpuspeed &&
				g_tnconfig.vshcpuspeed != 0) {
			SetSpeed(g_tnconfig.vshcpuspeed, g_tnconfig.vshbusspeed);
			g_timestamp_1 = sceKernelGetSystemTimeLow();
		}
	}

	pspSdkSetK1(k1);

	return 0;
}

/* 0x00002D38 */
void
PatchSceUmdCacheDriver(u32 text_addr)
{
	u32 *i;

	if (sceKernelApplicationType() != PSP_INIT_KEYCONFIG_GAME)
		return;
	if (sceKernelBootFrom() != 0x40)
		return;
	_sw(0x03E00008, text_addr + 0x9C8);
	_sw(0x24020001, text_addr + 0x9CC);
	ClearCaches();

	for (i = (u32 *) 0xBC000040; i != (u32 *) 0xBC000080; i++)
		*i = 0xFFFF;
}

/* 0x00002DBC */
void
PatchSceImposeDriver(void)
{
	SceUID timer;
	SceModule2 *mod;
	u32 text_addr;

	sctrlSEGetConfig(&g_tnconfig);
	PatchRegion();
	if (g_model == 0)
		return;
	if (g_tnconfig.slimcolor == 0)
		return;
	timer = sceKernelCreateVTimer("", NULL);
	if (timer < 0)
		return;
	sceKernelStartVTimer(timer);
	sceKernelSetVTimerHandlerWide(timer, 0, 5000000, 0, UsbChargingHandler, 0);

	if ((mod = sceKernelFindModuleByName("sceUSB_Driver"))) {
		text_addr = mod->text_addr;
		_sw(0x03E00008, text_addr + 0x8FE8);
		_sw(0x00001021, text_addr + 0x8FEC);
		_sw(0x03E00008, text_addr + 0x8FF0);
		_sw(0x00001021, text_addr + 0x8FF4);
		ClearCaches();
	}
}

void
sub_00002EB0(int a0, int a1, int a2)
{
	/* un-used routine */
}

/* 0x00002FDC */
int
mallocinit(void)
{
	u32 size;
	int apptype = sceKernelApplicationType();

	if (apptype == PSP_INIT_KEYCONFIG_VSH) {
		size = 14 * 1024;
		goto init_heap;
	}

	if (apptype != PSP_INIT_KEYCONFIG_GAME) {
		size = 44 * 1024;
		goto init_heap;
	}

	if (sceKernelInitApitype() == 0x123)
		return 0;

	size = 44 * 1024;

init_heap:
	g_heapid = sceKernelCreateHeap(1, size, 1, "");

	return g_heapid < 0 ? g_heapid : 0;
}

/* 0x00003054 */
/* SystemCtrlForKernel_F9584CAD */
void *
oe_malloc(u32 size)
{
	return sceKernelAllocHeapMemory(g_heapid, size);
}

/* 0x000030A4 */
/* SystemCtrlForKernel_A65E8BC4 */
void
oe_free(void *ptr)
{
	sceKernelFreeHeapMemory(g_heapid, ptr);
}

/* 0x000030F4 */
/* SystemCtrlForUser_8E426F09 SystemCtrlForKernel_8E426F09 */
int
sctrlSEGetConfigEx(void *buf, SceSize size)
{
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
	return sctrlSEGetConfigEx(buf, 0x40);
}

/* 0x000031A4 */
/* SystemCtrlForUser_AD4D5EA5 SystemCtrlForKernel_AD4D5EA5 */
int
sctrlSESetConfigEx(TNConfig *config, int size)
{
	int ret = 0;
	u32 k1;
	SceUID fd;

	k1 = pspSdkSetK1(0);
	if((fd = sceIoOpen("flashfat1:/config.tn", 0x602, 0x1FF)) < 0) {
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
	return sctrlSESetConfigEx(config, 0x40);
}

/* 0x00003264 */
/* KUBridge_4C25EA72 */
SceUID
kuKernelLoadModule(const char *path, int flags, SceKernelLMOption *option)
{
	int k1;
	SceUID ret;

	k1 = pspSdkSetK1(0);

	ret = sceKernelLoadModule(path, flags, option);
	pspSdkSetK1(k1);

	return ret;
}

/* 0x000032D0 */
/* KUBridge_1E9F0498 */
SceUID
kuKernelLoadModuleWithApitype2(int apitype, const char *path, int flags, SceKernelLMOption *option)
{
	SceUID ret;
	int k1 = pspSdkSetK1(0);

	//ModuleMgrForKernel_B691CB9F
	ret = sceKernelLoadModuleForLoadExec(apitype, path, flags, option);
	pspSdkSetK1(k1);

	return ret;
}

/* 0x0000334C */
/* KUBridge_8E5A4057 */
int
kuKernelInitApitype(void)
{
	return sceKernelInitApitype();
}

/* 0x00003354 */
/* KUBridge_1742445F */
int
kuKernelInitFileName(char *fname)
{
	char *file;
	int k1 = pspSdkSetK1(0);

	file = sceKernelInitFileName();
	strcpy(fname, file);
	pspSdkSetK1(k1);

	return 0;
}

/* 0x000033A4 */
/* KUBridge_60DDB4AE */
int
kuKernelBootFrom(void)
{
	return sceKernelBootFrom();
}

/* 0x000033AC */
/* KUBridge_B0B8824E */
int
kuKernelInitKeyConfig(void)
{
	return sceKernelApplicationType();
}

/* 0x000033B4 */
/* KUBridge_A2ABB6D3 */
int
kuKernelGetUserLevel(void)
{
	int ret;
	int k1 = pspSdkSetK1(0);

	ret = sceKernelGetUserLevel();
	pspSdkSetK1(k1);

	return ret;
}

/* 0x000033F8 */
/* KUBridge_C4AF12AB */
int
kuKernelSetDdrMemoryProtection(void *addr, int size, int prot)
{
	int ret;
	int k1 = pspSdkSetK1(0);

	//SysMemForKernel_31DFE03F
	ret = sceKernelSetDdrMemoryProtection(addr, size, prot);
	pspSdkSetK1(k1);

	return ret;
}

/* 0x00003464 */
/* KUBridge_24331850 */
int
kuKernelGetModel(void)
{
	int ret;
	int k1 = pspSdkSetK1(0);

	//SysMemForKernel_864EBFD7
	ret = sceKernelGetModel();
	pspSdkSetK1(k1);

	return ret;
}

/* 0x000034A8 */
/* SystemCtrlForUser_D339E2E9 SystemCtrlForKernel_D339E2E9 */
int
sctrlHENIsSE(void)
{
	return 1;
}

/* 0x000034B0 */
/* SystemCtrlForUser_2E2935EF SystemCtrlForKernel_2E2935EF */
int
sctrlHENIsDevhook(void)
{
	return 0x0; /* not dummy routine */
}

/* 0x000034B8 */
/* SystemCtrlForUser_1090A2E1 SystemCtrlForKernel_1090A2E1 */
int
sctrlHENGetVersion(void)
{
	return 0x00001000;
}

/* 0x000034C0 */
/* SystemCtrlForUser_B47C9D77 SystemCtrlForKernel_B47C9D77 */
int
sctrlSEGetVersion(void)
{
	return 0x00020000;
}

/* 0x000034C8 */
/* SystemCtrlForUser_D8FF9B99 SystemCtrlForKernel_D8FF9B99 */
int
sctrlKernelSetDevkitVersion(int ver)
{
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
	int k1, ret;
	u32 text_addr;
	int (*_sceKernelLoadExecVSHWithApitype)(int, const char *, void *);

	k1 = pspSdkSetK1(0);
	text_addr = find_text_addr_by_name("sceLoadExec");
	if (g_model == 4)
		_sceKernelLoadExecVSHWithApitype = (void *) (text_addr + 0x2558);
	else
		_sceKernelLoadExecVSHWithApitype = (void *) (text_addr + 0x2304);

	ret = _sceKernelLoadExecVSHWithApitype(apitype, file, param);
	pspSdkSetK1(k1);

	return ret;
}

/* 0x00003650 */
/* SystemCtrlForUser_78E46415 SystemCtrlForKernel_78E46415 */
PspIoDrv *
sctrlHENFindDriver(char* drvname)
{
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
	int ret;
	int k1 = pspSdkSetK1(0);

	ret = sceKernelExitVSHVSH(param);
	pspSdkSetK1(k1);

	return ret;
}

/* 0x00003938 */
int
PatchSceKernelStartModule(int a0, int a1)
{
	static char g_00008428[0x24];

	int (*func) (int, u32) = (void *) a0;

	memset(g_00008428, 0, 0x24);
	_sw(MAKE_JMP(sceKernelStartModule_Patched), a0 + 0x00000278);
	_sw(a1, (u32) g_00008428 + 4);
	ClearCaches();

	return func(4, a1);
}

/* 0x000039BC */
void
StartPlugin(char *path)
{
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
	char *s = buf + strlen(buf);

	while ((*s == ' ' || *s == '\t') && s > buf) {
		*s = '\0';
		s--;
	}
}

/* 0x00003BD4 */
int
ParsePluginsConfig(char *buf, int len, char *path, int *active)
{
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

/* 0x00003EA8 */
/* SystemCtrlForUser_CB76B778 SystemCtrlForKernel_CB76B778 */
int
sctrlKernelSetInitKeyConfig(int key)
{
	int ret;
	int k1 = pspSdkSetK1(0);

	ret = sceKernelApplicationType();
	*g_keyconfig_addr = key;
	pspSdkSetK1(k1);

	return ret;
}

/* 0x00003F04 */
/* SystemCtrlForUser_128112C3 SystemCtrlForKernel_128112C3 */
char *
sctrlKernelSetInitFileName(char *file)
{
	int k1 = pspSdkSetK1(0);

	*g_init_filename_addr = file;
	pspSdkSetK1(k1);

	return NULL;
}

/* 0x00003F44 */
/* SystemCtrlForUser_8D5BE1F0 SystemCtrlForKernel_8D5BE1F0 */
int
sctrlKernelSetInitApitype(int apitype)
{
	int ret;
	int k1 = pspSdkSetK1(0);

	ret = sceKernelInitApitype();
	*g_apitype_addr = apitype;
	pspSdkSetK1(k1);

	return ret;
}

/* 0x00003FA0 pspSdkSetK1 */
