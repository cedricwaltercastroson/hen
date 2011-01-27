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

PSP_MODULE_INFO("SystemControl", 0x3007, 1, 1);
PSP_MAIN_THREAD_ATTR(0);

#define MAKE_CALL(__f) \
	(((((unsigned int)__f) >> 2) & 0x03FFFFFF) | 0x0C000000)

#define MAKE_JMP(__f) \
	(((((u32)__f) & 0x0FFFFFFC) >> 2) | 0x08000000)

#define find_text_addr_by_name(__name) \
	(u32) _lw((u32) (sceKernelFindModuleByName(__name)) + 108)

extern int sub_00001CBC(u32, u32);
extern int sceKernelCheckExecFile_Patched(void *buf, int *check);
extern int sub_000012A0(int, int);
extern void PartitionCheck_Patched(int, int);
extern SceUID sceKernelCreateThread_Patched(const char *name, SceKernelThreadEntry entry, int priority, int stacksize, SceUInt attr, SceKernelThreadOptParam *opt);
extern int sceKernelStartThread_Patched(SceUID tid, SceSize len, void *p);
extern int sub_0000037C(PSP_Header *hdr);
extern int sceIoMkDir_Patched(char *dir, SceMode mode);
extern int sceIoAssign_Patched(const char *, const char *, const char *, int, void *, long);
extern void sub_000016D8(int, int, int, int);
extern int PatchSceKernelStartModule(int, int);
extern int sub_00001838(int, int, int, int, int, int, int, int);
extern int sctrlHENGetVersion(void);
extern int sceKernelStartModule_Patched(int modid, SceSize argsize, void *argp, int *modstatus, SceKernelSMOption *opt);
extern void SetSpeed(int, int);
extern int mallocinit(void);

extern void sub_00001F50(int);
extern void sub_00001FA4(int);
extern void sub_00002058(int);
extern void sub_000020F0(int);
extern void sub_000024E4(int);
extern void sub_00002780(int);
extern void sub_00002D38(int);
extern void sub_00002DBC(void);
extern void PatchSysconfPlugin(u32);


/* 0x000083E8 */
TNConfig g_tnconfig;

int g_model; /* 0x00008270 */
u32 g_000083A4;
int g_00008258;
int g_0000825C;
unsigned int g_rebootex_size; /* 0x00008264 */
void *g_alloc_addr; /* 0x0000828C */

int *g_apitype_addr; /* 0x00008288 */
char **g_filename_addr; /* 0x00008284 */
int *g_keyconfig_addr; /* 0x0000839C */

int g_0000827C;
int g_00008280;  
int g_00008260;
int g_00008290;
int g_0000826C;

int g_000083B8;
int g_000083C8;
int g_000083D0;
int g_000083CC;
int g_00008270;
int g_00008264;
int g_0000828C;

int g_000083B0;
int g_000083DC;
int g_000083D4;

int g_00008408; 
int g_00008244;

SceUID g_00008298;
void *g_00008274;

char g_00008428[0x24];

/* 0x00006A70 */
wchar_t g_verinfo[] = L"6.20 TN- (HEN)";
/* 0x00006A90 */
wchar_t g_macinfo[] = L"00:00:00:00:00:00";

u32 g_scePowerSetClockFrequency_addr; /* 0x000083AC */

int (*func_00008268)(void); 
int (*func_000083BC) (int, int, int, int);

int (*ProbeExec1) (void *, int *); /* 0x00008278 */
int (*ProbeExec2) (void *, int *); /* 0x000083A0 */
int (*PartitionCheck) (void *, void *); /* 0x00008294 */

SceUID g_heapid; /* 0x00008240 */


/* 0x00000000 */
int
is_static_elf(void *buf)
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

	addr = (u32) (buf + index);
	if (addr < 0x88800001U)
		return 0;

	addr = _lw(addr);
	check[22] = addr & 0xFFFF;

	return addr;
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
		if (i != 0x120 && i != 0x141 && i != 0x142 &&
				i != 0x143 && i != 0x140)
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
		if (is_static_elf(buf)) {
			check[8] = 3;
		}
		return res;
	}

	if (!(PatchExec2(buf, check) & 0xFF00))
		return res;

	check[17] = 1;

	return 0;
}

