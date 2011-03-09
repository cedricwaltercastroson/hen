#include "pspstub.s"

STUB_START "sceUtility_private", 0x40010011, 0x00020005
STUB_FUNC 0x2DC8380C, scePowerUnregisterCallback
STUB_FUNC 0x764F5A3C, scePowerRegisterCallback
STUB_END

