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

#include "systemctrl_priv.h"

/* 0x00003264 */
/* KUBridge_4C25EA72 */
SceUID
kuKernelLoadModule(const char *path, int flags, SceKernelLMOption *option)
{
	ASM_FUNC_TAG();
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
	ASM_FUNC_TAG();
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
	ASM_FUNC_TAG();
	return sceKernelInitApitype();
}

/* 0x00003354 */
/* KUBridge_1742445F */
int
kuKernelInitFileName(char *fname)
{
	ASM_FUNC_TAG();
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
	ASM_FUNC_TAG();
	return sceKernelBootFrom();
}

/* 0x000033AC */
/* KUBridge_B0B8824E */
int
kuKernelInitKeyConfig(void)
{
	ASM_FUNC_TAG();
	return sceKernelApplicationType();
}

/* 0x000033B4 */
/* KUBridge_A2ABB6D3 */
int
kuKernelGetUserLevel(void)
{
	ASM_FUNC_TAG();
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
	ASM_FUNC_TAG();
	int ret;
	int k1 = pspSdkSetK1(0);

	ret = sceKernelSetDdrMemoryProtection(addr, size, prot);
	pspSdkSetK1(k1);

	return ret;
}

/* 0x00003464 */
/* KUBridge_24331850 */
int
kuKernelGetModel(void)
{
	ASM_FUNC_TAG();
	int ret;
	int k1 = pspSdkSetK1(0);

	ret = sceKernelGetModel();
	pspSdkSetK1(k1);

	return ret;
}
