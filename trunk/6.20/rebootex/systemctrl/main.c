#include <stdio.h>
#include <string.h>

#include "pspsdk.h"
#include "pspkernel.h"
#include "psputility.h"
#include "psputilsforkernel.h"
#include "pspinit.h"

#define MAKE_CALL(__f) \
	(((((unsigned int)__f) >> 2) & 0x03FFFFFF) | 0x0C000000)

#define MAKE_JMP(__f) \
	(((((u32)__f) & 0x0FFFFFFC) >> 2) | 0x08000000)

#define find_text_addr_by_name(__name) \
	(u32) _lw((u32) (sceKernelFindModuleByName(__name)) + 108)

extern int sub_00001CBC(u32, u32);
extern void sceKernelCheckExecFile_Patched(void *buf, int *check);
extern int sub_000012A0(int, int);
extern void PartitionCheck_Patched(int, int);
extern int sceKernelCreateThread_Patched(int, int);
extern int sceKernelStartThread_Patched(int, int, int);
extern int sub_0000037C(int);
extern int sceIoMkDir_Patched(int, int);
extern int sceIoAssign_Patched(const char *, const char *, const char *, int, void *, long);
extern void sub_000016D8(int, int, int, int);
extern void sub_00003938(int, int);

PSP_MODULE_INFO("SystemControl", 0x3007, 1, 1);
PSP_MAIN_THREAD_ATTR(0);

/* 0x00006A0C */
u32 model0[] = {
	0x00000F10, 0x00001158, 0x000010D8, 0x0000112C,
	0x00000E10, 0x00000E74
};

/* 0x00006A24 */
u32 model1[] = {
	0x00000FA8, 0x000011F0, 0x00001170, 0x000011C4,
	0x00000EA8, 0x000011F0
};

int model; /* 0x00008270 */
int g_000083A4;
int g_00008258;
int g_0000825C;
unsigned int rebootex_size; /* 0x00008264 */
void *alloc_addr; /* 0x00008264 */

int *apitype_addr; /* 0x00008288 */
int *filename_addr; /* 0x00008284 */
int *keyconfig_addr; /* 0x0000839C */

int g_00008268; 
int g_0000827C;
int g_00008280;  

int (*ProbeExec1) (void *, int *); /* 0x00008278 */
int (*ProbeExec2) (void *, int *); /* 0x000083A0 */
int (*PartitionCheck) (void *, void *); /* 0x00008294 */

/* param of sub_00000000 */
typedef struct {
	u32 unk1;
	u8 pad[12];
	u8 unk2;
	u8 unk3;
} __attribute__((packed)) unk1_t;

int
sub_00000000(u32 a0)
{
	unk1_t *unk = (unk1_t *) a0;

	if (unk->unk1 == 0x464C457F)
		return 0;
	return (((unk->unk3 << 8) | unk->unk2) ^ 2) < 1;
}

u32
sub_00000038(u32 a0, u32 a1)
{
	int a2;
	u32 v0;

	a2 = (int) _lw(a1 + 0x4C);
	if (a2 < 0)
		a2 += 3;

	if ((a0 + (u32) a2 + 0x77C00000U) < 0x00400001U)
		return 0;

	v0 = (u32) a2 / 4;
	v0 <<= 2;
	v0 += a0;
	_sw(*(unsigned short *) v0, a1 + 0x58);

	return _lw(v0);
}

/* 0x00000090 */
int
PatchExec1(u32 a0, u32 a1)
{
	return 0;
}


/* 0x00000150 */
int
ProbeExec1_Patched(void *a0, void *a1)
{
	return 0;
}

int
sub_000001E4(u32 a0)
{
	return 0;
}

/* 0x00000280 */
int
PatchExec3(int a0, int a1, int a2, int a3)
{
	return 0;
}

void
sub_0000031C(int a0, int a1, int a2)
{
}

/* 0x00000364 */
int
SystemCtrlForUser_1C90BECB(int a0)
{
	return 0;
}

int
sub_0000037C(int a0)
{
	return 0;
}

/* 0x000003D4 */
int
SystemCtrlForKernel_AC0E84D1(u32 a0)
{
	return 0;
}

