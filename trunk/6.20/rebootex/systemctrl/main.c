#include <stdio.h>
#include <string.h>

#include "pspsdk.h"
#include "pspkernel.h"
#include "psputility.h"
#include "psputilsforkernel.h"

#define MAKE_CALL(__f) \
	(((((unsigned int)__f) >> 2) & 0x03FFFFFF) | 0x0C000000)

PSP_MODULE_INFO("SystemControl", 0x3007, 1, 1);
PSP_MAIN_THREAD_ATTR(0);

int model;
int g_000083A4;
int g_00008258;
int g_0000825C;
unsigned int rebootex_size;
void *alloc_addr;

int (*ProbeExec1) (void *, int *);
int (*ProbeExec2) (void *, int *);

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
	char *module;
	u32 text_addr;
	u32 fp;

	module = (char *) sceKernelFindModuleByName("sceLoaderCore");
	text_addr = _lw((u32) (module + 108)); /* module->text_addr */

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
