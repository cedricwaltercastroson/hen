#include <stdio.h>
#include <string.h>

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

char *reg_table[] = {
	"zr", "at",
	"v0", "v1",
	"a0", "a1", "a2", "a3",
	"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
	"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
	"t8", "t9",
	"k0", "k1",
	"gp",
	"sp",
	"fp",
	"ra"
};

static u32
ator(char *r)
{
	u32 i;

	for (i = 0; i < sizeof(reg_table) / sizeof(char *); i++) {
		if (strcmp(r, reg_table[i]) == 0)
			return i;
	}

	return 0;
}

/* val - our hook func
 * addr - start addr of patch
 * r0   - the register to be set (usually a0/a1...)
 * r1   - the utility register (usually v0/v1...)
 */
static void
patch_load_value(u32 val, u32 addr_hi, u32 addr_lo, char *r0, char *r1)
{
	u32 hi, lo;

	/* hi is lui r1, val >> 16 */
	hi = 0x3C000000 | (ator(r1) << 16);
	hi |= val >> 16;

	/* lo is addiu r0, val & 0xFFFF */
	lo = 0x24000000 | (ator(r0) << 16);
	lo |= ator(r1) << 21;
	lo |= val & 0xFFFF;

	/* then write hi and lo to addr */
	sctrlPatchModule("sceVshLftvMw_Module", hi, addr_hi);
	sctrlPatchModule("sceVshLftvMw_Module", lo, addr_lo);
}

struct rtp_1 {
	void *func_table; //0x0 table at 0x000592A0
	char *data; //0x4
	u32 len; //0x8
	u8 flag; //0xC
	u32 offset; //0x10
};


static u32 g_payload_c = 0; /* set in 00007B7C */

static void
print_payload(void)
{
	u32 pc = g_payload_c;

	if (pc == 0) {
		logstr("XXXXXXXX");
		return;
	}
	logstr("payload:");
	logint(pc);
	pc = _lw(0x58+pc);
	if (pc == 0) {
		logstr("0x58");
		return;
	}
	pc = _lw(0x24+pc);
	if (pc == 0) {
		logstr("0x24");
		return;
	}
	pc = _lw(0xC+pc);
	if (pc == 0) {
		logstr("0xC");
		return;
	}
	logint(_lw(0xC+pc));
	logint(_lw(0x10+pc));
	logint(_lw(0x14+pc));
	logint(_lw(0x18+pc));
}

#if 0
// size 0x30
struct rtp_1_cc {
	void *func_table; //0x0
	u32 ; //0x4
	u32 ; //0x8
	u8 ; //0xC
	u8 ; //0xD
	u32 ; //0x10
	u8 ; //0x14
	u32 ; //0x18
	u32 ; //0x1C
	u8 ; //0x20
	struct rtp_1_c; //0x24
};

// size 0xC
struct rtp_1_c {
	u32 next??;//0x0
	u32 ;//0x4
	struct rtp_1 *;//0x8
};

#endif

/* for sub_00014C34 only */
static inline u32 sema_id(u32 a0)
{
	return _lw(0x4+(_lw(0x20+a0)));
}

static void
print_sema(u32 sid)
{
	SceKernelSemaInfo info;

	info.name[0] = '\0';

	if (sceKernelReferSemaStatus(sid, &info) < 0) {
		logstr("print_sema error");
		return;
	}

	logstr("sema:");
	logint(sid);
	//logint(info.numWaitThreads);
	//logint(info.initCount);
	//logint(info.currentCount);
	//logint(info.maxCount);
	logstr(info.name);
	logstr("sema done");
}

static u32 g_text_addr = 0;

static u32
get_text_addr(u32 offset)
{

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

#define load_text_addr2(__func, __offset) do {\
	if ((__func) == NULL) {\
		(__func) = (void *) get_text_addr((__offset));\
		if ((__func) == NULL)\
			return;\
	}\
} while(0)

#if 0
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
#endif

#if 0
u32
parse_desc_resp(u32 a0, u32 a1)
{
	static u32 (*func) (u32, u32) = NULL;
	u32 ret = 1;

	load_text_addr(func, 0x0000A44C, ret);
	ret = func(a0, a1);
	logstr("0x0000A44C:");
	logint(ret);

	return ret;
}

u32
parse_rtsp_resp(u32 a0, u32 a1, u32 a2)
{
	static u32 (*func) (u32, u32, u32) = NULL;
	u32 ret = 1;

	load_text_addr(func, 0x00009FA8, ret);
	ret = func(a0, a1, a2);
	logstr("0x00009FA8:");
	logint(ret);

	return ret;
}

u32
parse_rtsp_resp2(u32 a0, u32 a1)
{
	static u32 (*func) (u32, u32) = NULL;
	u32 ret = 1;

	load_text_addr(func, 0x00009ED0, ret);
	ret = func(a0, a1);
	logstr("0x00009ED0:");
	logint(ret);

	return ret;
}

u32
parse_rtsp_resp3(u32 a0, u32 a1)
{
	static u32 (*func) (u32, u32) = NULL;
	u32 ret = 1;

	load_text_addr(func, 0x000031A0, ret);
	ret = func(a0, a1);
	logstr("0x000031A0:");
	logint(ret);

	return ret;
}

u32
send_rtsp_req(u32 a0, u32 a1, u32 a2, u32 a3)
{
	static u32 (*func) (u32, u32, u32, u32) = NULL;
	u32 ret = 1;

	load_text_addr(func, 0x00002F94, ret);
	ret = func(a0, a1, a2, a3);
	logstr("0x00002F94:");
	logint(ret);

	return ret;
}

u32
sub_00004D54(u32 a0, u32 a1, u32 a2)
{
	static u32 (*func) (u32, u32, u32) = NULL;
	u32 ret = 1;

	load_text_addr(func, 0x00004D54, ret);
	ret = func(a0, a1, a2);
	logstr("0x00004D54:");
	logint(ret);

	return ret;
}

u32
sub_000054C8(u32 a0, u32 a1)
{
	static u32 (*func) (u32, u32) = NULL;
	u32 ret = 1;

	load_text_addr(func, 0x000054C8, ret);
	ret = func(a0, a1);
	logstr("0x000054C8:");
	logint(ret);

	return ret;
}

u32
sub_0000543C(u32 a0, u32 a1)
{
	static u32 (*func) (u32, u32) = NULL;
	u32 ret = 1;

	load_text_addr(func, 0x0000543C, ret);
	ret = func(a0, a1);
	logstr("0x0000543C:");
	logint(ret);

	return ret;
}
#endif

#if 0

u32
sub_00005758(u32 a0)
{
	static u32 (*func) (u32) = NULL;
	u32 ret = 1;

	load_text_addr(func, 0x00005758, ret);
	logstr("sub_00005758:");
	logint(_lw(0x18 + _lw(a0)));
	ret = func(a0);
	logstr("0x00005758:");
	logint(ret);

	return ret;
}

u32
sub_00005550(u32 a0, u32 a1)
{
	static u32 (*func) (u32, u32)= NULL;
	u32 ret = 0x15;

	load_text_addr(func, 0x00005550, ret);
	logstr("sub_00005550:");
	logint(a0);
	logint(a1);
	ret = func(a0, a1);
	logstr("0x00005550:");
	logint(ret);

	return ret;
}

u32
sub_0000604C(u32 a0)
{
	static u32 (*func) (u32) = NULL;
	u32 ret = 405;

	load_text_addr(func, 0x0000604C, ret);
	logstr("sub_0000604C:");
	logint(a0);
	ret = func(a0);
	logstr("0x0000604C:");
	logint(ret);

	return ret;
}
#endif

#if 0
u32
sub_00011E3C(u32 a0, u32 a1, u32 a2)
{
	static u32 (*func) (u32, u32, u32) = NULL;
	u32 ret = -1;

	load_text_addr(func, 0x00011E3C, ret);
	logstr("sub_00011E3C:");
	logint(_lb(0x2E + a0));
	logint(a2);
	logint(_lw(0x4 + _lw(_lw(0x34 + a0))));
	logint(_lw(0x8 + _lw(_lw(0x34 + a0))));
	ret = func(a0, a1, a2);
	logstr("0x00011E3C:");
	logint(ret);

	return ret;
}

