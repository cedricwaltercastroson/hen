	.set noreorder

#include "pspstub.s"

	STUB_START "ModuleMgrForKernel", 0x00090011, 0x00070005
	STUB_FUNC 0x329C89DB, sceKernelLoadModuleVSH
	STUB_FUNC 0xB691CB9F, sceKernelLoadModuleForLoadExec
	STUB_FUNC 0xE3CCC6EA, sceKernelLoadModule
	STUB_FUNC 0xDF8FFFAB, sceKernelStartModule
	STUB_FUNC 0xAAFA90C2, sceKernelStopModule
	STUB_FUNC 0x9CEB18C4, sceKernelUnloadModule
	STUB_FUNC 0x5352C26C, sceKernelLoadModuleBuffer
	STUB_END

	STUB_START "SysMemForKernel", 0x00010011, 0x00090005
	STUB_FUNC 0xE5E18A99, sceKernelAllocPartitionMemory
	STUB_FUNC 0xFEB5C72B, sceKernelGetBlockHeadAddr
	STUB_FUNC 0x31DFE03F, sceKernelSetDdrMemoryProtection
	STUB_FUNC 0x864EBFD7, sceKernelGetModel
	STUB_FUNC 0xFE9BC18B, sceKernelDevkitVersion
	STUB_FUNC 0xC6A782DA, sceKernelCreateHeap
	STUB_FUNC 0x957A966D, sceKernelGetSystemStatus
	STUB_FUNC 0x96BFE779, sceKernelAllocHeapMemory
	STUB_FUNC 0xC2A8C491, sceKernelFreeHeapMemory
	STUB_END

	STUB_START "LoadCoreForKernel", 0x00010011, 0x00060005
	STUB_FUNC 0x77E8F516, sceKernelFindModuleByUID
	STUB_FUNC 0xBEF0A05E, sceKernelFindModuleByName
	STUB_FUNC 0x452C5524, sceKernelFindModuleByAddress
	STUB_FUNC 0xB0466E46, sceKernelCheckExecFile
	STUB_FUNC 0xB95FA50D, sceKernelProbeExecutableObject
	STUB_FUNC 0x8C712A03, sceKernelLinkLibraryEntries
	STUB_END

	STUB_START "LoadExecForKernel", 0x00090011, 0x00070005
	STUB_FUNC 0xAEFEF0F4, sceKernelExitVSHVSH
	STUB_FUNC 0x783EA19F, sceKernelLoadExecVSHMs1
	STUB_FUNC 0x254F01CB, sceKernelLoadExecVSHMs2
	STUB_FUNC 0xE09DAFDC, sceKernelLoadExecVSHMs3
	STUB_FUNC 0xEFA048F1, sceKernelLoadExecVSHMs4
	STUB_FUNC 0x73BAD4F8, sceKernelLoadExecVSHDisc
	STUB_FUNC 0xD364D324, sceKernelLoadExecVSHDiscUpdater
	STUB_END

