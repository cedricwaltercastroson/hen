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
	/* XXX what is 0x77040000 ??? */
	memcpy((void *) 0x88FC0000, rebootex_bin + 0x77040000 + 0x88FC0000, size_rebootex_bin);
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

	/* In PSP 3000, 0x00002D24 of module loadexec is as below:
	 
		0x00002D18: 0x02402021 '! @.' - addu       $a0, $s2, $zr
		0x00002D1C: 0x02A02821 '!(..' - addu       $a1, $s5, $zr
		0x00002D20: 0x02603021 '!0`.' - addu       $a2, $s3, $zr
		0x00002D24: 0x3C018860 '`..<' - lui        $at, 0x8860
		0x00002D28: 0x0020F809 '.. .' - jalr       $at, $ra
		0x00002D2C: 0x03C03821 '!8..' - addu       $a3, $fp, $zr

	   It is entrance to reboot.bin.
	   This line replaces entry to reboot.bin with 0x88FC0000, which
	   rebootex locates.
	*/
	_sw(0x3C0188FC, addr + ((m == 0) ? 0x2F74 : 0x2D24)); /* lui $at, 0x88FC */

	/* Somehow prxtool doesn't work with sysmem.prx =(
	 * But lucky that Davee posted on his blog:

		0x0000CCB0: 0xACC205B0 '....' - sw         $v0, 1456($a2)
		0x0000CCB4: 0x08003322 '"3..' - j          loc_0000CC88
		0x0000CCB8: 0x00001021 '!...' - move       $v0, $zr
 
		; ======================================================
		; Subroutine sceKernelPowerLockForUser - Address 0x0000CCBC - Aliases: sceKernelPowerLock
		; Exported in sceSuspendForKernel
		; Exported in sceSuspendForUser
		sceKernelPowerLockForUser:
			0x0000CCBC: 0x3C050000 '...<' - lui        $a1, 0x0
			0x0000CCC0: 0x8CA305B4 '....' - lw         $v1, 1460($a1)
			0x0000CCC4: 0x27BDFFF0 '...'' - addiu      $sp, $sp, -16
			0x0000CCC8: 0xAFBF0000 '....' - sw         $ra, 0($sp)
			0x0000CCCC: 0x14600004 '..`.' - bnez       $v1, loc_0000CCE0
			0x0000CCD0: 0x00001021 '!...' - move       $v0, $zr
			0x0000CCD4: 0x8FBF0000 '....' - lw         $ra, 0($sp)
			 
	   So the code below is to swap code from CCB0 to CCBC.
	*/
	_sw(0xACC24230, 0x8800CCB0); /* sw v0, 0x4230(a2) */
	_sw(0x0A003322, 0x8800CCB4); /* j 0x02003322 */
	_sw(0x00001021, 0x8800CCB8); /* addu $v0, $zr, $zr */
	_sw(0x3C058801, 0x8800CCBC); /* lui $a1, 0x8801 */

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

	/* search for "sceVshHV" */
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

	_sw((unsigned int) power_callback, 0x08800010);
	_sw(0x08800000, 0x08804234);
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
