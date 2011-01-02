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

extern unsigned int size_rebootex_bin;
extern unsigned char rebootex_bin[];

#include "rebootex_bin.inc"


/* sub_000002B4 */
static int
rebootex_callback(unsigned int a1, unsigned int a2, unsigned int a3, 
		unsigned int a4, unsigned int a5)
{
	char *s, *s2;

	s = (char *) 0x88FC0000;
	s2 = (char *) (rebootex_bin + 0x77040000 + 0x88FC0000);
	while (s < (char *) 0x88FC71F0) {
		*s = *s2;
		s++;
		s2++;
	}

	s = (char *) 0x88FB0000;
	while (s < (char *) 0x88FB0100) {
		*s = 0;
		s++;
	}

	_sw(size_rebootex_bin, 0x88FB0004);
	_sw(model, 0x88FB0000);

	return func_rebootex(a1, a2, a3, a4, a5);
}

#define MAKE_CALL(a, f) _sw(0x0C000000 | (((u32)(f) >> 2) & 0x03FFFFFF), a)

/* sub_00000328 */
static int
power_callback(void)
{
	unsigned int (*f1)(char *) = (void *) 0x8801EB78; /* sceKernelFindModuleByName */
	int (*f2)(void) = (void *) 0x8800A1C4; /* sceKernelGetModel */
	void (*f3)(void) = (void *) 0x88000E98; /* sceKernelIcacheInvalidateAll */
	void (*f4)(void) = (void *) 0x88000744; /* sceKernelDcacheWritebackInvalidateAll */
	unsigned int addr;
	int m;

	addr = f1("sceLoadExec");
	addr += 108;
	addr = *(unsigned int *) addr;

	m = f2();
	if (m == 3)
		m = 2;
	model = m;
	m ^= 4;

	MAKE_CALL(addr + ((m == 0) ? 0x2F28 : 0x2CD8), rebootex_callback);
	_sw(0x3C0188FC, addr + ((m == 0) ? 0x2F74 : 0x2D24));
	_sw(0xACC24230, 0x8800CCB0);
	_sw(0x0A003322, 0x8800CCB4);
	_sw(0x00001021, 0x8800CCB8);
	_sw(0x3C058801, 0x8800CCBC);

	func_rebootex = (void *) addr;

	f3();
	f4();

	return 0;
}

static void
clear_cache(void)
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
	int (*f1)(int);
	/* prototype of sceUtility_private_764F5A3C, scePower_driver_1A41E0ED */
	void *(*f2)(int, SceUID);


	loginit();

	memset(buf, 0, 168);
	*p = 168;
	p += 4;
	*p = 19;

	sceUtilityHtmlViewerInitStart(param);
	sceKernelDelayThread(1000000);

	log("start...\n");

	do {
		if ((*address_low == 0x56656373) && (*address_high == 0x56486873))
			break;
		address_low++;
		address_high++;
	} while (address_high < (unsigned int *) 0x0A000000);

	if (address_high == (unsigned int *) 0x0A000000)
		panic();

	memset((void *) 0x08800000, 0, 0x00100000);
	f1 = (void*) ((unsigned int) address_low - 648U); /* sceUtility_private_2DC8380C */
	log("power unregister at %p\n", f1);
	f1(0x08080000);
	log("power unregister done\n");
	clear_cache();

	p = (unsigned int *) 0x08800000;

	do {
		if (*p == 0xFFFFFFFF)
			break;
		p++;
	} while (p < (unsigned int *) 0x08900000);

	if (p == (unsigned int *) 0x08900000) /* panic if unregister fails */
		panic();

	log("create callback\n");
	sceuid = sceKernelCreateCallback("hen", 0, 0);
	f2 = (void *) ((unsigned int) address_low - 624U);
	log("power register\n");
	f2((0x0880CCB0U -(unsigned int) p) >> 4, sceuid); /* sceUtility_private_764F5A3C */
	log("power register done\n");
	clear_cache();

	p = (unsigned int *) 0x08800010;
	*p = (unsigned int) power_callback;
	p = (unsigned int *) 0x08804234;
	*p = 0x08800000;
	clear_cache();

	log("suspend intr\n");
	intr = sceKernelCpuSuspendIntr();
	sceKernelPowerLock(0, 0x08800000);
	sceKernelCpuResumeIntr(intr);
	log("resumed intr\n");

	sceKernelExitGame();
	sceKernelExitDeleteThread(0);

	return 1;
}
