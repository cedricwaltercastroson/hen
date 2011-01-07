#include "psptypes.h"

#define REBOOT_BASE 0x88600000

#define MAKE_CALL(__f) \
	(((((unsigned int)__f) >> 2) & 0x03FFFFFF) | 0x0C000000)

#define SAVE_VALUE(__a, __v) do {\
	_sw((__v), (__a) + REBOOT_BASE);\
} while (0)

#define SAVE_CALL(__a, __f) do {\
	SAVE_VALUE((__a), MAKE_CALL(__f));\
} while (0)


/* global variables */

#define HEN_STR "/hen.prx"
#define RTM_STR "/rtm.prx"
#define INIT_STR "/kd/init.prx"

unsigned int model0[] = {
	0x000082AC, 0x00008420, 0x000083C4, 0x0000565C,
	0x000026DC, 0x0000274C, 0x00002778, 0x000070F0,
	0x00003798, 0x0000379C, 0x000026D4, 0x00002728,
	0x00002740, 0x00005550, 0x00005554, 0x00005558,
	0x00007388
}; /* ref 0x88FC0940 */

unsigned int model1[] = {
	0x00008374, 0x000084E8, 0x0000848C, 0x00005724,
	0x000027A4, 0x00002814, 0x00002840, 0x000071B8,
	0x00003860, 0x00003864, 0x0000279C, 0x000027F0,
	0x00002808, 0x00005618, 0x0000561C, 0x00005620,
	0x00007450
}; /* ref 0x88FC0984 */

/* all rebootX come from reboot.bin */
int (*Real_Reboot)(void *, void *, void *, void *) = (void *) REBOOT_BASE; /* ref 0x88FC09C8 */
void (*DcacheClear)(void) = (void *) 0x88600938; /* ref 0x88FC09CC */
void (*IcacheClear)(void) = (void *) 0x886001E4; /* ref 0x88FC09D0 */

int (*sceBootLfatRead)(void *, void *); /* ref 0x88FC7210 */
int (*sceBootLfatOpen)(char *); /* ref 0x88FC71F0 */
int (*sceBootLfatClose)(void); /* ref 0x88FC71F4 */
unsigned int (*reboot6)(unsigned char *, unsigned int); /* ref 0x88FC721C */

int (*func1)(void *, unsigned int, void *); /* ref 0x88FC7200 */
int (*func2)(unsigned char *, unsigned int); /* ref 0x88FC7218 */

int has_hen_prx; /* ref 0x88FC7214 */
int has_rtm_prx; /* ref 0x88FC71F8 */

char *rtm_init; /* ref 0x88FC71FC */
void *rtm_addr; /* ref 0x88FC7204 */
unsigned int rtm_len; /* ref 0x88FC720C */
unsigned int rtm_op; /* ref 0x88FC7208 */

static unsigned int size_sysctrl_bin;
static unsigned char sysctrl_bin[];

#include "sysctrl_bin.inc"

/* util functions */

static int __attribute__((noinline))
__memcmp(const char *s1, const char *s2, int len)
{
	int i;

	if (len <= 0)
		return 0;

	for (i = 0; i < len; i++) {
		if (s1[i] != s2[i])
			return s1[i] - s2[i];
	}

	return 0;
}

static void __attribute__((noinline))
__memset(void *p, char c, int len)
{
	int i;
	char *s;

	if (len <= 0)
		return;

	for (i = 0, s = p; i < len; i++)
		s[i] = c;

	return;
}

static void __attribute__((noinline))
__memcpy(void *dst, void *src, int len)
{
	int i;
	char *d, *s;

	if (len <= 0)
		return;

	d = dst;
	s = src;
	for (i = 0; i < len; i++, d++, s++)
		*d = *s;

	return;
}

static int __attribute__((noinline))
__strlen(char *s)
{
	int len = 0;

	while (*s) {
		len++;
		s++;
	}

	return len;
}

/* 0x88FC00D4 */
void __attribute__((noinline))
ClearCaches(void)
{
	DcacheClear();
	IcacheClear();
}

/* 0x88FC0100 */
int __attribute__((noinline))
sceBootLfatReadPatched(void *a0, void *a1)
{
	if (has_hen_prx) {
		__memcpy((void *) 0x88FC0000, sysctrl_bin, size_sysctrl_bin);
		return size_sysctrl_bin;
	}
	if (has_rtm_prx) {
		__memcpy((void *) 0x88FC0000, rtm_addr, rtm_len);
		return rtm_len;
	}
	return sceBootLfatRead(a0, a1);
}