u32
sub_000160AC(u32 a0, u32 a1, u32 a2)
{
	static u32 (*func) (u32, u32, u32) = NULL;
	u32 ret = -1;

	load_text_addr(func, 0x000160AC, ret);
	logstr("sub_000160AC:");
	ret = func(a0, a1, a2);
	logstr("0x000160AC:");
	logint(ret);

	return ret;
}

u32
sub_00011C8C(u32 a0, u32 a1)
{
	static u32 (*func) (u32, u32) = NULL;
	u32 ret = -1;

	load_text_addr(func, 0x00011C8C, ret);
	logstr("sub_00011C8C:");
	logint(_lw(0x4+_lw(_lw(0x30+a0))));
	ret = func(a0, a1);
	logstr("0x00011C8C:");
	logint(ret);

	return ret;
}
#endif

u32
sub_0000377C(u32 a0)
{
	static u32 (*func) (u32) = NULL;
	u32 myra;
	u32 ret = -1;

	__asm__ volatile ("addiu %0, $ra, 0;" : "=r"(myra));

	load_text_addr(func, 0x0000377C, ret);
	logstr("sub_0000377C:");
	logint(_lw(0x8+_lw(_lw(0x4+a0))));
	logint(myra);
	ret = func(a0);
	logstr("0x0000377C");
	logint(ret);

	return ret;
}

#if 0
u32
sub_0000127C(u32 a0)
{
	static u32 (*func) (u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x0000127C, ret);
	logstr("sub_0000127C:");
	logint(_lw(0x10+_lw(_lw(0x34+a0))));
	ret = func(a0);
	logstr("0x0000127C:");
	logint(ret);

	return ret;
}
#endif

#if 0
u32
sub_0000E494(u32 a0, u32 a1, u32 a2)
{
	static u32 (*func) (u32, u32, u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x0000E494, ret);
	logstr("sub_0000E494:");
	ret = func(a0, a1, a2);
	logstr("0x0000E494");
	logint(ret);

	return ret;

}

u32
sub_0000E420(u32 a0, u32 a1, u32 a2)
{
	static u32 (*func) (u32, u32, u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x0000E420, ret);
	logstr("sub_0000E420:");
	ret = func(a0, a1, a2);
	logstr("0x0000E420");
	logint(ret);

	return ret;

}

u32
sub_00002B44(u32 a0, u32 a1, u32 a2)
{
	static u32 (*func) (u32, u32, u32) = NULL;
	u32 myra;
	u32 ret;

	__asm__ volatile ("addiu %0, $ra, 0;" : "=r"(myra));

	ret = -1;
	load_text_addr(func, 0x00002B44, ret);
	logstr("sub_00002B44:");
	logint(myra);
	ret = func(a0, a1, a2);
	logstr("0x00002B44");
	logint(ret);

	return ret;

}

u32
sub_00041E9C(u32 a0, u32 a1, u32 a2)
{
	static u32 (*func) (u32, u32, u32) = NULL;
	u32 myra;
	u32 ret;

	__asm__ volatile ("addiu %0, $ra, 0;" : "=r"(myra));

	ret = -1;
	load_text_addr(func, 0x00041E9C, ret);
	logstr("sub_00041E9C:");
	logint(myra);
	ret = func(a0, a1, a2);
	logstr("0x00041E9C");
	logint(ret);

	return ret;
}
#endif

#if 0
u32
sub_00007818(u32 a0, u32 a1)
{
	static u32 (*func) (u32, u32) = NULL;
	u32 myra;
	u32 ret;

	__asm__ volatile ("addiu %0, $ra, 0;" : "=r"(myra));
	ret = 0x9;
	load_text_addr(func, 0x00007818, ret);
	logstr("sub_00007818:");
	logint(myra);
	logint(_lb(0x1A+a0));
	ret = func(a0, a1);
	logstr("0x00007818");
	logint(ret);

	return ret;
}

#endif

#if 0
u32
sub_00006A14(u32 a0, u32 a1, u32 a2)
{
	static u32 (*func) (u32, u32, u32) = NULL;
	u32 ret;

	ret = 0;
	load_text_addr(func, 0x00006A14, ret);
	if (a2 >= 512) {
		logstr("sub_00006A14:");
		logint(a2);
		logint(_lw(0x28+a0));
		logint(_lb(_lw(0x24+a0)));
	}
	ret = func(a0, a1, a2);
	if (a2 >= 512) {
		logstr("0x00006A14");
		logint(ret);
	}

	return ret;
}
#endif

static inline void
print_data(u32 a0)
{
	u32 tmp;
	struct rtp_1 *r;
	char *s;
	static u32 saved_a0 = 0;

	logstr("==print_data start");
	if (a0 == 0) {
		if (saved_a0 != 0)
			a0 = saved_a0;
		else
			goto out;
	} else {
		if (saved_a0 != a0)
			saved_a0 = a0;
	}

	tmp = _lw(0x1c+a0);
	if (tmp != 0) {
		tmp = _lw(0x8+tmp);
		logint(tmp);
		if (tmp != 0) {
			r = (void *) _lw(tmp+0x4);
			logint((u32) r);
			if (r != 0) {
				s = r->data;
				logint((u32) (r->data));
				if (s != 0) {
					logint(s[0]);
					logint(s[1]);
					logint(s[2]);
					logint(s[3]);
					logint(s[4]);
					logint(s[5]);
					logint(s[6]);
					logint(s[7]);
				}
			} else {
				logstr("r is NULL");
			}
		} else {
			logstr("tmp 2 is NULL");
		}
	} else {
		logstr("tmp is NULL");
	}

out:
	logstr("==print_data end");
}

u32
sub_000078BC(u32 a0)
{
	static u32 (*func) (u32) = NULL;
	//u32 myra;
	u32 ret;

	//__asm__ volatile ("addiu %0, $ra, 0;" : "=r"(myra));
	ret = 0;
	load_text_addr(func, 0x000078BC, ret);
	//print_data(a0);
	logstr("sub_000078BC:");
	//logint(_lw(a0));
	//if (_lw(a0))
	//	logint(_lw(0x1c+_lw(a0)));
	//logint(myra);
	ret = func(a0);
	//print_data(a0);
	logstr("0x000078BC:");
	logint(ret);

	return ret;
}


u32
sub_000087B4(u32 a0, u32 a1)
{
	static u32 (*func) (u32, u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x000087B4, ret);
	logstr("sub_000087B4:");
	//logint(_lw(_lw(0x4+a1)));
	//print_data(0);
	ret = func(a0, a1);
	//print_data(0);
	logstr("0x000087B4:");
	logint(ret);

	return ret;
}

u32
sub_00008AD0(u32 a0, u32 a1)
{
	static u32 (*func) (u32, u32) = NULL;
	u32 ret = 0;
	struct rtp_1 *r;
	char *p;
	u32 s0;

	load_text_addr(func, 0x00008AD0, ret);
	logstr("sub_00008AD0:");
	r = (void *) _lw(0x4+a1);
	logint((u32) r);
	p = r->data;
	logint(r->offset);
	logint(r->len);
	p += r->offset;
	logint((u32) p);
	logint(p[0]);
	logint(p[1]);
	logint(p[2]);
	logint(p[3]);
	logint(p[4]);
	logint(p[5]);
	logint(p[6]);
	logint(p[7]);
	//print_data(a0);
	//logint(_lb(0xD+a1));
	//s0 = _lw(a1);
	//logint(s0); //593A0
	//logint(_lw(0xC+s0)); //9C14
	//logint(_lw(0x14+s0)); //9974
	//logint(_lw(0x10+s0)); //9C5C
	//logint(_lw(0x8+s0)); //9168
	//g_payload_c = a0;
	ret = func(a0, a1);
	logstr("0x00008AD0:");
	print_payload();
	logint(ret);

	return ret;
}

