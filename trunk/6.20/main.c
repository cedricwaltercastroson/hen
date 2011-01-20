#include <stdio.h>
#include <string.h>

#include "pspsdk.h"
#include "pspkernel.h"
#include "psputility.h"
#include "psputilsforkernel.h"

#ifdef DEBUG
#include "pspdebug.h"
#define printf pspDebugScreenPrintf
#define loginit pspDebugScreenInit
#define log printf
#else
#define loginit()
#define log(fmt...)
#endif

PSP_MODULE_INFO("HEN", 0, 1, 0);

extern void sceKernelPowerLock(unsigned int, unsigned int);

/* DecompressReboot */
static int (*func_rebootex)(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);

/* model number */
static int model;

#define panic() do {\
	__asm__ ("break");\
} while (0)

static unsigned int size_rebootex_bin;
static unsigned char rebootex_bin[];

#include "rebootex_bin.inc"


/* sub_000002B4 */
static int
rebootex_callback(unsigned int a1, unsigned int a2, unsigned int a3, 
		unsigned int a4, unsigned int a5)
{
	memcpy((void *) 0x88FC0000, rebootex_bin, size_rebootex_bin);
	memset((void *) 0x88FB0000, 0, 256);

	_sw(size_rebootex_bin, 0x88FB0004);
	_sw(model, 0x88FB0000);

	return func_rebootex(a1, a2, a3, a4, a5);
}

#define MAKE_CALL(a, f) _sw(0x0C000000 | (((u32)(f) >> 2) & 0x03FFFFFF), a)

/* sub_00000328 */
static int
power_callback(void)
{
	unsigned int (*_sceKernelFindModuleByName)(char *) = (void *) 0x8801EB78;
	int (*_sceKernelGetModel)(void) = (void *) 0x8800A1C4;
	void (*_sceKernelIcacheInvalidateAll)(void) = (void *) 0x88000E98;
	void (*_sceKernelDcacheWritebackInvalidateAll)(void) = (void *) 0x88000744;
	unsigned int addr;
	int m;

	addr = _sceKernelFindModuleByName("sceLoadExec");
	addr += 108;
	addr = *(unsigned int *) addr;

	m = _sceKernelGetModel();
	if (m == 3)
		m = 2;
	model = m;
	m ^= 4;

	MAKE_CALL(addr + ((m == 0) ? 0x2F28 : 0x2CD8), rebootex_callback);

	/* In PSP 3000, 0x00002D24 of module loadexec is as below:
	 
		0x00002D18: 0x02402021 '! @.' - addu       $a0, $s2, $zr
		0x00002D1C: 0x02A02821 '!(..' - addu       $a1, $s5, $zr
		0x00002D20: 0x02603021 '!0`.' - addu       $a2, $s3, $zr
		0x00002D24: 0x3C018860 '`..<' - lui        $at, 0x8860
		0x00002D28: 0x0020F809 '.. .' - jalr       $at, $ra
		0x00002D2C: 0x03C03821 '!8..' - addu       $a3, $fp, $zr

	   It is entrance to reboot.bin.
	   This line replaces entry to reboot.bin with 0x88FC0000, where
	   rebootex locates.
	*/
	_sw(0x3C0188FC, addr + ((m == 0) ? 0x2F74 : 0x2D24)); /* lui $at, 0x88FC */

	/* restore the effect of sceKernelPowerLock */
	_sw(0xACC24230, 0x8800CCB0); /* sw v0, 0x4230(a2) */
	_sw(0x0A003322, 0x8800CCB4); /* j 0x0800CC88 */
	_sw(0x00001021, 0x8800CCB8); /* addu $v0, $zr, $zr */
	_sw(0x3C058801, 0x8800CCBC); /* lui $a1, 0x8801 */

	func_rebootex = (void *) addr;

	_sceKernelIcacheInvalidateAll();
	_sceKernelDcacheWritebackInvalidateAll();

	return 0;
}

static void
ClearCaches(void)
{
	sceKernelIcacheInvalidateAll();
	sceKernelDcacheWritebackInvalidateAll();
}

int
main(void)
{
	char buf[168];
	pspUtilityHtmlViewerParam *param = (pspUtilityHtmlViewerParam *) buf;
	unsigned int *p = (unsigned int *) buf;
	SceUID sceuid;
	unsigned int intr;
	unsigned int* address_low = (unsigned int *) 0x08800000;
	unsigned int* address_high = (unsigned int *) 0x08800004;
	/* prototype of sceUtility_private_2DC8380C, scePower_driver_CE5D389B */
	int (*_scePowerUnregisterCallback)(int);
	/* prototype of sceUtility_private_764F5A3C, scePower_driver_1A41E0ED */
	void *(*_scePowerRegisterCallback)(int, SceUID);


	loginit();

	memset(buf, 0, 168);
	*p = 168;
	p += 4;
	*p = 19;

	sceUtilityHtmlViewerInitStart(param);
	sceKernelDelayThread(1000000);

	log("start...\n");

	/* search for "sceVshHV" */
	do {
		if ((*address_low == 0x56656373) && (*address_high == 0x56486873))
			break;
		address_low++;
		address_high++;
	} while (address_high < (unsigned int *) 0x0A000000);

	if (address_high == (unsigned int *) 0x0A000000)
		panic();

	log("find htmlviewer entry at %p\n", address_low);
	memset((void *) 0x08800000, 0, 0x00100000);
	_scePowerUnregisterCallback = (void*) ((unsigned int) address_low - 648U);
	log("power unregister at %p\n", _scePowerUnregisterCallback);
	_scePowerUnregisterCallback(0x08080000); /* it will save -1 to addr (p) so that scePowerRegisterCallback can move on */
	log("power unregister done\n");
	ClearCaches();

	p = (unsigned int *) 0x08800000;

	do {
		if (*p == 0xFFFFFFFF)
			break;
		p++;
	} while (p < (unsigned int *) 0x08900000);

	if (p == (unsigned int *) 0x08900000) /* panic if unregister fails */
		panic();

	log("find -1 at %p\n", p);
	sceuid = sceKernelCreateCallback("hen", 0, 0);
	_scePowerRegisterCallback = (void *) ((unsigned int) address_low - 624U);
	log("power register\n");
	/* this line stores 0/nop to 0x0880CCBC */
	_scePowerRegisterCallback((0x0880CCB0U -(unsigned int) p) >> 4, sceuid);
	log("power register done\n");
	ClearCaches();

	_sw((unsigned int) power_callback, 0x08800010);
	_sw(0x08800000, 0x08804234);
	ClearCaches();

	log("suspend intr\n");
	intr = sceKernelCpuSuspendIntr();
	sceKernelPowerLock(0, 0x08800000);
	sceKernelCpuResumeIntr(intr);
	log("resumed intr\n");

	sceKernelExitGame();
	sceKernelExitDeleteThread(0);

	return 1;
}
