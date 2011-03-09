#include <stdio.h>
#include <string.h>
#include "../sysctrl/psploadcore.h"
#include "pspsdk.h"
#include "pspkernel.h"
#include "psputility.h"
#include "psputilsforkernel.h"
#include "kgtext.h"

// Do you want to flash anything?
// 0 - full install
// 1 - partial install, only copy CFW modules
// 2 - fake install, no flash0 write
//#define INSTALL_MODE 0
#ifndef INSTALL_MODE
	#error "You have to set INSTALL_MODE to compile this."
#endif

int scePowerUnregisterCallback(u32);
void scePowerRegisterCallback(u32, SceUID);

PSP_MODULE_INFO("PermInst", 0, 1, 0);

extern unsigned char vshmain_start[];
extern int vshmain_size;

#if INSTALL_MODE < 2
#include "install.inc"
#endif

int kmode;

static int SetUserLevel(void)
{
	SceModule2 *mod;
	unsigned int *addr;

	SceModule2 * (*_sceKernelFindModuleByName)(char *) = (void *) 0x8801EB78;

// sctrlKernelSetUserLevel(4)
	mod = _sceKernelFindModuleByName("sceThreadManager");
	addr = (void*)(mod->text_addr + 0x00019E80);
	addr = (void*)*addr;
	addr[5] = 0xC0000000;

	kmode = 1;
	return 0;
}

static int CleanFunc(void)
{
	void (*_sceKernelIcacheInvalidateAll)(void) = (void *) 0x88000E98;
	void (*_sceKernelDcacheWritebackInvalidateAll)(void) = (void *) 0x88000744;

	/* restore the effect of scePowerRegisterCallback */
	_sw(0xACC24230, 0x8800CCB0); /* sw v0, 0x4230(a2) */
	_sw(0x0A003322, 0x8800CCB4); /* j 0x0800CC88 */
	_sw(0x00001021, 0x8800CCB8); /* addu $v0, $zr, $zr */
	_sw(0x3C058801, 0x8800CCBC); /* lui $a1, 0x8801 */

	_sceKernelIcacheInvalidateAll();
	_sceKernelDcacheWritebackInvalidateAll();

	kmode = 1;
	return 0;
}

int KernelFunc(void *func)
{
	int i;
	u32 kernel_entry, entry_addr;

	kernel_entry = (u32) func;
	entry_addr = ((u32) &kernel_entry) - 16;
	kmode = 0;
	i = sceKernelCpuSuspendIntr();
	sceKernelPowerLock(0, ((u32) &entry_addr) - 0x4234);
	sceKernelCpuResumeIntr(i);

	return kmode;
}

static void
ClearCaches(void)
{
	sceKernelIcacheInvalidateAll();
	sceKernelDcacheWritebackInvalidateAll();
}

u32 power_offset()
{
	u32 offset;
	SceUID sceuid = sceKernelCreateCallback("hen2", 0, 0);

	scePowerRegisterCallback(0, sceuid);
	scePowerRegisterCallback(0, sceuid);
	__asm__ volatile ("addiu $v0, $v1, 0;" : "=r"(offset));

	return offset;
}

void TextExit(char *text, int x, int y, int color)
{
	kgt_write(text, x, y, color);
	sceKernelDelayThread(4000*1000);
	KernelFunc(CleanFunc);
	sceKernelExitGame();
}