u32
sub_00007B7C(u32 a0)
{
	static u32 (*func) (u32) = NULL;
	//u32 myra;
	u32 ret;
	//u32 s0;

	//__asm__ volatile ("addiu %0, $ra, 0;" : "=r"(myra));
	ret = 0;
	load_text_addr(func, 0x00007B7C, ret);
	logstr("sub_00007B7C:");
	g_payload_c = a0;
	//logint(myra);
	//logint(_lw(0x20+_lw(a0))); //0x00008AD0
	//print_data(a0);
	ret = func(a0);
	//print_data(a0);
	logstr("0x00007B7C:");
	//print_payload();
	logint(ret);

	return ret;
}

u32
sub_000094CC(u32 a0)
{
	static u32 (*func) (u32) = NULL;
	u32 ret = 0;
	struct rtp_1 *r;
	char *p;

	load_text_addr(func, 0x000094CC, ret);
	logstr("sub_000094CC:");
	logint(a0);
	r = (void *) _lw(0x4+a0);
	logint((u32) r);
	p = r->data;
	logint(r->offset);
	logint(r->len);
	p += r->offset;
	logint((u32) p);
	logint(p[0]);
	logint(p[1]);
	logint(p[2]);
	logint(p[3]);
	logint(p[4]);
	logint(p[5]);
	logint(p[6]);
	logint(p[7]);
	logint(_lb(0xD+a0));
	logint(_lb(0xC+a0));
	logint(_lw(0x8+_lw(_lw(0x4+a0)))); //7178
	logint(_lw(0x14+_lw(a0))); //9974
	ret = func(a0);
	logstr("0x000094CC:");
	logint(ret);

	return ret;
}

u32
sub_00009CA4(u32 a0)
{
	static u32 (*func) (u32) = NULL;
	u32 ret = 0;
	struct rtp_1 *r;
	char *p;

	load_text_addr(func, 0x00009CA4, ret);
	logstr("sub_00009CA4:");
	r = (void *) _lw(0x4+a0);
	p = r->data;
	logint(r->offset);
	logint(r->len);
	p += r->offset;
	logint((u32) p);
	logint(*(int *) p);
	logint(*(int *) (p + 4));
	logint(*(int *) (p + 8));
	ret = func(a0);
	logstr("0x00009CA4:");
	logint(ret);

	return ret;
}

u32
sub_00009240(u32 a0, u32 a1)
{
	static u32 (*func) (u32, u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x00009240, ret);
	logstr("sub_00009240:");
	//logint(_lb(0xD+a0));
	//logint(_lb(0xC+a0));
	//print_data(0);
	ret = func(a0, a1);
	//print_data(0);
	logstr("0x00009240:");
	logint(ret);

	return ret;
}


/* scevideoxxx */
u32
sub_0000D3D0(u32 a0, u32 a1)
{
	static u32 (*func) (u32, u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x0000D3D0, ret);
	logstr("sub_0000D3D0:");
	ret = func(a0, a1);
	logstr("0x0000D3D0:");
	logint(ret);

	return ret;
}

u32
sub_0000DE10(u32 a0, u32 a1)
{
	static u32 (*func) (u32, u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x0000DE10, ret);
	logstr("sub_0000DE10:");
	ret = func(a0, a1);
	logstr("0x0000DE10:");
	logint(ret);

	return ret;
}

typedef union {
	struct {
		unsigned short pad:3;
		unsigned short len:13;
	} s;
	unsigned short val;
} rtp_seg;

int
check_rtp_payload(struct rtp_1 *r)
{
	char *p;
	rtp_seg seg;
	int len;

	logint(r->len);
	logint(r->offset);
	if (r->len - r->offset < 2) {
		return 1;
	}
	p = r->data + r->offset;
	logint(*(int *) p);
	logint(*(int *) (p + 4));
	logint(*(int *) (p + 8));
	seg.val = (p[0] << 8) | p[1];
	r->offset += 2;
	len = seg.s.len;
	logint(len);
	if ((r->len - r->offset) <= len)
		return 2; // first return 8
	p = r->data + r->offset;
	seg.val = (p[0] << 8) | p[1];
	r->offset += len;
	len = seg.s.len;
	logint(len);
	if (r->len <= r->offset)
		return 3; // 2nd 8
	if ((r->len - r->offset) <= len)
		return 4; // first return 8
	r->offset += len;
	logint(r->offset);
	logint(r->len);
	if (r->len - r->offset < 2)
		return 0;
	else
		return -1;
}

u32
sub_00008D04(u32 a0, u32 a1, u32 a2, u32 a3)
{
	static u32 (*func) (u32, u32, u32, u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x00008D04, ret);
	logstr("sub_00008D04:");
	print_data(0);
	/* skip original call and do our check */
	//logint(check_rtp_payload((void *) a1));
	//ret = 0xC;
	ret = func(a0, a1, a2, a3);
	print_data(0);
	logstr("0x00008D04:");
	logint(ret);

	return ret;
}

void
sub_00006EA8(u32 a0, u32 a1, u32 a2)
{
	static void (*func) (u32, u32, u32) = NULL;

	load_text_addr2(func, 0x00006EA8);
	logstr("sub_00006EA8:");
	logint(a1);
	logint(a2);
	//logint(_lw(a1));
	func(a0, a1, a2);
	logstr("0x00006EA8");
	return;
}

u32
sub_00007178(u32 a0)
{
	static u32 (*func) (u32) = NULL;
	u32 ret = 0;
	struct rtp_1 *r;

	load_text_addr(func, 0x00007178, ret);
	logstr("sub_00007178:");
	r = (void *) a0;
	logint((u32) r->data);
	logint(_lb((u32) r->data));
	logint(r->len);
	logint(r->offset);
	ret = func(a0);
	logstr("0x00007178:");
	logint(ret);
	return ret;
}

u32
sub_00006FB4(struct rtp_1 *r1, struct rtp_1 *r2)
{
	static u32 (*func) (struct rtp_1 *, struct rtp_1 *) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x00006FB4, ret);
	logstr("sub_00006FB4:");
	logint((u32) r1);
	logint((u32) r2);

	logint((u32) (r2->data));
	logint(r2->len);
	logint(r2->offset);
	logint((r2->data)[0]);
	logint((r2->data)[1]);
	logint((r2->data)[2]);
	logint((r2->data)[3]);
	logint((r2->data)[4]);
	logint((r2->data)[5]);
	logint((r2->data)[6]);
	logint((r2->data)[7]);

	ret = func(r1, r2);
	logint((u32) r1->data);
	logint(r1->len);
	logint(r1->offset);
	logint((r1->data)[0]);
	logint((r1->data)[1]);
	logint((r1->data)[2]);
	logint((r1->data)[3]);
	logint((r1->data)[4]);
	logint((r1->data)[5]);
	logint((r1->data)[6]);
	logint((r1->data)[7]);

	logstr("0x00006FB4:");
	logint(ret);
	return ret;
}

u32
sub_000071AC(u32 a0, u32 a1, u32 a2)
{
	static u32 (*func) (u32, u32, u32) = NULL;
	struct rtp_1 *r;
	char *p;
	u32 ret = 0;

	load_text_addr(func, 0x000071AC, ret);
	//r = (void *) a0;
	logstr("sub_000071AC:");
	//logint(r);
	//print_data(0);
	ret = func(a0, a1, a2);
	//p = r->data;
	//logint(r->len);
	//logint(r->offset);
	//logint(p);
	//p += r->offset;
	//logint(p);
	//logint(p[0]);
	//logint(p[1]);
	//logint(p[2]);
	//logint(p[3]);
	//print_data(0);
	logstr("0x000071AC:");
	logint(ret);

	return ret;
}

