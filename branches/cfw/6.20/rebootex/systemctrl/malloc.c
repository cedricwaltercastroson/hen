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

#include "systemctrl_priv.h"

SceUID g_heapid = -1; /* 0x00008240 */

/* 0x00002FDC */
int
mallocinit(void)
{
	ASM_FUNC_TAG();
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
	ASM_FUNC_TAG();
	return sceKernelAllocHeapMemory(g_heapid, size);
}

/* 0x000030A4 */
/* SystemCtrlForKernel_A65E8BC4 */
void
oe_free(void *ptr)
{
	ASM_FUNC_TAG();
	sceKernelFreeHeapMemory(g_heapid, ptr);
}