/* 0x0000031C */
u32
find_nid_in_lib(void *buf, u32 nid)
{
	int i, cnt;
	u32 *addr;
	int *lib = buf;

	cnt = lib[2];
	for (i = 0; i < cnt; i++) {
		addr = (u32 *) (i * 8 + lib[1]);
		if (*addr == nid)
			return addr[1];
	}

	return 0;
}

/* 0x00000364 */
u32
sctrlHENSetStartModuleHandler(u32 a0)
{
	u32 prev = g_000083A4;

	g_000083A4 = a0 | 0x80000000U;

	return prev;
}

int
sub_0000037C(PSP_Header *hdr)
{
	char *p, *end;
	PSP_Header *hdr2;

	if (hdr->signature != 0x5053507E) /* ~PSP */
		return 0;

	p = (char *) hdr;
	end = (char *) &hdr->seg_size[1];

	for (hdr2 = (PSP_Header *) p; p != end; p++) {
		if (hdr2->scheck[0] != 0 &&
				hdr->reserved2[0] != 0 &&
				hdr->reserved2[1] != 0)
			return func_00008268(); /* XXX at most 3 parameters */
	}

	return 0;
}

/* 0x000003D4 */
int
SystemCtrlForKernel_AC0E84D1(int a0)
{
	int prev = g_00008260;

	g_00008260 = a0;

	return prev;
}

/* 0x000003E4 */
int
SystemCtrlForKernel_1F3037FB(int a0)
{
	int prev = g_00008290;

	g_00008290 = a0;

	return prev;
}

/* 0x000003F4 */
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

/* 0x00000428 */
void *
find_lib_by_name(char *name)
{
	int i;
	char **s;

	if (name == NULL)
		return NULL;

	s = (char **) 0x00006888;

	for (i = 0; i < 27; i++, s += 3) {
		if (!strcmp(name, *s))
			return (void *) (0x00006888 + i / 12);
	}

	return NULL;
}

/* 0x000004B4 */
int
ProbeExec2_Patched(char *buf, int *check)
{
	Elf32_Ehdr *hdr;
	int res;

	res = ProbeExec2(buf, check);
	if (*(u32 *) buf != ELF_MAGIC)
		return res;

	hdr = (Elf32_Ehdr *) buf;
	if (hdr->e_type == 2 && (check[2] + 0xFEC0 < 5))
		check[2] = 0x120;

	if (check[19] != 0)
		return res;

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

	return 0;
}