u32
sub_00009330(u32 a0)
{
	static u32 (*func) (u32) = NULL;
	int ret = 0;
	u32 x;
	struct rtp_1 *r;
	char *p;

	load_text_addr(func, 0x00009330, ret);
	logstr("sub_00009330:");
	//print_data(0);
	x = _lw(0x24+a0);
	r = (void *) _lw(0x8+x);
	logint((u32) r->func_table);
	p = r->data;
	//logint(_lw(_lw(0x8+x))); //592A0
	logint(p[0]);
	logint(p[1]);
	logint(p[2]);
	logint(p[3]);
	logint(p[4]);
	logint(p[5]);
	logint(p[6]);
	logint(p[7]);
	x = _lw(g_text_addr + 0x0005DB14);
	logint(_lw(x)); //599C8
	ret = func(a0);
	//print_data(0);
	logstr("0x00009330:");
	logint(ret);
	return ret;
}

u32
sub_000072A4(u32 a0, u32 a1)
{
	static u32 (*func) (u32, u32) = NULL;
	u32 ret = 0;
	struct rtp_1 *r0, *r1;
	char *p;

	load_text_addr(func, 0x000072A4, ret);
	logstr("sub_000072A4:");
	r0 = (void *) a0;
	r1 = (void *) a1;
	ret = func(a0, a1);
	p = r0->data;
	logint(p[0]);
	logint(p[1]);
	logint(p[2]);
	logint(p[3]);
	logint(p[4]);
	logint(p[5]);
	logint(p[6]);
	logint(p[7]);
	logstr("0x000072A4:");
	logint(ret);
	return ret;
}

u32
sub_00009550(u32 a0)
{
	static u32 (*func) (u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x00009550, ret);
	logstr("sub_00009550:");
	ret = func(a0);
	logstr("0x00009550:");
	logint(ret);
	return ret;
}

/* wrapper of aes_decrypt */
u32
sub_0002124C(u32 a0, char *s1, u32 len, char *s2)
{
	static u32 (*func) (u32, char *, u32, char *) = NULL;
	u32 ret = 0;
	u32 *table;

	load_text_addr(func, 0x0002124C, ret);
	logstr("sub_0002124C:");
	//logint(a0);
	//logint(_lw(a0));
	//logint(_lw(0x4+a0));
	//logint(_lw(0x8+a0));
	//logint(_lw(0x1c+a0));
	//table = (void *)((_lw(0x1c+a0)<<4)+a0);
	//logint((u32) table);
	//logint(table[0x20/4]);
	//logint(table[0x24/4]);
	//logint(table[0x28/4]);
	//logint(table[0x2C/4]);
	logint(len);
	logint(_lw((u32) s1));
	logint(_lw(0x4 + ((u32) s1)));
	ret = func(a0, s1, len, s2);
	logstr("0x0002124C:");
	logint(_lw((u32) s2));
	logint(_lw(0x4 + ((u32) s2)));
	logint(ret);
	return ret;
}

// 0x000204C8
u32
sub_0002059C(u32 a0, u32 a1, u32 a2, u32 a3, u32 t0, u32 t1)
{
	static u32 (*func)(u32, u32, u32, u32, u32, u32) = NULL;
	u32 myra;
	u32 ret;

	__asm__ volatile ("addiu %0, $ra, 0;" : "=r"(myra));
	ret = 0;
	load_text_addr(func, 0x0002059C, ret);
	logstr("sub_0002059C:");
	logint(myra);
	logint(_lb(a3));
	logint(_lb(1+a3));
	logint(_lb(2+a3));
	logint(_lb(3+a3));
	logint(_lb(4+a3));
	logint(_lb(5+a3));
	logint(_lb(6+a3));
	logint(_lb(7+a3));
	ret = func(a0, a1, a2, a3, t0, t1);
	logstr("0x0002059C:");
	logint(ret);
	return ret;
}

/*
a1 is the aes KEY:
68109ADA
351FF0C3
E1E5D29A
ACC98A8B
*/
void
sub_000214E4(u32 a0, u32 a1)
{
	static void (*func)(u32, u32) = NULL;
	//int i;
	//char *p;
	u32 *p;
	int i, j;

	load_text_addr2(func, 0x000214E4);
	logstr("sub_000214E4:");
	i = _lw(0x1c+a0);
	i -= 6;
	logint(i);
	p = (u32 *) a1;
	for (j = 0; j < i; j++)
		logint(p[j]);
#if 0
	p = (char *) a1;
	for (i = 0; i < 32; i++) {
		logint(p[i]);
	}
#endif
	func(a0, a1);
	logstr("0x000214E4:");
	return;
}

u32
sub_00009C5C(u32 a0)
{
	static u32 (*func) (u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x00009C5C, ret);
	logstr("sub_00009C5C:");
	ret = func(a0);
	logstr("0x00009C5C:");
	logint(ret);
	return ret;
}

u32
sub_00009AA8(u32 a0, u32 a1)
{
	static u32 (*func) (u32, u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x00009AA8, ret);
	logstr("sub_00009AA8:");
	ret = func(a0, a1);
	logstr("0x00009AA8:");
	logint(_lw(a1));
	logint(ret);
	return ret;
}

u32
sub_00013F1C(u32 a0, u64 a1)
{
	static u32 (*func) (u32, u64) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x00013F1C, ret);
	logstr("sub_00013F1C:");
	print_sema(_lw(0x4+(_lw(0x4+a0))));
	//logint(a0);
	//logint((u32)a1);
	//logint((u32)(a1>>32));
	//logint(_lw(0x10+_lw(_lw(0x2c+a0)))); //called in sub_00013FE8 , 0x0000FA9C
	//logint(_lw(0x4+_lw(_lw(0x4+a0)))); //2473C
	//logint(_lw(0x8+_lw(_lw(0x4+a0)))); // 24704
	print_sema(_lw(0x4+_lw(0x1020+_lw(0x2c+a0))));
	ret = func(a0, a1);
	logstr("0x00013F1C:");
	//logint(ret);
	return ret;
}

u64
sub_000271CC(u64 a0, u32 a1, u32 a2)
{
	static u64 (*func) (u64, u32, u32) = NULL;
	u64 ret = 0;

	load_text_addr(func, 0x000271CC, ret);
	logstr("sub_000271CC:");
	logint((u32) a0);
	logint((u32) (a0 >> 32));
	logint(a1);
	logint(a2);
	ret = func(a0, a1, a2);
	logstr("0x000271CC:");
	logint((u32) ret);
	logint((u32) (ret >> 32));
	return ret;
}

u64
sub_00013F9C(u32 a0)
{
	static u64 (*func) (u32) = NULL;
	u64 ret = 0;

	load_text_addr(func, 0x00013F9C, ret);
	logstr("sub_00013F9C:");
	logint(a0);
	logint(_lw(a0));
	ret = func(a0);
	logstr("0x00013F9C:");
	logint((u32) ret);
	logint((u32) (ret >> 32));
	return ret;
}

u32
sub_00008F68(u32 a0)
{
	static u32 (*func) (u32) = NULL;
	u32 ret = 0;
	struct rtp_1 *r;
	int *p;

	load_text_addr(func, 0x00008F68, ret);
	logstr("sub_00008F68:");
	r = (void *) _lw(a0+0x4);
	logint((u32) r);
	if (r != NULL) {
		logint(r->len);
		logint(r->offset);
		logint((u32) (r->data));
		p = (void *) r->data;
		if (p) {
			logint(p[0]);
			logint(p[1]);
			logint(p[2]);
		}
	}
	ret = func(a0);
	logstr("0x00008F68:");
	logint(ret);
	return ret;
}

