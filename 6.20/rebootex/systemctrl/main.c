#include <stdio.h>
#include <string.h>

#include "pspsdk.h"
#include "pspkernel.h"
#include "psputility.h"
#include "psputilsforkernel.h"

#define MAKE_CALL(__f) \
	(((((unsigned int)__f) >> 2) & 0x03FFFFFF) | 0x0C000000)

#define MAKE_JMP(__f) \
	(((((u32)__f) & 0x0FFFFFFC) >> 2) | 0x08000000)

#define find_text_addr_by_name(__name) \
	(u32) _lw((u32) (sceKernelFindModuleByName(__name)) + 108)


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

int (*ProbeExec1) (void *, int *); /* 0x00008278 */
int (*ProbeExec2) (void *, int *); /* 0x000083A0 */
int (*PartitionCheck) (void *, void *); /* 0x00008294 */

/* 0x0000165C */
int
hook_sceKernelStartThread(int a0, int a1, int a2)
{
	return 0;
}

/* 0x000015E8 */
int
hook_sceKernelCreateThread(int a0, int a1)
{
	return 0;
}

int
sub_000012A0(int a0, int a1)
{
	return 0;
}

/* 0x00001AB8 */
void
PartitionCheckPatched(int a0, int a1)
{
}

int
sub_00001CBC(u32 a0, u32 a1)
{
	return 0;
}

/* 0x00001A34 */
void
sceKernelCheckExecFilePatched(void *buf, int *check)
{
}

/* 0x00000150 */
int
ProbeExec1Patched(void *a0, void *a1)
{
	return 0;
}

/* 0x000004B4 */
int
ProbeExec2Patched(void *a0, void *a1)
{
	return 0;
}

void
sub_00003938(int a0, int a1)
{
}

/* 0x00000BA8 */
void
ClearCaches(void)
{
	sceKernelIcacheInvalidateAll();
	sceKernelDcacheWritebackInvalidateAll();
}

/* 0x00000BC4 */
u32
sctrlHENFindFunction(char *module, char *name, u32 addr)
{
	return 0;
}

/* 0x0000101C */
void
PatchLoadCore(void)
{
	u32 text_addr;
	u32 fp;

	text_addr = find_text_addr_by_name("sceLoaderCore");

	_sw((u32) sceKernelCheckExecFilePatched, text_addr + 0x000086B4);
	fp = MAKE_CALL(sceKernelCheckExecFilePatched);
	_sw(fp, text_addr + 0x00001578);
	_sw(fp, text_addr + 0x000015C8);
	_sw(fp, text_addr + 0x00004A18);

	_sw(text_addr + 0x00008B58, text_addr + 0x00008B74);
	_sw(MAKE_CALL(ProbeExec1Patched), text_addr + 0x000046A4);
	_sw(MAKE_CALL(ProbeExec2Patched), text_addr + 0x00004878);
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

/* 0x00000648 */
void
PatchModuleMgr(void)
{
	u32 text_addr;
	u32 fp;

	text_addr = find_text_addr_by_name("sceModuleManager");

	if (model == 4)
		_sw(MAKE_CALL(sub_00001CBC), text_addr + 0x00007C3C);

	_sw(MAKE_JMP(sceKernelCheckExecFilePatched), text_addr + 0x00008854);

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

	fp = MAKE_CALL(PartitionCheckPatched);
	_sw(fp, text_addr + 0x000064FC);
	_sw(fp, text_addr + 0x00006878);

	_sw(MAKE_CALL(sub_000012A0), text_addr + 0x0000842C);
	_sw(0, text_addr + 0x00004360);
	_sw(0, text_addr + 0x000043A8);
	_sw(0, text_addr + 0x000043C0);

	_sw(MAKE_JMP(hook_sceKernelCreateThread), text_addr + 0x0000894C);
	_sw(MAKE_JMP(hook_sceKernelStartThread), text_addr + 0x00008994);
}

/* 0x00000878 */
void
PatchMemlmd(void)
{
}

/* 0x00000814 */
void
PatchIoFileMgr(void)
{
}

/* 0x000007DC */
void
PatchInterruptMgr(void)
{
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
