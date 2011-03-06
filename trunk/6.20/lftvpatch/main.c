#include <stdio.h>

#include "pspsdk.h"
#include "pspkernel.h"

#include "systemctrl.h"
#include "kubridge.h"

#include "log.h"

#define MAKE_CALL(__f) \
	(((((unsigned int)__f) >> 2) & 0x03FFFFFF) | 0x0C000000)

PSP_MODULE_INFO("lftv_patch_module", 0, 1, 0);

u32 (*send_request_original) (u32 a0, u32 a1, u32 a2, char *a3, u32 t0, u32 t1, u32 t2, u32 t3) = NULL;

u32 g_text_addr = 0;

u32
send_request_patched(u32 a0, u32 a1, u32 a2, char *a3, u32 t0, u32 t1, u32 t2, u32 t3)
{
	u32 ret = -1;

	logint(a0);
	logint(a1);
	logint(a2);
	logstr(a3);
	logint(t0);
	logint(t1);
	logint(t2);
	logint(t3);

	if (send_request_original == NULL) {
		g_text_addr = sctrlModuleTextAddr("sceVshLftvMw_Module");
		if (g_text_addr != 0)
			send_request_original = (void *) (g_text_addr + 0x000159C8);
	}

	if (send_request_original) {
		ret = send_request_original(a0, a1, a2, a3, t0, t1, t2, t3);
		logstr("ret");
		logint(ret);
	}

	return ret;
}

int
module_start(SceSize args, void* argp)
{
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(send_request_patched), 0x0000292C);
	sctrlPatchModule("sceVshLftvMw_Module", 0x24020000, 0x00033DA0); /* bypassing registration check */

	return 0;
}

int
module_stop(SceSize args, void* argp)
{
	return 0;
}
