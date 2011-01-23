	.set noreorder

#include "pspstub.s"

	STUB_START "ModuleMgrForKernel", 0x00010000, 0x00010005
	STUB_FUNC 0x329C89DB, sceKernelLoadModuleVSH
	STUB_END
