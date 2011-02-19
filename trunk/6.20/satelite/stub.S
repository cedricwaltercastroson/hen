	.set noreorder

#include "pspstub.s"

	STUB_START "scePaf", 0x00090000, 0x00020005 
	STUB_FUNC 0x23C8DAB5, scePaf_memcpy
	STUB_FUNC 0xE8473E80, scePaf_sprintf
	STUB_END

	STUB_START "scePower", 0x40010011, 0x00030005 
	STUB_FUNC 0x0442D852, scePowerRequestColdReset
	STUB_FUNC 0x2B7C7CF4, scePowerRequestStandby
	STUB_FUNC 0xAC32C9CC, scePowerRequestSuspend
	STUB_END

	STUB_START "SystemCtrlForUser", 0x40090000, 0x00030005
	STUB_FUNC 0x2794CCF4, sctrlKernelExitVSH
	STUB_FUNC 0x1DDDAD0C, sctrlSESetConfig
	STUB_FUNC 0x16C3B7EE, sctrlSEGetConfig
	STUB_END

	STUB_START "KUBridge", 0x40090000, 0x00010005
	STUB_FUNC 0x24331850, kuKernelGetModel
	STUB_END

	STUB_START "VshCtrlLib", 0x40090000, 0x00020005
	STUB_FUNC 0xFD26DA72, vctrlVSHRegisterVshMenu
	STUB_FUNC 0xCD6B3913, vctrlVSHExitVSHMenu
	STUB_END

	STUB_START "sceDisplay", 0x40010000, 0x00030005
	STUB_FUNC 0xDEA197D4, sceDisplayGetMode
	STUB_FUNC 0xEEDA2E54, sceDisplayGetFrameBuf
	STUB_FUNC 0x984C27E7, sceDisplayWaitVblankStart
	STUB_END

	STUB_START "ThreadManForUser", 0x40010000, 0x00060005
	STUB_FUNC 0x278C0DF5, sceKernelWaitThreadEnd
	STUB_FUNC 0x446D8DE6, sceKernelCreateThread
	STUB_FUNC 0xF475845D, sceKernelStartThread
	STUB_FUNC 0x809CE29B, sceKernelExitDeleteThread
	STUB_FUNC 0x383F7BCC, sceKernelTerminateDeleteThread
	STUB_FUNC 0x71BC9871, sceKernelChangeThreadPriority
	STUB_END
