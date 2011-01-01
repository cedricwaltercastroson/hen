/* all functions and variables are public (non-static) to avoid being
 * optimized by -O2
 */

extern int __strncmp(const char *s1, const char *s2, int num);
extern void __memset(void *s, char c, int len);
extern void __memcpy(void *dst, void *src, int len);
extern int __strlen(char *s);

/* global variables */

/* all rebootX come from reboot.bin */
void (*reboot0)(unsigned int, unsigned int, unsigned int, unsigned int) = (void *) 0x88660000; /* ref 0x88FC09C8 */
void (*reboot1)(void) = (void *) 0x88600938; /* ref 0x88FC09CC */
void (*reboot2)(void) = (void *) 0x886001E4; /* ref 0x88FC09D0 */

unsigned int (*reboot3)(unsigned int, unsigned int); /* ref 0x88FC7210 */
int (*reboot4)(void); /* ref 0x88FC71F0 */
int (*reboot5)(void); /* ref 0x88FC71F4 */
void (*reboot6)(void); /* ref 0x88FC721C */

int (*func1)(void); /* ref 0x88FC7200 */
void (*func2)(char *, int, int, int); /* ref 0x88FC7218 */

char *str1; /* ref 0x88FC71FC */

int has_hen_prx; /* ref 0x88FC7214 */
int has_rtm_prx; /* ref 0x88FC71F8 */

char *hen_str = "/hen.prx";
char *rtm_str = "/rtm.prx";
char *init_str = "/kd/init.prx";

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

char *rtm_addr; /* ref 0x88FC7204 */
unsigned int rtm_len; /* ref 0x88FC720C */
unsigned int rtm_op; /* ref 0x88FC7208 */

extern unsigned char systemctrl[];

#define SYSTEM_CTL_SZ	26635
#include "systemctl.inc"

/* 0x88FC00D4 */
void
sub_88FC00D4(void)
{
	reboot1();
	reboot2();
}

/* 0x88FC0100 */
unsigned int
sub_88FC0100(unsigned int a0, unsigned int a1)
{
	if (has_hen_prx) {
		__memcpy((void *) 0x88FC0000, systemctrl, SYSTEM_CTL_SZ);
		return SYSTEM_CTL_SZ;
	} else if (has_rtm_prx) {
		__memcpy((void *) 0x88FC0000, rtm_addr, rtm_len);
		return rtm_len;
	}
	return reboot3(a0, a1);
}

/* 0x88FC0188 */
int
sub_88FC0188(char *s)
{
	if (!__strncmp(s, hen_str, 9)) {
		has_hen_prx = 1;
		return 0;
	} else if (!__strncmp(s, rtm_str, 9)) {
		has_rtm_prx = 1;
		return 0;
	}
	return reboot4();
}

/* 0x88FC021C */
int
sub_88FC021C(void)
{
	if (has_hen_prx) {
		has_hen_prx = 0;
		return 0;
	}
	if (has_rtm_prx) {
		has_rtm_prx = 0;
		return 0;
	}
	return reboot5();
}

/* 0x88FC025C */
int
sub_88FC025C(char *s)
{
	int len;
	char *s2;

	if (*(unsigned int *) (s + 304) == 0xB301AEBAU) {
		len = *(int *) (s + 176);
		s2 = s + 336;
		__memcpy(s, s2, len);
		*(int *) (s + 176) = len;
		return 0;
	}

	return func1();
}

/* 0x88FC02C8 */
int
sub_88FC02C8(char *s, int a1)
{
	int a2 = 0, a3 = 88;
	char *v0;
	char v1;

	do {
		v0 = s + a2;
		v1 = v0[212];
		if (v1)
			func2(s, a1, a2, a3); /* not sure how many params here. at most 4 */
		a2++;
	} while (a2 != a3);

	return 0;
}

#define MAKE_CALL(__f) \
	(((((unsigned int)__f) >> 2) & 0x03FFFFFF) | 0x0C000000)

/* 0x88FC0304 */
void
sub_88FC0304(unsigned int a0, int a1, int a2, unsigned int a3)
{
	unsigned int f1 = MAKE_CALL(sub_88FC025C);
	unsigned int f2 = MAKE_CALL(sub_88FC02C8);
	void (*f3)(unsigned int, int, int) = (void *) a3;

	*(unsigned int *) (a3 + 13792) = f1;
	*(unsigned int *) (a3 + 23852) = f1;
	*(unsigned int *) (a3 + 23888) = f2;
	*(unsigned int *) (a3 + 23936) = f2;
	*(unsigned int *) (a3 + 24088) = f2;

	func1 = (void *) (a3 + 30640);
	func2 = (void *) (a3 + 30616);
	sub_88FC00D4();

	f3(a0, a1, a2);
}