/* 0x88FC0188 */
int __attribute__((noinline))
sceBootLfatOpenPatched(char *s)
{
	if (!__memcmp(s, HEN_STR, 9)) {
		has_hen_prx = 1;
		return 0;
	}
	if (!__memcmp(s, RTM_STR, 9)) {
		has_rtm_prx = 1;
		return 0;
	}
	return sceBootLfatOpen(s);
}

/* 0x88FC021C */
int __attribute__((noinline))
sceBootLfatClosePatched(void)
{
	if (has_hen_prx) {
		has_hen_prx = 0;
		return 0;
	}
	if (has_rtm_prx) {
		has_rtm_prx = 0;
		return 0;
	}
	return sceBootLfatClose();
}

/* 0x88FC025C */
int __attribute__((noinline))
sub_88FC025C(void *a0, unsigned int a1, void *a2)
{
	unsigned int len;

	/* a0 is beginning of systemctrl. offset 304 is 0xB301AEBAU */
	if (_lw(a0 + 304) == 0xB301AEBAU) {
		len = _lw(a0 + 176);
		__memcpy(a0, a0 + 336, len);
		_sw(len, a2);
		return 0;
	}

	return func1(a0, a1, a2);
}

/* 0x88FC02C8 */
int __attribute__((noinline))
sub_88FC02C8(unsigned char *s, unsigned int a1)
{
	int i;

	for (i = 0; i < 88; i++) {
		if (s[i + 212] != 0)
			return func2(s, a1);
	}

	return 0;
}

/* 0x88FC0304 */
int __attribute__((noinline))
PatchLoadCore(void *a0, void *a1, void *a2, int (*module_start)(void *, void *, void*))
{
	u32 text_addr = (u32) module_start;
	unsigned int f1 = MAKE_CALL(sub_88FC025C);
	unsigned int f2 = MAKE_CALL(sub_88FC02C8);

	_sw(f1, text_addr + 13792);
	_sw(f1, text_addr + 23852);
	_sw(f2, text_addr + 23888);
	_sw(f2, text_addr + 23936);
	_sw(f2, text_addr+ 24088);

	func1 = (void *) (text_addr + 30640);
	func2 = (void *) (text_addr + 30616);

	ClearCaches();

	return module_start(a0, a1, a2);
}

unsigned int sub_88FC0890(void *, void *) __attribute__((noinline));

int __attribute__((noinline))
main(void *a0, void *a1, void *a2, void *a3)
{
	unsigned int *pf;

	if (_lw(0x88FB0000) == 0)
		pf = model0;
	else
		pf = model1;

	SAVE_CALL(pf[4], sceBootLfatOpenPatched); /* replace sceBootLfatOpen */
	SAVE_CALL(pf[5], sceBootLfatReadPatched); /* replace sceBootLfatRead */
	SAVE_CALL(pf[6], sceBootLfatClosePatched); /* replace sceBootLfatClose */
	SAVE_CALL(pf[7], sub_88FC0890); /* replace reboot6 */

	/* mask sub_88603798 of reboot */
	SAVE_VALUE(pf[8], 0x03E00008); /* jr ra */
	SAVE_VALUE(pf[9], 0x24020001); /* addiu $v1, $zr, 1 */

	/* mask some condition branch in sub_88602670 of reboot */
	SAVE_VALUE(pf[10], 0); /* nop */
	SAVE_VALUE(pf[11], 0); /* nop */
	SAVE_VALUE(pf[12], 0); /* nop */

	/* before return from sub_88605430 of reboot, call our PatchLoadCore */
	SAVE_VALUE(pf[13], 0x00113821); /* addu $a3, $zr, $s1 */
	SAVE_VALUE(pf[14], (((((unsigned int) PatchLoadCore) & 0x0FFFFFFC) >> 2) | 0x08000000)); /* j PatchLoadCore */
	SAVE_VALUE(pf[15], 0x02A0E821); /* addu $sp, $zr, $s5 */
	SAVE_VALUE(pf[16], 0); /* nop */

	sceBootLfatOpen = (void *) (pf[0] | REBOOT_BASE);
	sceBootLfatRead = (void *) (pf[1] | REBOOT_BASE);
	sceBootLfatClose = (void *) (pf[2] | REBOOT_BASE);
	reboot6 = (void *) (pf[3] | REBOOT_BASE);

	rtm_init = *(char **) 0x88FB0010;
	rtm_addr = *(void **) 0x88FB0014;
	rtm_len = _lw(0x88FB0018);
	rtm_op = _lw(0x88FB001C);

	has_hen_prx = 0;
	has_rtm_prx = 0;

	ClearCaches();

	return Real_Reboot(a0, a1, a2, a3);
}