/* 0x00000648 */
void
PatchModuleMgr(void)
{
	u32 text_addr;
	u32 fp;

	text_addr = find_text_addr_by_name("sceModuleManager");

	if (g_model == 4)
		_sw(MAKE_CALL(sub_00001CBC), text_addr + 0x00007C3C);

	_sw(MAKE_JMP(sceKernelCheckExecFile_Patched), text_addr + 0x00008854);

	PartitionCheck = (void *) (text_addr + 0x00007FC0);
	g_apitype_addr = (void *) (text_addr + 0x00009990);
	g_filename_addr = (void *) (text_addr + 0x00009994);
	g_keyconfig_addr = (void *) (text_addr + 0x000099EC);

	_sw(0, text_addr + 0x00000760);
	_sw(0x24020000, text_addr + 0x000007C0); /* addiu v0, $zr, 0	*/
	_sw(0, text_addr + 0x000030B0);
	_sw(0, text_addr + 0x0000310C);
	_sw(0x10000009, text_addr + 0x00003138); /* beq $zr, $zr, 0x9 ??? */
	_sw(0, text_addr + 0x00003444);
	_sw(0, text_addr + 0x0000349C);
	_sw(0x10000010, text_addr + 0x000034C8); /* beq $zr, $zr, 0x10 ??? */

	fp = MAKE_CALL(PartitionCheck_Patched);
	_sw(fp, text_addr + 0x000064FC);
	_sw(fp, text_addr + 0x00006878);

	_sw(MAKE_CALL(sub_000012A0), text_addr + 0x0000842C);
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
	if (sceKernelApplicationType() == 0x100)
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

	fp = MAKE_CALL(sub_0000037C);
	_sw(fp, text_addr + table[2]);
	_sw(fp, text_addr + table[3]);

	fp = MAKE_CALL(sub_000016D8);
	_sw(fp, text_addr + table[4]);

	func_00008268 = (void *) (text_addr + table[0]);
	g_0000827C = text_addr + 0x00000134;
	g_00008280 = text_addr + table[1];

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
	static u32 model23[] = {0x00001F5C, 0x000041F0, 0x00004684, 0x00001FEC};
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

	fp = MAKE_CALL(sub_00001838);
	_sw(fp, text_addr + 0x00001908);
	_sw(fp, text_addr + p[0]);
	g_0000826C = text_addr + 0xE0;
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
	void *lib_addr;
	int i, j, ent_sz, stub_cnt;
	void *ent_top;
	struct SceLibraryEntryTable *entry;
	u32 *entry_table;

	if (!(mod = (SceModule2 *) sceKernelFindModuleByName(module_name))) {
		if (!(mod = (SceModule2 *) sceKernelFindModuleByAddress((u32) module_name)))
			return 0;
	}

	if ((lib_addr = find_lib_by_name(lib_name)))
		nid = find_nid_in_lib(lib_addr, nid);

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
PatchVLF(u32 a0)
{
	u32 fp = sctrlHENFindFunction("VLF_Module", "VlfGui", a0);

	if (fp) {
		_sw(0x03E00008, fp);
		_sw(0x00001021, fp + 4);
	}
}

/* 0x00000D90 */
void
PatchModules(SceModule2 *mod)
{
	u32 text_addr;

#define __panic() do { *(int *) 0 = 0; } while (1)

	text_addr = mod->text_addr;

	if (!strcmp(mod->modname, "sceLowIO_Driver")) {
		if (mallocinit() < 0) {
			__panic();
		}
	} else if (!strcmp(mod->modname, "sceUmdCache_driver")) {
		sub_00002D38(text_addr);
	} else if (!strcmp(mod->modname, "sceMediaSync")) {
		sub_00002780(text_addr);
	} else if (!strcmp(mod->modname, "sceImpose_Driver")) {
		sub_00002DBC();
	} else if (!strcmp(mod->modname, "sceWlan_Driver")) {
		sub_000020F0(text_addr);
	} else if (!strcmp(mod->modname, "vsh_module")) {
		sub_000024E4(text_addr);
	} else if (!strcmp(mod->modname, "sysconf_plugin_module")) {
		PatchSysconfPlugin(text_addr);
	} else if (!strcmp(mod->modname, "msvideo_main_plugin_module")) {
		sub_00002058(text_addr);
	} else if (!strcmp(mod->modname, "game_plugin_module")) {
		sub_00001FA4(text_addr);
	} else if (g_00008408 == 0 && 
			!strcmp(mod->modname, "update_plugin_module")) {
		sub_00001F50(text_addr);
	} else if (!strcmp(mod->modname, "VLF_Module")) {
		PatchVLF(0x2A245FE6);
		PatchVLF(0x7B08EAAB);
		PatchVLF(0x22050FC0);
		PatchVLF(0x158EE61A);
		PatchVLF(0xD495179F);
		ClearCaches();
	}

	if (g_00008244 == 0) {
		if (sceKernelGetSystemStatus() != 0x00020000)
			return;
		if (sceKernelApplicationType() == 0x200)
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

	_sw(text_addr + 0x00008B58, text_addr + 0x00008B74);
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

	g_000083A4 = 0x80000D90;
	g_00008258 = _lw(0x88FB0008);
	g_0000825C = _lw(0x88FB000C);
	g_rebootex_size = _lw(0x88FB0004); /* from launcher: uncompressed rebootex size */

	partition_id = sceKernelAllocPartitionMemory(1, "", 1, g_rebootex_size, 0);
	if (partition_id >= 0) {
		g_alloc_addr = sceKernelGetBlockHeadAddr(partition_id);
		memset(g_alloc_addr, 0, g_rebootex_size);
		memcpy(g_alloc_addr, (void *) 0x88FC0000, g_rebootex_size);
	}

	ClearCaches();

	return 0;
}

int
sub_000012A0(int a0, int a1)
{
	return 0;
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
		g_00008298 = tid;
		g_00008274 = (SceModule2 *) sceKernelFindModuleByAddress((u32) entry);
	}

	return tid;
}

/* 0x0000165C */
int
sceKernelStartThread_Patched(SceUID tid, SceSize len, void *p)
{
	int (*func) (void *, SceSize, void *);

	if (tid == g_00008298) {
		g_00008298 = -1;
		if ((g_000083A4 != 0) && (g_00008274 != 0)) {
			func = (void *) g_000083A4;
			return func(g_00008274, len, p);
		}
	}
	return sceKernelStartThread(tid, len, p);
}

void
sub_000016D8(int a0, int a1, int a2, int a3)
{
}

int
sub_00001838(int a0, int a1, int a2, int a3, int t0, int t1, int t2, int t3)
{
	return 0;
}

/* 0x00001A34 */
int
sceKernelCheckExecFile_Patched(void *buf, int *check)
{
	int res = PatchExec1(buf, check);

	if (res == 0)
		return res;

	res = sceKernelCheckExecFile(buf, check);
	return PatchExec3(buf, check, ((*(int *) buf) + 0xB9B3BA81) < 1, res);
}

/* 0x00001AB8 */
void
PartitionCheck_Patched(int a0, int a1)
{
}

int
sub_00001CBC(u32 a0, u32 a1)
{
	return 0;
}

/* 0x00001D50 */
void
SystemCtrlForKernel_CE0A654E(int a0, int a1, int a2, int a3)
{
	g_000083B8 = a0;
	g_000083C8 = a1;
	g_000083D0 = a2;
	g_000083CC = a3;
}

/* 0x00001D74 */
void
SystemCtrlForKernel_B86E36D1(void)
{
	int *(*func)(int);
	int *res, *p;
	int s1;

	if (g_00008258 == 0)
		return;
	
	if (g_00008258 + g_0000825C >= 0x35)
		return;

	func = (void *) 0x88003E2C;
	res = func(2);
	s1 = g_00008258 << 20;
	res[2] = s1;
	p = (int *) res[4];
	p[5] = (g_0000825C << 21) | 0xFC;

	res = func(8);
	res[1] = s1 + 0x88800000;
	res[2] = g_0000825C << 20;
	p = (int *) res[4];
	p[5] = (g_0000825C << 21) | 0xFC;
}

int
sub_00001E1C(char *a0)
{
	int fakeregion, a1;

	a0[0] = 1;
	a0[1] = 0;

	fakeregion = g_tnconfig.fakeregion;

	if (fakeregion < 0xC) {
		fakeregion += 2;
		fakeregion += 0xFFF5;
	}

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

void
sub_00001E74(int a0)
{
}

int
sub_00001F28(void)
{
	return 0;
}

void
sub_00001F50(int a0)
{
}

void
sub_00001FA4(int a0)
{
}

void
sub_00002058(int a0)
{
}

void
sub_000020F0(int a0)
{
	_sw(0, a0 + 0x00000CC8);
	ClearCaches();
}

void
sub_000020FC(int a0)
{
	_sw(0, a0 + 0x00002690);
	ClearCaches();
}

/* 0x00002108 */
void
PatchSysconfPlugin(u32 text_addr)
{
	if (g_tnconfig.nospoofversion == 0) {
		int ver = sctrlHENGetVersion();

		g_verinfo[9] = (ver & 0xF) + 0x41;
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

int
sub_00002200(int a0, int a1, int a2, int a3)
{
	char *s = (char *) 0x88FB0000;

	for (; s != (char *) 0x88FB0020; s++)
		*s = 0;

	_sw(g_00008258, 0x88FB0008);
	_sw(g_0000825C, 0x88FB000C);
	_sw(g_000083B8, 0x88FB0010);
	_sw(g_000083C8, 0x88FB0014);
	_sw(g_000083D0, 0x88FB0018);
	_sw(g_000083CC, 0x88FB001C);
	_sw(g_00008270, 0x88FB0000);
	_sw(g_00008264, 0x88FB0004);
	memcpy((void *) 0x88FC0000, (void *) g_0000828C, g_00008264);

	return func_000083BC(a0, a1, a2, a3);
}

/* 0x00002324 */
void
SetConfig(TNConfig *config)
{
	memcpy(&g_tnconfig, config, sizeof(TNConfig));
}

/* 0x00002338 */
void
PatchRegion(void)
{
	u32 orig_addr = sctrlHENFindFunction("sceChkreg", "sceChkreg_driver", 0x59F8491D);
	if (orig_addr) {
		if (g_tnconfig.fakeregion) {
			_sw(MAKE_JMP(sub_00001E1C), orig_addr);
			_sw(0x00000000, orig_addr + 4);
		}
	}
	ClearCaches();
}

/* 0x000023A4 */
u32
findFunctionIn_scePower_Service(u32 nid)
{
	return sctrlHENFindFunction("scePower_Service", "scePower", nid);
}

/* 0x000023BC */
void
sctrlHENSetSpeed(int cpuspd, int busspd)
{
	int (*_scePowerSetClockFrequency)(int, int, int);

	g_scePowerSetClockFrequency_addr = findFunctionIn_scePower_Service(0x545A7F3C);
	_scePowerSetClockFrequency = (void *) g_scePowerSetClockFrequency_addr;
	_scePowerSetClockFrequency(cpuspd, cpuspd, busspd);
}

u32
sub_0000240C(int a0)
{
	return 0;
}

void
sub_00002424(void)
{
}

void
sub_000024E4(int a0)
{
}

/* 0x00002620 */
int
vctrlVSHRegisterVshMenu(int a0)
{
	int k1 = pspSdkSetK1(0);

	g_000083B0 = a0 | 0x80000000;
	pspSdkSetK1(k1);

	return 0;
}

/* 0x00002664 */
int
sctrlHENSetMemory(int a0, int a1)
{
	int k1;

	if (!a0)
		goto out;
	if (a0 + a1 >= 0x35)
		goto out;

	k1 = pspSdkSetK1(0);
	g_00008258 = a0;
	g_0000825C = a1;
	pspSdkSetK1(k1);
	return 0;

out:
	return 0x80000107;
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
		SceModule2 *mod = (SceModule2 *) sceKernelFindModuleByName("SceUpdateDL_Library");
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

void
sub_00002780(int a0)
{
	SceModule2 *loadexec = NULL;

	if(sceKernelInitFileName()) {
		if(strstr(sceKernelInitFileName(), ".PBP")) {
			_sw(0x88210000, a0 + 0x960);
			_sw(0x88210000, a0 + 0x83C);
		}
	}

	loadexec = (SceModule2 *) sceKernelFindModuleByName("sceLoadExec");
	sub_00001E74(loadexec->text_addr);
	PatchSceMesgLed();
	ClearCaches();
}

/* 0x000027EC SystemCtrlForKernel_98012538 */
void
SetSpeed(int cpuspd, int busspd)
{
	u32 fp;
	int (*_scePowerSetClockFrequency)(int, int, int);

	switch (cpuspd) {
	case 0x14:
	case 0x4B:
	case 0x64:
	case 0x85:
	case 0xDE:
	case 0x10A:
	case 0x12C:
	case 0x14D:
		fp = findFunctionIn_scePower_Service(0x737486F2);
		g_scePowerSetClockFrequency_addr = fp;
		_scePowerSetClockFrequency = (void *) fp;
		_scePowerSetClockFrequency(cpuspd, cpuspd, busspd);

		if (sceKernelApplicationType() == 0x100)
			return;

		_sw(0x03E00008, fp);
		_sw(0x00001021, fp + 4);

		fp = findFunctionIn_scePower_Service(0x545A7F3C);
		_sw(0x03E00008, fp);
		_sw(0x00001021, fp + 4);

		/* scePowerSetBusClockFrequency */
		fp = findFunctionIn_scePower_Service(0xB8D7B3FB);
		_sw(0x03E00008, fp);
		_sw(0x00001021, fp + 4);

		/* scePowerSetCpuClockFrequency */
		fp = findFunctionIn_scePower_Service(0x843FBF43);
		_sw(0x03E00008, fp);
		_sw(0x00001021, fp + 4);

		fp = findFunctionIn_scePower_Service(0xEBD177D6);
		_sw(0x03E00008, fp);
		_sw(0x00001021, fp + 4);

		ClearCaches();
		break;
	
	default:
		break;
	}
}

/* 0x00002948 */
void
sceCtrlReadBufferPositive_Patched(SceCtrlData *pad_data, int count)
{
}

/* 0x00002C90 */
int
vctrlVSHExitVSHMenu(TNConfig *conf)
{
	int k1 = pspSdkSetK1(0);
	int cpuspeed = g_tnconfig.vshcpuspeed;

	g_000083B0 = 0;
	memcpy(&g_tnconfig, conf, sizeof(TNConfig));

	if (g_000083DC == 0) {
		if (cpuspeed != g_tnconfig.vshcpuspeed) {
			if (g_tnconfig.vshcpuspeed != 0) {
				SetSpeed(g_tnconfig.vshcpuspeed, g_tnconfig.vshbusspeed);
				g_000083D4 = sceKernelGetSystemTimeLow();
			}
		}
	}

	pspSdkSetK1(k1);

	return 0;
}

void
sub_00002D38(int a0)
{
}

void
sub_00002DBC(void)
{
}

void
sub_00002EB0(int a0, int a1, int a2)
{
}

/* 0x00002FDC */
int
mallocinit(void)
{
	u32 size;
	int apptype = sceKernelApplicationType();

	if (apptype == 0x100) {
		size = 14 * 1024;
		goto init_heap;
	}

	if (apptype != 0x200) {
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

/* 0x00003054  SystemCtrlForKernel_F9584CAD */
void *
oe_malloc(u32 size)
{
	return sceKernelAllocHeapMemory(g_heapid, size);
}

/* 0x000030A4 SystemCtrlForKernel_A65E8BC4 */
void
oe_free(void *ptr)
{
	sceKernelFreeHeapMemory(g_heapid, ptr);
}

/* 0x000030F4 SystemCtrlForUser_8E426F09 */
int
sctrlSEGetConfigEx(void *buf, SceSize size)
{
	int res = -1;
	int k1;
	SceUID fd;

	k1 = pspSdkSetK1(0);
	memset (buf, 0, size);
	if ((fd = sceIoOpen("flashfat1:/config.tn", PSP_O_RDONLY, 0)) > 0) {
		res = sceIoRead(fd, buf, size);
		sceIoClose(fd);
	}
	pspSdkSetK1(k1);

    return res;
}

/* 0x0000319C */
int
sctrlSEGetConfig(void *buf)
{
	return sctrlSEGetConfigEx(buf, 0x40);
}

/* 0x000031A4 SystemCtrlForUser_AD4D5EA5 */
int
sctrlSESetConfigEx(TNConfig *config, int size)
{
	int w;
	u32 k1;
	SceUID fd;

	k1 = pspSdkSetK1(0);
	if((fd = sceIoOpen("flashfat1:/config.tn", 0x602, 0x1FF)) < 0) {
		pspSdkSetK1(k1);
		return -1;
	}

	config->magic = 0x47434E54;
	if ((w = sceIoWrite(fd,config,size)) < size) {
		sceIoClose(fd);
		pspSdkSetK1(k1);
		return -1;
	}

	sceIoClose(fd);
	pspSdkSetK1(k1);

	return 0;
}

/* 0x0000325C */
int
sctrlSESetConfig(TNConfig *config)
{
	return sctrlSESetConfigEx(config, 0x40);
}

/* 0x00003264 */
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
int
kuKernelInitApitype(void)
{
	return sceKernelInitApitype();
}

/* 0x00003354 */
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
int
kuKernelBootFrom(void)
{
	return sceKernelBootFrom();
}

/* 0x000033AC */
int
kuKernelInitKeyConfig(void)
{
	return sceKernelApplicationType();
}

/* 0x000033B4 */
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
int
sctrlHENIsSE(void)
{
	return 1;
}

/* 0x000034B0 */
int
sctrlHENIsDevhook(void)
{
	return 0x0; /* not dummy routine */
}

/* 0x000034B8 */
int
sctrlHENGetVersion(void)
{
	return 0x00001000;
}

/* 0x000034C0 */
int
sctrlSEGetVersion(void)
{
	return 0x00020000;
}

/* 0x000034C8 */
int
sctrlKernelSetDevkitVersion(int a0)
{
	u32 k1;
	int old;

	k1 = pspSdkSetK1(0);
	old = sceKernelDevkitVersion();
	_sh(a0 >> 16, 0x88011AAC);
	_sh(a0 & 0xFFFF, 0x88011AB4);
	pspSdkSetK1(k1);

	return old;
}

/* 0x0000353C */
int
sctrlKernelSetUserLevel(int level)
{
	int res, k1;
	u32 text_addr, *thstruct;

	k1 = pspSdkSetK1(0);
	res = sceKernelGetUserLevel();
	text_addr = find_text_addr_by_name("sceThreadManager");
	thstruct = (void *) _lw(text_addr + 0x00019E80);
	thstruct[5] = (level ^ 8) << 28;
	pspSdkSetK1(k1);

	return res;
}

/* 0x000035B8 */
int
sctrlKernelLoadExecVSHWithApitype(int apitype, const char *file, struct SceKernelLoadExecVSHParam *param)
{
	int k1, res;
	u32 text_addr;
	int (*_sceKernelLoadExecVSHWithApitype)(int, const char *, void *);

	k1 = pspSdkSetK1(0);
	text_addr = find_text_addr_by_name("sceLoadExec");
	if (g_model == 4)
		_sceKernelLoadExecVSHWithApitype = (void *) (text_addr + 0x2558);
	else
		_sceKernelLoadExecVSHWithApitype = (void *) (text_addr + 0x2304);

	res = _sceKernelLoadExecVSHWithApitype(apitype, file, param);
	pspSdkSetK1(k1);

	return res;
}

/* 0x00003650 */
PspIoDrv *
sctrlHENFindDriver(char* drvname)
{
	u32 text_addr, k1;
	PspIoDrv *(*iomgr_find_driver)(char *);
	PspIoDrv *ret;

	k1 = pspSdkSetK1(0);
	text_addr = find_text_addr_by_name("sceIOFileManager");

	iomgr_find_driver = (void *) (text_addr + 0x2A38);
	ret = iomgr_find_driver(drvname);
	pspSdkSetK1(k1);

	return ret;
}

/* 0x000036C4 */
int
sctrlKernelLoadExecVSHDisc(const char *file, struct SceKernelLoadExecVSHParam *param)
{
	int res;
	int k1 = pspSdkSetK1(0);

	res = sceKernelLoadExecVSHDisc(file, param);
	pspSdkSetK1(k1);

	return res;
}

/* 0x00003720 */
int
sctrlKernelLoadExecVSHDiscUpdater(const char *file, struct SceKernelLoadExecVSHParam *param)
{
	int res;
	int k1 = pspSdkSetK1(0);

	res = sceKernelLoadExecVSHDiscUpdater(file, param);
	pspSdkSetK1(k1);

	return res;
}

/* 0x0000377C */
int
sctrlKernelLoadExecVSHMs1(const char *file, struct SceKernelLoadExecVSHParam *param)
{
	int res;
	int k1 = pspSdkSetK1(0);

	res = sceKernelLoadExecVSHMs1(file, param);
	pspSdkSetK1(k1);

	return res;
}

/* 0x000037D8 */
int
sctrlKernelLoadExecVSHMs2(const char *file, struct SceKernelLoadExecVSHParam *param)
{
	int res;
	int k1 = pspSdkSetK1(0);

	res = sceKernelLoadExecVSHMs2(file, param);
	pspSdkSetK1(k1);

	return res;
}

/* 0x00003834 */
int
sctrlKernelLoadExecVSHMs3(const char *file, struct SceKernelLoadExecVSHParam *param)
{
	int res;
	int k1 = pspSdkSetK1(0);

	res = sceKernelLoadExecVSHMs3(file, param);
	pspSdkSetK1(k1);

	return res;
}

/* 0x00003890 */
int
sctrlKernelLoadExecVSHMs4(const char *file, struct SceKernelLoadExecVSHParam *param)
{
	int res;
	int k1 = pspSdkSetK1(0);

	res = sceKernelLoadExecVSHMs4(file, param);
	pspSdkSetK1(k1);

	return res;
}

/* 0x000038EC */
int
sctrlKernelExitVSH(struct SceKernelLoadExecVSHParam *param)
{
	int res;
	int k1 = pspSdkSetK1(0);

	res = sceKernelExitVSHVSH(param);
	pspSdkSetK1(k1);

	return res;
}

/* 0x00003938 */
int
PatchSceKernelStartModule(int a0, int a1)
{
	int (*func) (int, u32) = (void *) a0;

	memset(g_00008428, 0, 0x24);
	_sw(MAKE_JMP(sceKernelStartModule_Patched), a0 + 0x00000278);
	_sw(a1, (u32) g_00008428 + 4);
	ClearCaches();

	return func(4, a1);
}

void
sub_000039BC(char *buf)
{
}

void
sub_00003B7C(int a0)
{
}

int
sub_00003BD4(char *buf, int len, char *path, int *active)
{
	return 0;
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
	case 0x100:
		fd = sceIoOpen("ms0:/seplugins/vsh.txt", 1, 0);
		break;

	case 0x200:
		fd = sceIoOpen("ms0:/seplugins/game.txt", 1, 0);
		break;

	default:
		goto out;
	}

	if (fd < 0) {
		fd = sceIoOpen(apptype == 0x100 ? 
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
		ret = sub_00003BD4(buf, len, plugin_path, &active);
		if (ret > 0) {
			len -= ret;
			if (active)
				sub_000039BC(plugin_path);
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
int
sctrlKernelSetInitKeyConfig(int key)
{
	int res;
	int k1 = pspSdkSetK1(0);

	res = sceKernelApplicationType();
	*g_keyconfig_addr = key;
	pspSdkSetK1(k1);

	return res;
}

/* 0x00003F04 */
char *
sctrlKernelSetInitFileName(char *file)
{
	int k1 = pspSdkSetK1(0);

	*g_filename_addr = file;
	pspSdkSetK1(k1);

	return NULL;
}

/* 0x00003F44 */
int
sctrlKernelSetInitApitype(int apitype)
{
	int res;
	int k1 = pspSdkSetK1(0);

	res = sceKernelInitApitype();
	*g_apitype_addr = apitype;
	pspSdkSetK1(k1);

	return res;
}

/* 0x00003FA0 pspSdkSetK1 */