u32
sub_00014A98(u32 a0, u32 a1, u32 a2, u32 *a3)
{
	static u32 (*func) (u32, u32, u32, u32 *) = NULL;
	u32 ret = 0;
	u32 p;

	load_text_addr(func, 0x00014A98, ret);
	logstr("sub_00014A98:");
	logint(a0); // a0 of 0x00015270
	logint(a1);
	logint(a2);
	logint(_lw(a1));

	//p = _lw(0x24+a0); // a0 of sub_00015484
	//logint(_lw(p));
	//logint(_lw(0x4+p));
	//logint(_lw(0x8+p));
	//p = _lw(0xC+p); // watch me
	//print_sema(_lw(0x4+_lw(0x20+a0)));
	//print_sema(_lw(0x4+_lw(0x1C+a0)));
	//logint(_lb(0x19+a0));
	//logint(_lb(0x1A+a0));
	//logint(_lb(0x18+a0));
	//logint(_lw(0x1C+a0));
	logstr("functions:");
	logint(_lw(0x1C+_lw(a0)));
	logint(_lw(0x8+_lw(_lw(0x20+a0))));
	logint(_lw(0x4+_lw(_lw(0x1C+a0))));
	ret = func(a0, a1, a2, a3);
	logstr("0x00014A98:");
	logint(ret);
	//logint(_lw(0xC+p));
	//logint(_lw(0x4+p));
	//logint(_lw(0x8+p));
	logint(a3[0]);
	return ret;
}

u64
sub_00009168(u32 a0)
{
	static u32 (*func) (u32) = NULL;
	u32 myra;
	u64 ret = 0;
	struct rtp_1 *r;
	char *p;

	__asm__ volatile ("addiu %0, $ra, 0;" : "=r"(myra));
	load_text_addr(func, 0x00009168, ret);
	logstr("sub_00009168:");
	logint(myra);
	logint(_lw(0x14+_lw(a0))); //9974
	logint(_lw(0x8+_lw(_lw(0x4+a0)))); //7178
	r = (void *) _lw(0x4+a0);
	p = r->data + r->offset;
	logint(p[0]);
	logint(p[1]);
	logint(p[2]);
	logint(p[3]);
	logint(p[8]);
	logint(p[9]);
	logint(p[0xA]);
	logint(p[0xB]);
	ret = func(a0);
	logstr("0x00009168:");
	logint((u32) ret);
	logint((u32) (ret >> 32));
	return ret;
}

u64
sub_00007A44(u32 a0, u32 a1)
{
	static u64 (*func) (u32, u32) = NULL;
	u64 ret = 0;

	load_text_addr(func, 0x00007A44, ret);
	logstr("sub_00007A44:");
	logint(a1);
	logint(_lw(0x38+a0));
	ret = func(a0, a1);
	logstr("0x00007A44:");
	logint((u32) ret);
	logint((u32) (ret >> 32));
	return ret;
}

u64
sub_00007D7C(u32 a0, u64 a1)
{
	static u64 (*func) (u32, u64) = NULL;
	u64 ret = 0;

	load_text_addr(func, 0x00007D7C, ret);
	logstr("sub_00007D7C:");
	logint((u32) a1);
	logint((u32) (a1 >> 32));
	ret = func(a0, a1);
	logstr("0x00007D7C:");
	logint((u32) ret);
	logint((u32) (ret >> 32));
	return ret;
}

void
sub_00013D30(u32 a0, u64 a1)
{
	static void (*func) (u32, u64) = NULL;
	u32 tmp;

	load_text_addr2(func, 0x00013D30);
	logstr("sub_00013D30:");
	print_sema(_lw(0x4+_lw(0x4+a0)));
	logint(a0);
	logint((u32) a1);
	logint((u32) (a1 >> 32));
#if 0
	tmp = _lw(0x2C+a0);
	logint(tmp);
	if (tmp != 0) {
		tmp = _lw(0x1020+tmp);
		logint(tmp);
		if (tmp != 0) {
			tmp = _lw(tmp);
			logint(tmp);
		}
	}
#endif

#if 0
	logint(_lw(0x4+_lw(_lw(0x4+a0))));
	logint(_lw(0x8+_lw(_lw(0x4+a0))));
	if (_lw(0x28+a0) != 0)
		logint(_lw(0x8+_lw(_lw(0x28+a0))));
#endif
	func(a0, a1);
	logstr("0x00013D30:");
	return;
}

u32
sub_0000FA9C(u32 a0, u32 a1, u32 a2)
{
	static u32 (*func) (u32, u32, u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x0000FA9C, ret);
	logstr("sub_0000FA9C:");
	ret = func(a0, a1, a2);
	logstr("0x0000FA9C:");
	logint(ret);
	return ret;
}

/* video decode */
/* video data is at 0x0005DE98 */
u32
sub_0000D410(u32 a0)
{
	static u32 (*func) (u32) = NULL;
	//static u64 (*func2) (u64, u32, u32) = NULL;
	//u32 myra;
	u32 ret = 0;
	//u64 test, result;

	//__asm__ volatile ("addiu %0, $ra, 0;" : "=r"(myra));
	load_text_addr(func, 0x0000D410, ret);
	//load_text_addr(func2, 0x000271CC, ret);
	logstr("sub_0000D410:");

	//test = sceKernelGetSystemTimeWide();
	//logint((u32) test);
	//logint((u32) (test >> 32));
	//result = func2(test, 1000, 0);
	//logint((u32) result);
	//logint((u32) (result >> 32));
	//result = func2(test, 1000, 1000);
	//logint((u32) result);
	//logint((u32) (result >> 32));

	//logint(_lw(0x20+_lw(_lw(0x18+a0))));
	logint(_lw(0x34+_lw(_lw(0x18+a0)))); //0000ebec
	//logint(_lw(0x8+_lw(_lw(0x18+a0))));

	//print_sema(sema_id(_lw(0xC+a0))); // semaphore NetAVSynBufVideoDecodR
	//logint(myra);
	ret = func(a0);
	logstr("0x0000D410:");
	logint(ret);
	return ret;
}

u32
sub_00014C34(u32 a0, u32 a1, u32 a2, u32 a3)
{
	static u32 (*func) (u32, u32, u32, u32) = NULL;
	u32 myra;
	u32 ret = 0;

	__asm__ volatile ("addiu %0, $ra, 0;" : "=r"(myra));
	load_text_addr(func, 0x00014C34, ret);

	logstr("sub_00014C34:");
	logint(myra);
	//print_sema(_lw(0x4+_lw(0x20+a0)));
	//logint(a0);
	//logint(a1);
	//logint(a2);
	//logint(_lb(a0 + 0x18));
	//logint(_lb(a0 + 0x19));
	//logint(_lw(a0));
	//logint(_lw(0x20 + _lw(a0)));
	//logint(_lw(0x24 + _lw(a0)));
	//logint(_lw(0x4 + _lw(a0)));
	//logint(_lw(0x8 + _lw(_lw(0x1C + a0))));
	//logint(_lw(0x4 + _lw(_lw(0x20 + a0))));
	ret = func(a0, a1, a2, a3);
	logstr("0x00014C34:");
	logint(ret);
	logint(_lw(a3));
	logint(_lw(a1));

	return ret;
}


/* audio codec cb */
u32
sub_0001124C(u32 a0)
{
	static u32 (*func) (u32) = NULL;
	u32 myra;
	u32 ret = 0;

	//__asm__ volatile ("addiu %0, $ra, 0;" : "=r"(myra));
	load_text_addr(func, 0x0001124C, ret);
	logstr("sub_0001124C:");
	//logint(a0);
	//logint(myra);
	print_sema(sema_id(_lw(0xb4+a0))); // semaphore NetAVSynBufAudioDecodR
	ret = func(a0);
	logstr("0x0001124C:");
	//logint(ret);
	return ret;
}

u32
sub_00007D0C(u32 a0, u32 a1)
{
	static u32 (*func) (u32, u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x00007D0C, ret);
	logstr("sub_00007D0C:");
	logint(a1);
	ret = func(a0, a1);
	logstr("0x00007D0C:");
	logint(ret);
	return ret;
}

u32
sub_00015484(u32 a0, u32 a1, u32 a2)
{
	static u32 (*func) (u32, u32, u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x00015484, ret);
	logstr("sub_00015484:");
	logint(_lw(a0));
	logint(_lw(a0+0x4));
	logint(_lw(a0+0x8));
	logint(_lw(a0+0xC));
	logint(a1);
	logint(a2);
	ret = func(a0, a1, a2);
	logstr("0x00015484:");
	logint(ret);
	return ret;
}

