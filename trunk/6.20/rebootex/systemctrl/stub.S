	.set noreorder

#include "pspstub.s"

	STUB_START "ModuleMgrForKernel", 0x00010000, 0x00070005
	STUB_FUNC 0x329C89DB, sceKernelLoadModuleVSH
	STUB_FUNC 0xB691CB9F, sceKernelLoadModuleForLoadExec
	STUB_FUNC 0xE3CCC6EA, sceKernelLoadModule
	STUB_FUNC 0xDF8FFFAB, sceKernelStartModule
	STUB_FUNC 0xAAFA90C2, sceKernelStopModule
	STUB_FUNC 0x9CEB18C4, sceKernelUnloadModule
	STUB_FUNC 0x5352C26C, sceKernelLoadModuleBuffer
	STUB_END

	STUB_START "SysMemForKernel", 0x00010000, 0x00070005
	STUB_FUNC 0xE5E18A99, sceKernelAllocPartitionMemory
	STUB_FUNC 0xFEB5C72B, sceKernelGetBlockHeadAddr
	STUB_FUNC 0x31DFE03F, sceKernelSetDdrMemoryProtection
	STUB_FUNC 0x864EBFD7, sceKernelGetModel
	STUB_FUNC 0xFE9BC18B, sceKernelDevkitVersion
	STUB_FUNC 0xC6A782DA, sceKernelCreateHeap
	STUB_FUNC 0x957A966D, sceKernelGetSystemStatus
	STUB_END

	STUB_START "LoadCoreForKernel", 0x00010000, 0x00060005
	STUB_FUNC 0x77E8F516, sceKernelFindModuleByUID
	STUB_FUNC 0xBEF0A05E, sceKernelFindModuleByName
	STUB_FUNC 0x452C5524, sceKernelFindModuleByAddress
	STUB_FUNC 0xB0466E46, sceKernelCheckExecFile
	STUB_FUNC 0xB95FA50D, sceKernelProbeExecutableObject
	STUB_FUNC 0x8C712A03, sceKernelLinkLibraryEntries
	STUB_END
