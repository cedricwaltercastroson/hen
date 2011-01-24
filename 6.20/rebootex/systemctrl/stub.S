	.set noreorder

#include "pspstub.s"

	STUB_START "ModuleMgrForKernel", 0x00010000, 0x00030005
	STUB_FUNC 0x329C89DB, sceKernelLoadModuleVSH
	STUB_FUNC 0xB691CB9F, sceKernelLoadModuleForLoadExec
	STUB_FUNC 0xE3CCC6EA, sceKernelLoadModule
	STUB_END

	STUB_START "SysMemForKernel", 0x00010000, 0x00030005
	STUB_FUNC 0x31DFE03F, sceKernelSetDdrMemoryProtection
	STUB_FUNC 0x864EBFD7, sceKernelGetModel
	STUB_FUNC 0xFE9BC18B, sceKernelDevkitVersion
	STUB_END