u32
sub_00015270(u32 a0, u32 a1, u32 a2)
{
	static u32 (*func) (u32, u32, u32) = NULL;
	u32 myra;
	u32 ret = 0;

	__asm__ volatile ("addiu %0, $ra, 0;" : "=r"(myra));
	load_text_addr(func, 0x00015270, ret);
	logstr("sub_00015270:");
	ret = func(a0, a1, a2);
	logstr("0x00015270:");
	logint(ret);
	logint(myra);
	logint(a1);
	if (a2 >= 4)
		logint(_lw(a1));
	else
		logint(_lb(a1));
	return ret;
}

u32
sub_00006410(u32 a0, u32 a1)
{
	static u32 (*func) (u32, u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x00006410, ret);
	logstr("sub_00006410:");
	//g_payload_c = a0;
	ret = func(a0, a1);
	logstr("0x00006410:");
	logint(ret);
	print_payload();
	return ret;
}

u32
sub_00006730(u32 a0, u32 a1, u32 a2)
{
	static u32 (*func) (u32, u32, u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x00006730, ret);
	logstr("sub_00006730:");
	ret = func(a0, a1, a2);
	logstr("0x00006730:");
	logint(ret);
	logint(_lb(a0));
	print_payload();
	return ret;
}

u32 g_49a4 = 0;

void
sub_00006818(u32 a0)
{
	static void (*func) (u32) = NULL;
	u32 i;

	load_text_addr2(func, 0x00006818);
	logstr("sub_00006818:");
	func(a0);
	logstr("0x00006818:");
	logint(_lb(4+g_49a4));
	i = _lw(0x2c+g_49a4);
	if (i) {
		logint(i);
		if (_lw(0x10+i)) {
			logint(_lw(0x20+_lw(0x10+i)));
		}
		if ((i = _lw(0x18+i))) {
			logint(_lw(4+i));
			if ((i = _lw(i))) {
				logint(_lw(4+i));
				logint(_lw(8+i));
			}
		}
	}
	print_payload();
}

u32
sub_000049A4(u32 a0)
{
	static u32 (*func) (u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x000049A4, ret);
	logstr("sub_000049A4:");
	g_49a4 = a0;
	ret = func(a0);
	logstr("0x000049A4:");
	logint(ret);
	print_payload();
	return ret;
}

u32
sub_000020D0(u32 a0, u32 a1, u32 a2)
{
	static u32 (*func) (u32, u32, u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x000020D0, ret);
	logstr("sub_000020D0:");
	g_49a4 = a0;
	logint(_lw(0x18+a0));
	logint(_lw(0x4+_lw(_lw(0x18+a0))));
	logint(_lw(0x8+_lw(_lw(0x18+a0))));
	ret = func(a0, a1, a2);
	logstr("0x000020D0:");
	logint(ret);
	print_payload();
	return ret;
}

u32
sub_0000F0E8(u32 a0, u32 a1, u32 a2, u32 a3, u32 t0)
{
	static u32 (*func) (u32, u32, u32, u32, u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x0000F0E8, ret);
	logstr("sub_0000F0E8:");
	logint(_lw(a2));
	logint(_lw(4+a2));
	logint(_lw(8+a2));
	logint(a3);
	ret = func(a0, a1, a2, a3, t0);
	logstr("0x0000F0E8:");
	logint(ret);
	return ret;
}

u32
sub_0000EBEC(u32 a0, u32 a1, u32 a2)
{
	static u32 (*func) (u32, u32, u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x0000EBEC, ret);
	logstr("sub_0000EBEC:");
	logint(_lw(a1));
	logint(_lw(4+a1));
	logint(_lw(8+a1));
	logint(a2);
	ret = func(a0, a1, a2);
	logstr("0x0000EBEC:");
	logint(ret);
	return ret;
}

u32
sub_00020474(u32 a0)
{
	static u32 (*func)(u32) = NULL;
	u32 myra;
	u32 ret;
	char *p;

	__asm__ volatile ("addiu %0, $ra, 0;" : "=r"(myra));
	ret = 0;
	load_text_addr(func, 0x00020474, ret);
	logstr("sub_00020474:");
	logint(myra);
	p = (char *) a0 + 4;
	logint(p[0]);
	logint(p[1]);
	logint(p[2]);
	logint(p[3]);
	ret = func(a0);
	logstr("0x00020474:");
	logint(ret);
	return ret;
}

/*
 * 6410
 *     78BC
 *         87B4
 *         8D04
 *		       9330
 *		           204E8 // AES decrypt
 *	   7B7C
 *	       8AD0
 *	           9C14
 *	               94CC
 *	           9C5C
 *
 *             hdr D0D0D0D0: // audio?
 *	           9AA8
 *	           sub_00013F1C(a0)
 *	               sub_000140D8(a0)
 *	                   sub_00013F9C(a0)
 *	                       sub_000271CC()
 *	                   sub_000271CC    
 *	                   sub_00013FE8
 *
 *	           hdr E0E0E0E0: // vedio?
 *	           0x00009974
 *	           sub_00007D0C
 *	           sub_00014A98 // move data from a1 to a0
 *	               0x00015270
 *	                   sub_00015484 . data is moved from a1 to a0 ...
 *	                       memmove_reverse
 *	           0x00009168 // read and return frame len
 *	           sub_00007A44
 *	           sub_00007D7C
 *	               sub_00007EC8
 *	               sub_00007E84
 *	               sub_00006CF4
 *	           sub_00014A98
 *	           sub_00014A98
 *	           sub_00013D30
 *
 *	        0x0000993C
 */

u32
sub_000202F4(u32 a0, u32 a1, u32 a2)
{
	static u32 (*func) (u32, u32, u32) = NULL;
	u32 ret = 0;
	int *p;

	load_text_addr(func, 0x000202F4, ret);
	logstr("sub_000202F4:");
	p = (int *) (a0 + 4);
	logint(p[0]);
	logint(p[1]);
	logint(p[2]);
	logint(p[3]);
	ret = func(a0, a1, a2);
	logstr("0x000202F4:");
	logint(p[0]);
	logint(p[1]);
	logint(p[2]);
	logint(p[3]);
	logint(ret);
	return ret;
}

/* a0 is
 * 67452301 45230189 23018967 01896745
 *
 * a1 ?
 *
 * output is in a0, which is to be used in next routine
 *
 * a1 is copied to a0
 */
u32
sub_00023520(u32 a0, u32 a1, u32 a2)
{
	static u32 (*func) (u32, u32, u32) = NULL;
	u32 ret = 0;
	int *p;

	load_text_addr(func, 0x00023520, ret);
	logstr("sub_00023520:");
	p = (int *) a0;
	logint(p[0]);
	logint(p[1]);
	logint(p[2]);
	logint(p[3]);
	p = (int *) a1;
	logint(p[0]);
	logint(p[1]);
	logint(p[2]);
	logint(p[3]);
	ret = func(a0, a1, a2);
	logstr("0x00023520:");
	p = (int *) a0;
	logint(p[0]);
	logint(p[1]);
	logint(p[2]);
	logint(p[3]);
	logint(ret);
	return ret;
}

/* a0 is in/out
 * in nonce + 3...
 * out AES key
 *
 * simply MD5 to make the key!
 */
u32
sub_00022490(u32 a0)
{
	static u32 (*func) (u32) = NULL;
	u32 ret = 0;
	int *p;

	load_text_addr(func, 0x00022490, ret);
	logstr("sub_00022490:");
	p = (int *) a0;
	logint(p[0]);
	logint(p[1]);
	logint(p[2]);
	logint(p[3]);
	ret = func(a0);
	logstr("0x00022490:");
	logint(p[0]);
	logint(p[1]);
	logint(p[2]);
	logint(p[3]);
	logint(ret);
	return ret;
}

