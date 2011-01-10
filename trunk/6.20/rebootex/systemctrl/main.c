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

int model;
int g_000083A4;
int g_00008258;
int g_0000825C;
unsigned int rebootex_size;
void *alloc_addr;

int *apitype_addr;
int *filename_addr;
int *keyconfig_addr;

int (*ProbeExec1) (void *, int *);
int (*ProbeExec2) (void *, int *);
int (*PartitionCheck) (void *, void *);

int
hook_sceKernelStartThread(int a0, int a1, int a2)
{
	return 0;
}

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

void
PartitionCheckPatched(int a0, int a1)
{
}

int
sub_00001CBC(u32 a0, u32 a1)
{
	return 0;
}

void
sceKernelCheckExecFilePatched(void *buf, int *check)
{
}

int
ProbeExec1Patched(void *a0, void *a1)
{
	return 0;
}

int
ProbeExec2Patched(void *a0, void *a1)
{
	return 0;
}

void
sub_00003938(int a0, int a1)
{
}

void
ClearCaches(void)
{
	sceKernelIcacheInvalidateAll();
	sceKernelDcacheWritebackInvalidateAll();
}

u32
sctrlHENFindFunction(char *module, char *name, u32 addr)
{
	return 0;
}

void
PatchLoadCore(void)
{
	u32 text_addr;
	u32 fp;

	text_addr = find_text_addr_by_name("sceLoaderCore");

	_sw((u32) sceKernelCheckExecFilePatched, text_addr + 0x86B4);
	fp = MAKE_CALL(sceKernelCheckExecFilePatched);
	_sw(fp, text_addr + 0x1578);
	_sw(fp, text_addr + 0x15C8);
	_sw(fp, text_addr + 0x4A18);

	_sw(text_addr + 0x8B58, text_addr + 0x8B74);
	_sw(MAKE_CALL(ProbeExec1Patched), text_addr + 0x46A4);
	_sw(MAKE_CALL(ProbeExec2Patched), text_addr + 0x4878);
	_sw(0x3C090000, text_addr + 0x40A4);
	_sw(0, text_addr + 0x7E84);
	_sw(0, text_addr + 0x6880);
	_sw(0, text_addr + 0x6884);
	_sw(0, text_addr + 0x6990);
	_sw(0, text_addr + 0x6994);
	_sw(MAKE_CALL(sub_00003938), text_addr + 0x1DB0);
	_sw(0x02E02021, text_addr + 0x1DB4);

	ProbeExec1 = (void *) (text_addr + 0x61D4);
	ProbeExec2 = (void *) (text_addr + 0x60F0);

	fp = sctrlHENFindFunction("sceMemlmd", "memlmd", 0x2E208358);
	fp = MAKE_CALL(fp);

	_sw(fp, text_addr + 0x6914);
	_sw(fp, text_addr + 0x6944);
	_sw(fp, text_addr + 0x69DC);

	fp = sctrlHENFindFunction("sceMemlmd", "memlmd", 0xCA560AA6);
	fp = MAKE_CALL(fp);

	_sw(fp, text_addr + 0x41A4);
	_sw(fp, text_addr + 0x68F0);
}

void
PatchModuleMgr(void)
{
	u32 text_addr;
	u32 fp;

	text_addr = find_text_addr_by_name("sceModuleManager");

	if (model == 4)
		_sw(MAKE_CALL(sub_00001CBC), text_addr + 0x7C3C);

	_sw(MAKE_JMP(sceKernelCheckExecFilePatched), text_addr + 0x8854);

	PartitionCheck = (void *) (text_addr + 0x7FC0);
	apitype_addr = (void *) (text_addr + 0x9990);
	filename_addr = (void *) (text_addr + 0x9994);
	keyconfig_addr = (void *) (text_addr + 0x99EC);

	_sw(0, text_addr + 0x760);
	_sw(0x24020000, text_addr + 0x7C0); /* addiu v0, $zr, 0	*/
	_sw(0, text_addr + 0x30B0);
	_sw(0, text_addr + 0x310C);
	_sw(0x10000009, text_addr + 0x3138); /* beq $zr, $zr, 0x9 ??? */
	_sw(0, text_addr + 0x3444);
	_sw(0, text_addr + 0x349C);
	_sw(0x10000010, text_addr + 0x34C8); /* beq $zr, $zr, 0x10 ??? */

	fp = MAKE_CALL(PartitionCheckPatched);
	_sw(fp, text_addr + 0x64FC);
	_sw(fp, text_addr + 0x6878);

	_sw(MAKE_CALL(sub_000012A0), text_addr + 0x842C);
	_sw(0, text_addr + 0x4360);
	_sw(0, text_addr + 0x43A8);
	_sw(0, text_addr + 0x43C0);

	_sw(MAKE_JMP(hook_sceKernelCreateThread), text_addr + 0x894C);
	_sw(MAKE_JMP(hook_sceKernelStartThread), text_addr + 0x8994);
}

void
PatchMemlmd(void)
{
}

void
PatchIoFileMgr(void)
{
}

void
PatchInterruptMgr(void)
{
}

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
