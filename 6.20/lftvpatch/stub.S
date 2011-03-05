	.set noreorder

#include "pspstub.s"

	STUB_START "IoFileMgrForUser", 0x40010000, 0x00030005
	STUB_FUNC 0x109F50BC, sceIoOpen
	STUB_FUNC 0x42EC03AC, sceIoWrite
	STUB_FUNC 0x810C4BC3, sceIoClose
	STUB_END

	STUB_START "SystemCtrlForUser", 0x40090000, 0x00020005
	STUB_FUNC 0x62CAC4CF, sctrlPatchModule
	STUB_FUNC 0x2A868045, sctrlModuleTextAddr
	STUB_END