u32
sub_0000CC20(u32 a0)
{
	static u32 (*func) (u32) = NULL;
	u32 ret = 0;
	int *p;

	load_text_addr(func, 0x0000CC20, ret);
	logstr("sub_0000CC20:");
	ret = func(a0);
	logstr("0x0000CC20:");
	logint(ret);
	return ret;
}
//e30fef576f60f29f74b99661a64dc0f93B8C7905473EB41C7392BA718F084439
/* a1 is nonce + 3B8C...
 * returns a 32byte string to be fed to sub_00022490,
 * which makes the key
 */
u32
sub_0001B29C(u32 a0, u32 a1)
{
	static u32 (*func) (u32, u32) = NULL;
	u32 ret = 0;
	int *p;

	load_text_addr(func, 0x0001B29C, ret);
	logstr("sub_0001B29C:");
	logstr((char *) a1); /* a1 is nonce+ 3B8C7905473EB41C7392BA718F084439 */
	p = (int *) a0;
	logint(p[0]);
	logint(p[1]);
	logint(p[2]);
	logint(p[3]);
	logint(p[4]);
	logint(p[5]);
	logint(p[6]);
	logint(p[7]);
	ret = func(a0, a1);
	logstr("0x0001B29C:");
	//logint(ret);
	logint(p[0]);
	logint(p[1]);
	logint(p[2]);
	logint(p[3]);
	return ret;
}

u32
sub_0004C6E0(int *buf, char *s, int len)
{
	static u32 (*func) (int *, char *, int) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x0004C6E0, ret);
	logstr("sub_0004C6E0:");
	logstr(s);
	logint(buf[0]);
	logint(buf[1]);
	logint(buf[2]);
	logint(buf[3]);
	ret = func(buf, s, len);
	logstr("0x0004C6E0:");
	logint(buf[0]);
	logint(buf[1]);
	logint(buf[2]);
	logint(buf[3]);
	return ret;
}

u32
sub_00026F90(int *buf, int *a1)
{
	static u32 (*func) (int *, int *) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x00026F90, ret);
	logstr("sub_00026F90:");
	logint(buf[0]);
	logint(buf[1]);
	logint(buf[2]);
	logint(buf[3]);
	logint(a1[0]);
	logint(a1[1]);
	logint(a1[2]);
	logint(a1[3]);
	logint(a1[4]);
	ret = func(buf, a1);
	logstr("0x00026F90:");
	logint(buf[0]);
	logint(buf[1]);
	logint(buf[2]);
	logint(buf[3]);
	return ret;
}

/* make LFX_MSG "d=" */
u32
sub_00023584(u32 a0, u32 a1, u32 a2, u32 a3, u32 t0)
{
	static u32 (*func) (u32, u32, u32, u32, u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x00023584, ret);
	logstr("sub_00023584:");
	ret = func(a0, a1, a2, a3, t0);
	logstr("0x00023584:");
	logstr((char *) a2);
	return ret;
}

/* base64 encode */
u32
sub_00026EC0(u32 a0, u32 a1, u32 a2)
{
	static u32 (*func) (u32, u32, u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x00026EC0, ret);
	logstr("sub_00026EC0:");
	logint(a1);
	logint(a2);
	ret = func(a0, a1, a2);
	logstr("0x00026EC0:");
	logstr((char *)a0);
	return ret;
}

u32
sub_00039EDC(u32 a0)
{
	static u32 (*func) (u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x00039EDC, ret);
	logstr("sub_00039EDC:");
	ret = func(a0);
	logstr("0x00039EDC:");
	logint(ret);
	return ret;
}

u32
NetAVLfxMsgCb(u32 a0, u32 a1)
{
	static u32 (*func) (u32, u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x00012F54, ret);
	logstr("NetAVLfxMsgCb:");
	ret = func(a0, a1);
	logstr("NetAVLfxMsgCb ret:");
	logint(ret);
	return ret;
}

u32
NetAVLfxMsgHdlr(void)
{
	static u32 (*func) (void) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x000127A0, ret);
	logstr("NetAVLfxMsgHdlr:");
	ret = func();
	logstr("NetAVLfxMsgHdlr ret:");
	logint(ret);
	return ret;
}

u32
parse_lfx_msg(char *a0, u32 a1)
{
	static u32 (*func) (char *, u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x00012CE0, ret);
	logstr("parseLfx:");
	logint(a1); // pointer to lfx_msg_t
	ret = func(a0, a1);
	logstr("parseLfx ret:");
	logint(ret);
	return ret;
}

u32
sub_00041B10(u32 a0, u32 a1, u32 a2)
{
	static u32 (*func) (u32, u32, u32) = NULL;
	u32 myra;
	u32 ret;

	__asm__ volatile ("addiu %0, $ra, 0;" : "=r"(myra));
	ret = 0;
	load_text_addr(func, 0x00041B10, ret);
	logstr("sub_00041B10:");
	logint(myra);
	ret = func(a0, a1, a2);
	logstr("0x00041B10:");
	logint(ret);
	return ret;
}

u32
sub_0003FCFC(u32 a0, u32 a1, u32 a2)
{
	static u32 (*func) (u32, u32, u32) = NULL;
	u32 myra;
	u32 ret;

	//__asm__ volatile ("addiu %0, $ra, 0;" : "=r"(myra));
	ret = 0;
	load_text_addr(func, 0x0003FCFC, ret);
	logstr("sub_0003FCFC:");
	//logint(myra);
	logint(_lw(0xC+a1));
	logint(_lw(0x10+a1));
	logint(_lw(0x10+a1+0x4));
	ret = func(a0, a1, a2);
	logstr("0x0003FCFC:");
	logint(ret);
	return ret;
}

u32
sub_0003F934(u32 a0, u32 a1)
{
	static u32 (*func) (u32, u32) = NULL;
	u32 myra;
	u32 ret;
	int i, len, *p;

	//__asm__ volatile ("addiu %0, $ra, 0;" : "=r"(myra));
	ret = 0;
	load_text_addr(func, 0x0003F934, ret);
	//logstr("sub_0003F934:");
	//logint(myra);
	logstr("-");
	len = _lw(0xc+a1);
	p = (int *) (a1 + 0x10);
	for (i = 0; i < len/4; i++)
		logint(p[i]);
	ret = func(a0, a1);
	//logstr("0x0003F934:");
	//logint(ret);
	return ret;
}

u32
sub_000498E4(u32 a0, u32 a1, u32 a2)
{
	static u32 (*func) (u32, u32, u32) = NULL;
	u32 ret;
	int i, len, *p;

	ret = 0;
	load_text_addr(func, 0x000498E4, ret);
	logstr("sub_000498E4:");
	p = (int *) _lw(0xc+a0);
	len = _lw(0x10+a0);
	for (i = 0; i < len / 4; i++)
		logint(p[i]);
	ret = func(a0, a1, a2);
	logstr("0x000498E4:");
	logint(ret);
	return ret;
}

u32
sub_00012090(u32 a0, u32 a1, u32 a2, u32 a3, u32 t0)
{
	static u32 (*func) (u32, u32, u32, u32, u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x00012090, ret);
	logstr("sub_00012090:");
	//logint(_lw(_lw(0x34+a0))); // 0x59ab8
	//logint(_lw(0x4+_lw(_lw(0x34+a0)))); //2473C
	//logint(_lw(0x8+_lw(_lw(0x34+a0)))); //24704
	ret = func(a0, a1, a2, a3, t0);
	logstr("0x00012090:");
	logint(ret);
	return ret;
}

char *
__strncpy(char *dst, char *src, int n)
{
	logstr("strncpy");
	if (n != 0) {
		char *d = dst;
		char *s = src;

		do {
			if ((*d++ = *s++) == 0) {
				while (--n != 0)
					*d++ = 0;
				break;
			}
		} while (--n != 0);

		logstr(dst);
	}

	return dst;
}