/* 0x88FC0604 */
int
sub_88FC0604(char *a0, char *a1, char *a2, unsigned int a3)
{
	char buf[32];
	int len, cnt, i, t0, t1, t2;
	char *s0, *s1, *s2;

#define _(__p, __off) (int *) ((__p) + (__off))

	t0 = *_(a0, 32);
	t1 = *_(a0, 48);
	t2 = *_(a0, 52);

	s0 = a0 + t0; /* 32 */
	s1 = a0 + t1; /* 48 */
	s2 = a0 + t2; /* 52 */

	len = __strlen(a2) + 1;
	__memcpy(s2, a2, len);
	*_(a0, 52) += len;

	cnt = *_(a0, 36);

	if (cnt == 0)
		return -2;

	if (cnt > 0) {
		len = __strlen(a1) + 1;
		for (i = 0; i < cnt; i++) {
			if (__strncmp(s1 + *_(s0, 0), a1, len) != 0)
				break;
			s0 += 32;
		}
		if (i == cnt)
			return -2;
	}

	memset(buf, 0, 32);
	*(int *) buf = t2 - t1;
	*(short *) &buf[8] = (short) a3;

	/* XXX */
	
	return 0;
#undef _
}

/* 0x88FC0890 */
int
sub_88FC0890(char *a0)
{
	int r;

	reboot6();
	r = sub_88FC0604(a0, init_str, hen_str, 255);
	if (str1 == 0)
		return r;
	return sub_88FC0604(a0, str1, rtm_str, rtm_op);
}

#define SAVE_CALL(__a, __f) do {\
	*(unsigned int *) (__a + 0x88600000U) = MAKE_CALL(__f);\
} while(0)

#define SAVE_VALUE(__a, __v) do {\
	*(unsigned int *) (__a + 0x88600000U) = (__v);\
} while(0)

void _start(unsigned int, unsigned int, unsigned int, unsigned int) __attribute__ ((section (".text.start")));

void
_start(unsigned int a0, unsigned int a1, unsigned int a2, unsigned int a3)
{
	int model = *(int *) 0x88FB0000;
	unsigned int *pf;

	if (model == 0)
		pf = model0;
	else
		pf = model1;

	SAVE_CALL(pf[4], sub_88FC0188);
	SAVE_CALL(pf[5], sub_88FC0100);
	SAVE_CALL(pf[6], sub_88FC021C);
	SAVE_CALL(pf[7], sub_88FC0890);
	SAVE_VALUE(pf[8], 0x03E00008); /* jr ra */
	SAVE_VALUE(pf[9], 0x24020001); /* addiu $v1, $zr, 1 */
	SAVE_VALUE(pf[10], 0); /* nop */
	SAVE_VALUE(pf[11], 0); /* nop */
	SAVE_VALUE(pf[12], 0); /* nop */
	SAVE_VALUE(pf[13], 0x00113821); /* addu $a3, $ar, $s1 */
	SAVE_CALL(pf[14], sub_88FC0304);
	SAVE_VALUE(pf[15], 0x02A0E821); /* addu $sp, $zr, $s5 */
	SAVE_VALUE(pf[16], 0); /* nop */

	reboot4 = (void *) (pf[0] | 0x88600000);
	reboot3 = (void *) (pf[1] | 0x88600000);
	reboot5 = (void *) (pf[2] | 0x88600000);
	reboot6 = (void *) (pf[3] | 0x88600000);

	str1 = *(char **) 0x88FB0010;
	rtm_addr = *(char **) 0x88FB0014;
	rtm_len = *(unsigned int *) 0x88FB0018;
	rtm_op = *(unsigned int *) 0x88FB001C;

	has_hen_prx = 0;
	has_rtm_prx = 0;

	sub_88FC00D4();

	reboot0(a0, a1, a2, a3);
}

/* util functions */

int
__strncmp(const char *s1, const char *s2, int len)
{
	int i;

	if (len <= 0)
		return 0;

	for (i = 0; *s1 == *s2 && i < len; ++s1, ++s2, ++i)
		if (*s1 == 0)
			return 0;

	return *(unsigned char *) s1 - *(unsigned char *) s2;
}

void
__memset(void *p, char c, int len)
{
	int i;
	char *s = p;

	if (len <= 0)
		return;

	for (i = 0; i < len; i++, ++s)
		*s = c;

	return;
}

void
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

int
__strlen(char *s)
{
	int len = 0;

	while (*s) {
		len++;
		s++;
	}

	return len;
}