int main(void)
{
	int i, reinstall;
	SceUID sceuid;
	SceIoStat stat;
	u32 offset;
	static int power_buf[0x80000] __attribute__ ((aligned (16)));

	kgt_init();
	kgt_clearbox(0, 0, 480, 16, 0xFF0000);
	kgt_write("kgsws's installer", 164, 0, 0x00FFFF);

	kgt_write("checking FW version", 0, 16, 0x808080);
	i = sceKernelDevkitVersion();
	if(i != 0x06020010) {
		if(i < 0x06020010)
			kgt_write("To use this CFW you must update to 6.20 OFW.", 42, 128, 0xFFFF00);
		else
			kgt_write("To use this CFW you must downgrade to 6.20 OFW.", 29, 128, 0xFFFF00);
		TextExit("unsupported version", 207, 16, 0x0000FF);
		return 1;
	}
	kgt_write("OK", 207, 16, 0x00FF00);
	
	kgt_write("preparing kernel mode", 0, 32, 0x808080);
        offset = power_offset();
	memset(power_buf, 0, sizeof(power_buf));
	/* 0x78000000 + 0x88000000 will overflow, while addr of power_buf is left */
	scePowerUnregisterCallback((((u32) power_buf) >> 4) + 0x07800000U);
	ClearCaches();
	sceuid = sceKernelCreateCallback("hen", 0, 0);
	/* this line stores 0/nop to 0x8800CCBC */
	scePowerRegisterCallback((0xCCB0U - (offset & 0xFFFFFF)) >> 4, sceuid);
	ClearCaches();
	sceKernelDelayThread(1000 * 1000);
	kgt_write("OK", 207, 32, 0x00FF00);

	kgt_write("getting user level", 0, 48, 0x808080);
	if(!KernelFunc(SetUserLevel)) {
		TextExit("error", 207, 48, 0x0000FF);
		return 1;
	}
	kgt_write("OK", 207, 48, 0x00FF00);
	reinstall = 0;
#if INSTALL_MODE < 2
	kgt_write("getting flash0 access", 0, 65, 0x808080);
	if(sceIoUnassign("flash0:") < 0) {
		TextExit("failed to unassign flash0", 207, 64, 0x0000FF);
		return 1;
	}
	if(sceIoAssign("flash0:", "lflash0:0,0", "flashfat0:", IOASSIGN_RDWR, NULL, 0) < 0) {
		TextExit("failed to assign flash0", 207, 64, 0x0000FF);
		return 1;
	}
	sceuid = sceIoOpen("flash0:/vsh/module/vshorig.prx", PSP_O_RDONLY, 0777);
	if(sceuid >= 0) {
		sceIoClose(sceuid);
		reinstall = 1;
		kgt_write("CFW detected", 207, 64, 0xFF0000);
	} else kgt_write("OK", 207, 64, 0x00FF00);
	sceIoMkdir("flash0:/plugins", 0777);
	for(i = 0; i < NUMITEMS; i++) {
		kgt_write("creating", 0, 80, 0x808080);
		kgt_write(iitems[i].name, 81, 80, 0x800080);
		sceKernelDelayThread(100 * 1000);
		sceuid = sceIoOpen(iitems[i].path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
		if(sceuid < 0) {
			TextExit("failed", 207, 80, 0x00FF00);
			return 1;
		}
		offset = sceIoWrite(sceuid, iitems[i].buff, *iitems[i].size);
		sceIoClose(sceuid);
		if(offset != *iitems[i].size) {
			TextExit("failed", 207, 80, 0x00FF00);
			return 1;
		}
		kgt_write("DONE", 207, 80, 0x00FF00);
		sceKernelDelayThread(500 * 1000);
		kgt_clearbox(0, 80, 480, 16, 0);
	}
#else
	kgt_write("FAKE instalation", 0, 64, 0xFF0000);
#endif
#if INSTALL_MODE == 1
	kgt_write("creating vshorig.prx", 0, 80, 0x808080);
#else
	kgt_write("creating vshmain.prx", 0, 80, 0x808080);
#endif
	if(reinstall) // CFW detected, use original vshmain (required for brick recovery)
		sceuid = sceIoOpen("flash0:/vsh/module/vshorig.prx", PSP_O_RDONLY, 0777);
	else // new CFW install
		sceuid = sceIoOpen("flash0:/vsh/module/vshmain.prx", PSP_O_RDONLY, 0777);
	if(sceuid < 0) {
		TextExit("failed to open file", 207, 80, 0x0000FF);
		return 1;
	}
	offset = sceIoLseek(sceuid, 0, SEEK_END);
	sceIoLseek(sceuid, 0, SEEK_SET);
	i = sceIoRead(sceuid, (void*)0x08900000, offset);
	sceIoClose(sceuid);
#if INSTALL_MODE != 1
	if(vshmain_size != offset) {
		TextExit("wrong size", 207, 80, 0x0000FF);
		return 1;
	}
#endif
	if(i != offset) {
		TextExit("read error", 207, 80, 0x0000FF);
		return 1;
	}
#if INSTALL_MODE != 2
	if(!reinstall) { // do not create backup if there is one
		sceuid = sceIoOpen("flash0:/vsh/module/vshorig.prx", PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
		if(sceuid < 0) {
			TextExit("backup create error", 207, 80, 0x0000FF);
			return 1;
		}
		i = sceIoWrite(sceuid, (void*)0x08900000, offset);
		sceIoClose(sceuid);
		if(i != offset) {
			TextExit("backup write error", 207, 80, 0x0000FF);
			return 1;
		}
	}
#endif
#if INSTALL_MODE == 0
	sceuid = sceIoGetstat("flash0:/vsh/module/vshmain.prx", &stat);
	if(sceuid < 0) {
		TextExit("failed to access file", 207, 80, 0x0000FF);
		return 1;
	}
	stat.st_mode = 777;
        stat.st_attr = 0x0038;
	sceuid = sceIoChstat("flash0:/vsh/module/vshmain.prx", &stat, (FIO_S_IRWXU | FIO_S_IRWXG | FIO_S_IRWXO));
	if(sceuid < 0) {
		TextExit("failed to access file", 207, 80, 0x0000FF);
		return 1;
	}
	sceuid = sceIoOpen("flash0:/vsh/module/vshmain.prx", PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
	if(sceuid < 0) {
		TextExit("failed to rewrite file", 207, 80, 0x0000FF);
		return 1;
	}
#endif
#if INSTALL_MODE == 2
	sceuid = sceIoOpen("ms0:/vshmain.prx", PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
	if(sceuid < 0)
		sceuid = sceIoOpen("ef0:/vshmain.prx", PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
	if(sceuid < 0) {
		TextExit("failed to save file", 207, 80, 0x0000FF);
		return 1;
	}
#endif
#if INSTALL_MODE != 1
	i = sceIoWrite(sceuid, (void*)0x08900000, 0x150);
	if(i == 0x150) {
		i = sceIoWrite(sceuid, &vshmain_start[0x150], offset - 0x150);
	} else i = 0;
	sceIoClose(sceuid);
	if(i != offset - 0x150) {
#if INSTALL_MODE == 0
		// problem, original vshmain is now bad (=brick), lets try to fix it
		sceIoRemove("flash0:/vsh/module/vshmain.prx"); // maybe help?
		sceuid = sceIoOpen("flash0:/vsh/module/vshmain.prx", PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
		if(sceuid < 0) {
			// too bad, failed to fix brick
			TextExit("oh no, this is bad", 207, 80, 0x0000FF);
			return 1;
		}
		i = sceIoWrite(sceuid, (void*)0x08900000, offset);
		sceIoClose(sceuid);
		if(i != offset) {
			// too bad, failed to fix brick
			TextExit("oh no, this is bad", 207, 80, 0x0000FF);
			return 1;
		}
		// brick is fixed, but vshmain is not replaced
		TextExit("replace error", 207, 80, 0x0000FF);
		return 1;
#else
		// just failed to save to ms0/ef0
		TextExit("memory write error", 207, 80, 0x0000FF);
		return 1;
#endif
	}
#endif
	kgt_write("OK", 207, 80, 0x00FF00);

	kgt_write("everything done", 173, 192, 0x00FF00);
	TextExit("CFW will start now", 159, 208, 0x0080FF);
	return 1;
}