u32
sub_0001264C(u32 a0, u32 a1, u32 a2, u32 a3, u32 t0, u32 t1)
{
	static u32 (*func) (u32, u32, u32, u32, u32, u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x0001264C, ret);
	logstr("sub_0001264C:");
	ret = func(a0, a1, a2, a3, t0, t1);
	logstr("0x0001264C:");
	logint(_lw(t1));
	logint(_lb(0x2c+a0));
	return ret;
}

/* parse decoded lfx_msg req from lftv */
u32
sub_00012448(u32 a0, u32 a1, u32 a2, u32 a3, u32 t0)
{
	static u32 (*func) (u32, u32, u32, u32, u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x00012448, ret);
	logstr("sub_00012448:");
	logint(_lw(0x8+_lw(_lw(0x30+a0))));
	ret = func(a0, a1, a2, a3, t0);
	logstr("0x00012448:");
	logint(ret);
	return ret;
}

/* get cseq number */
u32
sub_00013424(u32 a0)
{
	static u32 (*func) (u32) = NULL;
	u32 ret = 0;

	load_text_addr(func, 0x00013424, ret);
	logstr("sub_00013424:");
	logint(_lw(a0));
	logint(_lw(a0+4));
	logint(_lw(a0+8));
	ret = func(a0);
	logstr("0x00013424:");
	logint(ret);
	return ret;
}

// 0x0005D020 this is start of magic data
// sub_000394EC : make client "ALIVE" request

int
module_start(SceSize args, void* argp)
{
	//sctrlPatchModule("sceVshLftvMw_Module", 0x24020000, 0x00033DA0); /* bypassing registration check */

	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00013424), 0x00012538);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00012448), 0x00012738);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_0001264C), 0x0001284C);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_000498E4), 0x000498E4);
	//sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_0003F934, 0x00058E04);
	//sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_0003FCFC, 0x00058E20);
	//sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_00041B10, 0x00058E84);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00012090), 0x0001263C);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00012090), 0x00012748);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(__strncpy), 0x00012330);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(parse_lfx_msg), 0x0001206C);
	//patch_load_value((u32) NetAVLfxMsgHdlr, 0x00012978, 0x0001297C, "a1", "v0");
	//patch_load_value((u32) NetAVLfxMsgCb, 0x000130BC, 0x000130C8, "a1", "a3");
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00039EDC), 0x0003A3A8);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00014A98), 0x00013210);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00026EC0), 0x000235D4);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00023584), 0x00012BF8);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00023584), 0x00016904);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00026F90), 0x0004C7BC);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_0004C6E0), 0x0001B2C4);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_0001B29C), 0x0000CC78);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_0000CC20), 0x0000B8A0);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00022490), 0x00020310);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00023520), 0x00020308);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_000202F4), 0x0000299C);
	//sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_00020474, 0x000599D0);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_000020D0), 0x000049F4);
	//sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_000049A4, 0x000591D4);
	//sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_00015270, 0x0005972C);

	//sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_0000EBEC, 0x0005963C);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_BREAK(7), 0x0000D5CC);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_0000F0E8), 0x0000E4B8);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00006818), 0x000049B4);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00006730), 0x000068C8);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00006410), 0x000067AC);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00015484), 0x00015278);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00014A98), 0x00008CC0);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00007D0C), 0x00008BF8);
	//sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_0001124C, 0x00059698);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00014C34), 0x000112F0);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00014C34), 0x000112AC);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00014C34), 0x0001128C);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00014C34), 0x00011350);
#if 0
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00014C34), 0x0000D45C);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00014C34), 0x0000D47C);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00014C34), 0x0000D4F0);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00014C34), 0x0000D548);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00014C34), 0x0000D610);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00014C34), 0x0000DDC0);
#endif
	//sctrlPatchModule("sceVshLftvMw_Module", 0x2404FFF5, 0x0000D46C);
	//sctrlPatchModule("sceVshLftvMw_Module", 0x2404FFF6, 0x0000D48C);
	//sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_0000D410, 0x00059568);
	//sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_0000FA9C, 0x00059660);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00013D30), 0x00008CF4);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00007D7C), 0x00008C78);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00007A44), 0x00008C5C);
	
	//sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_00009168, 0x000059368);
	//sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_00009168, 0x000059388);
	//sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_00009168, 0x0000593A8);

	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00014A98), 0x00008C14);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00008F68), 0x00009954);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00013F9C), 0x000140E4);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_000271CC), 0x00013FD4);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_000271CC), 0x00014138);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00013F1C), 0x00008BD4);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00009AA8), 0x00008BB4);
	//sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_00009C5C, 0x0000593B0);
	//sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_00008AD0, 0x000059350);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_000094CC), 0x00009C24);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00009CA4), 0x00009C30);
	//sctrlPatchModule("sceVshLftvMw_Module", 0x24120001, 0x00009AA4);

	// aes init
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_0002059C), 0x0001FFFC);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_0002059C), 0x0002008C);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_0002059C), 0x0002014C);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_0002059C), 0x000201DC);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_0002059C), 0x000204C8);
	// aes init fini
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_000214E4), 0x00020674);


	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_000094CC), 0x0000948C);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00009550), 0x00009498);

	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_0002124C), 0x00020514);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_000072A4), 0x0000942C);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00009330), 0x00008E70);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_000094CC), 0x0000948C);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00008D04), 0x00008A64);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00009240), 0x00008E38);
	//sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_000087B4, 0x0005934C);
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_000071AC), 0x00008E84);
#if 0
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00006FB4), 0x00009304);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00007178), 0x00008DB0);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00007178), 0x00008E1C);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00006EA8), 0x00008E2C);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_0000DE10), 0x0000DA00);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_0000D3D0), 0x0000DECC);
	//sctrlPatchModule("sceVshLftvMw_Module", 0x00C09821, 0x00007C04);
	//sctrlPatchModule("sceVshLftvMw_Module", 0x0, 0x00007C00);
#endif
#if 0
	sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_000078BC, 0x00059310);
	sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_000078BC, 0x00059340);
	sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_000078BC, 0x000592E0);
#endif

	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00006A14), 0x000069C4);
#if 0
	sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_00007B7C, 0x000592DC);
	sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_00007B7C, 0x0005930C);
	sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_00007B7C, 0x0005933C);
#endif
#if 0
	sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_00007818, 0x00059338);
	sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_00007818, 0x000592D8);
	sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_00007818, 0x00059308);
#endif
#if 0
	sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_00002B44, 0x000590F0);
	sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_00041E9C, 0x00058EA0);
	sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_00002B44, 0x000590F0);
	sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_0000E420, 0x000595A4);
	sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_0000E494, 0x000595AC);
	sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_0000377C, 0x00059104);
#endif
	//sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_0000127C), 0x00002E84);
#if 0
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00011C8C), 0x00011F34);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_000160AC), 0x00011F08);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00011E3C), 0x00003820);
#endif

#if 0
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_0000604C), 0x00005570);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00014C34), 0x0000577C);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_00005758), 0x00004EA8);
	sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_00005550, 0x000591F8);
#endif

#if 0
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_000054C8), 0x00004E98);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_0000543C), 0x00004EC0);
	//sctrlPatchModule("sceVshLftvMw_Module", 0x0, 0x00002D7C);
	sctrlPatchModule("sceVshLftvMw_Module", (u32) sub_00004D54, 0x000591E8);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(send_rtsp_req), 0x00002D14);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(parse_rtsp_resp3), 0x00002D84);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(parse_rtsp_resp2), 0x00003208);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(parse_rtsp_resp), 0x00009F20);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(parse_desc_resp), 0x0000A10C);
#endif

#if 0
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(sub_0001A58C_patched), 0x00016BC0);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(send_message_http_patched), 0x00016D6C);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(parse_resp_patched), 0x000165D8);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(parse_resp_patched), 0x00016DA8);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(send_message_patched), 0x00015D60);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(send_message_patched), 0x00015D84);
	sctrlPatchModule("sceVshLftvMw_Module", MAKE_CALL(send_request_patched), 0x0000292C);
#endif

	return 0;
}

int
module_stop(SceSize args, void* argp)
{
	return 0;
}
