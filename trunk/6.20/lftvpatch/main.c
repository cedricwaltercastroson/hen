#include <stdio.h>

#include "pspsdk.h"
#include "pspkernel.h"
#include "psptypes.h"

#include "systemctrl.h"
#include "kubridge.h"

#include "log.h"

#define MAKE_CALL(__f) \
	(((((unsigned int)__f) >> 2) & 0x03FFFFFF) | 0x0C000000)

#define MAKE_BREAK(n) ((((u32)n << 6) & 0x03FFFFC0) | 0x0000000D)

PSP_MODULE_INFO("lftv_patch_module", 0, 1, 0);



static u32
get_text_addr(u32 offset)
{
	static u32 g_text_addr = 0;

	if (g_text_addr == 0) {
		g_text_addr = sctrlModuleTextAddr("sceVshLftvMw_Module");
		if (g_text_addr == 0)
			return 0;
		logstr("text_addr:");
		logint(g_text_addr);
	}

	return g_text_addr + offset;
}

#define load_text_addr(__func, __offset, __ret) do {\
	if ((__func) == NULL) {\
		(__func) = (void *) get_text_addr((__offset));\
		if ((__func) == NULL)\
			return (__ret);\
	}\
} while(0)

u32
send_request_patched(u32 a0, u32 a1, u32 a2, char *a3, char *t0, char *t1, char *t2, u32 t3)
{
	static u32 (*send_request_original) (u32, u32, u32, char *, char *, char *, char *, u32) = NULL;
	u32 ret = -1;

	logint(a0);
	logint(a1);
	logint(a2);
	logstr(a3);
	logstr(t0);
	logstr(t1);
	logstr(t2);
	logint(t3);

	load_text_addr(send_request_original, 0x000159C8, ret);

	ret = send_request_original(a0, a1, a2, a3, t0, t1, t2, t3);
	logstr("request:");
	logint(ret);

	return ret;
}

u32
send_message_patched(u32 a0, u32 a1)
{
	static u32 (*send_message_original) (u32, u32) = NULL;

	u32 ret = -5;

	logint(a0);
	logint(a1);

	load_text_addr(send_message_original, 0x00016CB8, ret);

	ret = send_message_original(a0, a1);
	logstr("message:");
	logint(ret);

	return ret;
}

u32
parse_resp_patched(u32 a0, u32 a1, u32 a2)
{
	static u32 (*parse_resp_original) (u32, u32, u32) = NULL;

	u32 ret = -5;

	logint(a0);
	logint(a1);
	logint(a2);

	load_text_addr(parse_resp_original, 0x00016970, ret);

	ret = parse_resp_original(a0, a1, a2);
	logstr("parse resp:");
	logint(ret);

	return ret;
}

u32
send_message_http_patched(u32 a0, u32 a1, u32 a2)
{
	static u32 (*send_message_http_original) (u32, u32, u32) = NULL;
	u32 ret = 5;

	load_text_addr(send_message_http_original, 0x000163F8, ret);

	logint(_lb(a0 + 0x20));

	ret = send_message_http_original(a0, a1, a2);
	logstr("send_message_http:");
	logint(ret);

	return ret;
}

u32
sub_0001A58C_patched(u32 a0, u32 a1, u32 a2)
{
	static u32 (*sub_0001A58C_original) (u32, u32, u32) = NULL;
	u32 ret = -1;

	load_text_addr(sub_0001A58C_original, 0x0001A58C, ret);
	ret = sub_0001A58C_original(a0, a1, a2);
	logstr("sub_0001A58C:");
	logint(ret);

	return ret;
}


int
module_start(SceSize args, void* argp)
{
	sctrlPatchModule("sceVshLftvMw_Module", 0x24020000, 0x00033DA0); /* bypassing registration check */

	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_0001A58C_patched), 0x00016BC0);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(send_message_http_patched), 0x00016D6C);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(parse_resp_patched), 0x000165D8);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(parse_resp_patched), 0x00016DA8);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(send_message_patched), 0x00015D60);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(send_message_patched), 0x00015D84);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(send_request_patched), 0x0000292C);

	return 0;
}

int
module_stop(SceSize args, void* argp)
{
	return 0;
}