/* 0x000003E4 */
int
SystemCtrlForKernel_1F3037FB(u32 a0)
{
	return 0;
}

/* 0x000003F4 */
void
PatchSyscall(u32 fp, u32 neufp)
{
	u32 sr;
	u32 *vectors, *end;
	u32 addr;

	__asm__ ("cfc0 $v0, $12;":"=r"(sr));
	vectors = (u32 *) _lw(sr);
	end = vectors + 0x10000;

again:
	addr = vectors[4];
	if (addr == fp)
		_sw(neufp, vectors[4]);
	vectors++;
	if (vectors != end)
		goto again;
}

int
sub_00000428(int a0)
{
	return 0;
}

/* 0x000004B4 */
int
ProbeExec2_Patched(void *a0, void *a1)
{
	return 0;
}

/* 0x00000648 */
void
PatchModuleMgr(void)
{
	u32 text_addr;
	u32 fp;

	text_addr = find_text_addr_by_name("sceModuleManager");

	if (model == 4)
		_sw(MAKE_CALL(sub_00001CBC), text_addr + 0x00007C3C);

	_sw(MAKE_JMP(sceKernelCheckExecFile_Patched), text_addr + 0x00008854);

	PartitionCheck = (void *) (text_addr + 0x00007FC0);
	apitype_addr = (void *) (text_addr + 0x00009990);
	filename_addr = (void *) (text_addr + 0x00009994);
	keyconfig_addr = (void *) (text_addr + 0x000099EC);

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
	u32 text_addr;
	u32 fp;
	u32 *table;

	text_addr = find_text_addr_by_name("sceMemlmd");

	if (model == 0)
		table = model0;
	else
		table = model1;

	fp = MAKE_CALL(sub_0000037C);
	_sw(fp, text_addr + table[2]);
	_sw(fp, text_addr + table[3]);

	fp = MAKE_CALL(sub_000016D8);
	_sw(fp, text_addr + table[4]);

	g_00008268 = text_addr + table[0];
	g_0000827C = text_addr + 0x00000134;
	g_00008280 = text_addr + table[1];

	_sw(fp, text_addr + table[5]);
}

void
sub_0000094C(void)
{
}

/* 0x00000A28 */
int
sceIoMkDir_Patched(int a0, int a1)
{
	return 0;
}

/* 0x00000ABC */
int
sceIoAssign_Patched(const char *dev1, const char *dev2, const char *dev3,
		int mode, void *unk1, long unk2)
{
	return 0;
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
sctrlHENFindFunction(char *module, char *name, u32 nid)
{
	return 0;
}

void
sub_00000D48(u32 a0)
{
}

void
sub_00000D90(int a0)
{
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
	_sw(MAKE_CALL(sub_00003938), text_addr + 0x00001DB0);
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

	model = _lw(0x88FB0000);

	PatchLoadCore();
	PatchModuleMgr();
	PatchMemlmd();
	PatchIoFileMgr();
	PatchInterruptMgr();

	ClearCaches();

	g_000083A4 = 0x80000D90;
	g_00008258 = _lw(0x88FB0008);
	g_0000825C = _lw(0x88FB000C);
	rebootex_size = _lw(0x88FB0004); /* from launcher: uncompressed rebootex size */

	partition_id = sceKernelAllocPartitionMemory(1, "", 1, rebootex_size, 0);
	if (partition_id >= 0) {
		alloc_addr = sceKernelGetBlockHeadAddr(partition_id);
		memset(alloc_addr, 0, rebootex_size);
		memcpy(alloc_addr, (void *) 0x88FC0000, rebootex_size);
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
int
sceKernelCreateThread_Patched(int a0, int a1)
{
	return 0;
}

/* 0x0000165C */
int
sceKernelStartThread_Patched(int a0, int a1, int a2)
{
	return 0;
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
void
sceKernelCheckExecFile_Patched(void *buf, int *check)
{
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
}

/* 0x00001D74 */
int
SystemCtrlForKernel_B86E36D1(void)
{
	return 0;
}

int
sub_00001E1C(int a0)
{
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
sub_00003938(int a0, int a1)
{
}