/* XXX probably BtcnfHeader */
typedef struct {
	char pad[16];
	unsigned int o16; /* offset 16 */
	unsigned int o20; /* offset 20 */
	char pad2[8];
	unsigned int o32; /* offset 32 */
	unsigned int o36; /* offset 36 */
	char pad3[8];
	unsigned int o48; /* offset 48 */
	unsigned int o52; /* offset 52 */
} __attribute__((packed)) uk_t;

/* XXX probably ModuleEntry */
typedef struct {
	unsigned int ozr; /* offset 0 */
	unsigned int pad;
	unsigned short o8; /* offset 8 */
	unsigned char o10; /* offset 10 */
	unsigned char o11; /* offset 11 */
	char pad2[20];
} __attribute__((packed)) uk2_t;
/*
typedef struct {
        u32 name;
        u32 unk0;
        u32 flags;
        u32 unk1;
        u8  hash[16];
}ModuleEntry;
*/


/* 0x88FC0604 */
int __attribute__((noinline))
sub_88FC0604(unsigned char *a0, unsigned char *a1, unsigned char *a2, unsigned int a3)
{
	uk2_t uk2;
	uk2_t *puk2;
	uk_t *uk = (uk_t *) a0;
	unsigned int len, i, len2;
	unsigned char *p0, *p1, *p2, *p;
	unsigned char *buf;

	p0 = a0 + uk->o32;
	p1 = a0 + uk->o48;
	p2 = a0 + uk->o52;

	len2 = __strlen(a2) + 1;
	__memcpy(p2, a2, len2);
	uk->o52 += len2;

	if ((int) uk->o36 <= 0)
		return -2;

	puk2 = (uk2_t *) p0;
	len = __strlen(a1) + 1;

	for (i = 0; i < uk->o36; i++) {
		if (!__memcmp(p1 + puk2->ozr, a1, len))
			break;
		puk2++;
	}
	if (i == uk->o36)
		return -2;

	__memset(&uk2, 0, sizeof(uk2_t));
	uk2.ozr = p2 - p1;
	uk2.o8 = (unsigned short) a3;
	uk2.o11 = -128;
	uk2.o10 = 1;

	len = (uk->o36 - i) << 5;
	len += len2;
	len += p2 - p1;

	p = p0 + (i << 5);
	/* XXX alloca can be replaced by memmove */
	buf = alloca(len); /* allocation on stack. gcc built-in inline function */
	__memcpy(buf, p, len);
	__memcpy(p0 + ((i + 1) << 5), buf, len);
	__memcpy(p, &uk2, sizeof(uk2_t));

	uk->o36 += 1;
	uk->o48 += 32;
	uk->o52 += 32;

	if ((int) uk->o20 <= 0)
		return -3;

	for (i = 0; i < uk->o20; i++) {
#if 0
		unsigned short *tmp = (unsigned short *) (a0 + uk->o16 + (i << 5));

		(*tmp)++;
#endif
		unsigned int v0, v1;

		p = a0 + uk->o16 + (i << 5);

		v0 = p[1];
		v1 = p[0];
		v0 <<= 8;
		v0 |= v1;
		v0++;
		v0 &= 0xFFFF;
		v1 = v0 >> 8;
		p[1] = v1;
		p[0] = v0;
	}

	return 0;
}

/* 0x88FC0890 */
unsigned int __attribute__((noinline))
sub_88FC0890(void *a0, void *a1)
{
	unsigned int r;
	
	r = reboot6(a0, a1);
	sub_88FC0604(a0, INIT_STR, HEN_STR, 255);
	if (rtm_init)
		sub_88FC0604(a0, rtm_init, RTM_STR, rtm_op);
	return r;
}

int __attribute__ ((section (".text.start")))
_start(void *a0, void *a1, void *a2, void *a3)
{
	return main(a0, a1, a2, a3);
}

