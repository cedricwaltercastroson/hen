#include <stdio.h>

#include "pspsdk.h"
#include "pspkernel.h"

#include "systemctrl.h"
#include "kubridge.h"

#include "log.h"

PSP_MODULE_INFO("lftv_patch_module", 0, 1, 0);

u32 (*send_request_original) (u32 a0, u32 a1, u32 a2, u32 a3, u32 t0, u32 t1, u32 t2, u32 t3) = NULL;

u32
send_request_patched(u32 a0, u32 a1, u32 a2, u32 a3, u32 t0, u32 t1, u32 t2, u32 t3)
{
	logint(a0);
	logint(a1);
	logint(a2);
	logint(a3);
	logint(t0);
	logint(t1);
	logint(t2);
	logint(t3);

	if (send_request_original == NULL) {
		send_request_original = (void *) sctrlModuleTextAddr("sceVshLftvMw_Module");
		if (send_request_original != 0)
			send_request_original += 0x000159C8;
		logstr("get send_request_original");
		logint((u32) send_request_original);
	}

	if (send_request_original) {
		return send_request_original(a0, a1, a2, a3, t0, t1, t2, t3);
	}

	return -1;
}

int
module_start(SceSize args, void* argp)
{
	sctrlSetModuleHook("sceVshLftvMw_Module", send_request_patched, 0x0000292C);
	return 0;
}

int
module_stop(SceSize args, void* argp)
{
	return 0;
}
