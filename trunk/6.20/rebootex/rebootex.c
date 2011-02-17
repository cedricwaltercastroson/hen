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

int (*RebootEntry)(void *, void *, void *, void *) = (void *) REBOOT_BASE;
void (*DcacheClear)(void) = (void *) 0x88600938;
void (*IcacheClear)(void) = (void *) 0x886001E4;

int (*sceBootLfatRead)(void *, void *);
int (*sceBootLfatOpen)(char *);
int (*sceBootLfatClose)(void);
int (*sceBootDecryptPSP)(void *, void *);

int (*sceDecryptPSP)(void *, unsigned int, int *);
int (*sceKernelCheckExecFile)(unsigned char *, unsigned int);

int has_hen_prx;

static unsigned int size_sysctrl_bin;
static unsigned char sysctrl_bin[];

#include "sysctrl_bin.inc"

/* util functions */

static int
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

static void
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

static void
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

static int
__strlen(char *s)
{
	int len = 0;

	while (*s) {
		len++;
		s++;
	}

	return len;
}

static void
__memmove(void *dst, void *src, int len)
{
	char *d, *s;

	d = dst;
	s = src;
	if (s < d) {
		for (s += len, d += len; len; --len)
			*--d = *--s;
	} else if (s != d) {
		for (; len; --len)
			*d++ = *s++;
	}
}

void
ClearCaches(void)
{
	DcacheClear();
	IcacheClear();
}

int
sceBootLfatRead_Patched(void *a0, void *a1)
{
	if (has_hen_prx) {
		__memcpy(a0, sysctrl_bin, size_sysctrl_bin);
		return size_sysctrl_bin;
	}
	return sceBootLfatRead(a0, a1);
}

int
sceBootLfatOpen_Patched(char *s)
{
	if (!__memcmp(s, HEN_STR, 9)) {
		has_hen_prx = 1;
		return 0;
	}
	return sceBootLfatOpen(s);
}

int
sceBootLfatClose_Patched(void)
{
	if (has_hen_prx) {
		has_hen_prx = 0;
		return 0;
	}
	return sceBootLfatClose();
}

typedef struct
{
	u32		signature;  // 0
	u16		attribute; // 4  modinfo
	u16		comp_attribute; // 6
	u8		module_ver_lo;	// 8
	u8		module_ver_hi;	// 9
	char	modname[28]; // 0A
	u8		version; // 26
	u8		nsegments; // 27
	int		elf_size; // 28
	int		psp_size; // 2C
	u32		entry;	// 30
	u32		modinfo_offset; // 34
	int		bss_size; // 38
	u16		seg_align[4]; // 3C
	u32		seg_address[4]; // 44
	int		seg_size[4]; // 54
	u32		reserved[5]; // 64
	u32		devkitversion; // 78
	u32		decrypt_mode; // 7C 
	u8		key_data0[0x30]; // 80
	int		comp_size; // B0
	int		_80;	// B4
	int		reserved2[2];	// B8
	u8		key_data1[0x10]; // C0
	u32		tag; // D0
	u8		scheck[0x58]; // D4
	u32		key_data2; // 12C
	u32		oe_tag; // 130
	u8		key_data3[0x1C]; // 134
} __attribute__((packed)) PSP_Header;

int
secDecryptPSP_Patched(void *buf, unsigned int a1, int *compressed_size)
{
	PSP_Header *hdr = (PSP_Header *) buf;

	/* M33 tag or psp-packer tag */
	if (hdr->oe_tag == 0xB301AEBA || hdr->oe_tag == 0x55668D96) {
		__memcpy(buf, buf + 0x150, hdr->comp_size);
		*compressed_size = hdr->comp_size;
		return 0;
	}

	return sceDecryptPSP(buf, a1, compressed_size);
}

int
sceKernelCheckExecFile_Patched(void *buf, int *check)
{
	int i;
	PSP_Header *hdr = buf;

	/* s is PSP_Header. check for hdr->scheck[] */
	for (i = 0; i < 0x58; i++) {
		if (hdr->scheck[i] != 0)
			return sceKernelCheckExecFile(buf, check);
	}

	return 0;
}

int
PatchLoadCore(void *a0, void *a1, void *a2,
		int (*module_start)(void *, void *, void*))
{
	u32 text_addr = (u32) module_start; /* loadcore.prx offset 0xBC4 */
	unsigned int fp;

	fp = MAKE_CALL(secDecryptPSP_Patched);
	_sw(fp, text_addr + 13792); /* 0x41A4 mask call to sceDecryptPSP */
	_sw(fp, text_addr + 23852); /* 0x68F0 mask call to sceDecryptPSP */

	fp = MAKE_CALL(sceKernelCheckExecFile_Patched);
	_sw(fp, text_addr + 23888); /* 0x6914 mask call to sceKernelCheckExecFile */
	_sw(fp, text_addr + 23936); /* 0x6944 mask call to sceKernelCheckExecFile */
	_sw(fp, text_addr+ 24088); /* 0x69DC mask call to sceKernelCheckExecFile */

	sceDecryptPSP = (void *) (text_addr + 30640); /* 0x8374 */
	sceKernelCheckExecFile = (void *) (text_addr + 30616); /* 0x835C */

	ClearCaches();

	return module_start(a0, a1, a2);
}


typedef struct {
	u32 magic;
	u32 version;
	u32 unk0[2];
	u32 modes_start;
	u32 modes_nr;
	u32 unk1[2];
	u32 modules_start;
	u32 modules_nr;
	u32 unk2[2];
	u32 names_start;
	u32 names_end;
	u32 unk3[2];
} BtcnfHeader;

typedef struct {
	u32 name;
	u32 unk0;
	u32 flags;
	u32 unk1;
	u8  hash[16];
} ModuleEntry;

typedef struct {
	u16 modules_nr;
	u16 module_start;
	u32 mode_mask;
	u32 mode_id;
	u32 mode_name;
	u32 unk[4];
} ModeEntry;


void
inject_module(void *a0, char *mod_name, char *neu_mod_name, u16 flags)
{
	ModuleEntry mod;
	ModuleEntry *pmod;
	ModeEntry *pme;
	int i, len, len2;
	char *modules_start, *names_start, *names_end;
	BtcnfHeader *hdr = a0;

	modules_start = (char *) a0 + hdr->modules_start;
	names_start = (char *) a0 + hdr->names_start;
	names_end = (char *) a0 + hdr->names_end;

	/* append new module name */
	len2 = __strlen(neu_mod_name) + 1;
	__memcpy(names_end, neu_mod_name, len2);
	hdr->names_end += len2;

	pmod = (ModuleEntry *) modules_start;
	len = __strlen(mod_name) + 1;

	if (hdr->modules_nr < 0)
		return;

	/* search mod by name */
	for (i = 0; i < hdr->modules_nr; i++) {
		if (!__memcmp(names_start + pmod->name, mod_name, len))
			break;
		pmod++;
	}
	if (i == hdr->modules_nr)
		return;

	__memset(&mod, 0, sizeof(ModuleEntry));
	mod.name = names_end - names_start;
	mod.flags = flags | 0x80010000;

	len = (hdr->modules_nr - i) * sizeof(ModuleEntry);
	len += len2;
	len += names_end - names_start;

	modules_start += i * sizeof(ModuleEntry);
	__memmove(modules_start + sizeof(ModuleEntry), modules_start, len);
	__memcpy(modules_start, &mod, sizeof(ModuleEntry));


	hdr->modules_nr += 1;
	hdr->names_start += 32;
	hdr->names_end += 32;

	if (hdr->modes_nr <= 0)
		return;

	pme = (ModeEntry *) ((char *) a0 + hdr->modes_start);
	for (i = 0; i < hdr->modes_nr; i++) {
		pme->modules_nr++;
		pme++;
	}
}

int
sceBootDecryptPSP_Patched(void *a0, void *a1)
{
	int r;
	
	r = sceBootDecryptPSP(a0, a1);
	inject_module(a0, "/kd/init.prx", HEN_STR, 255);

	return r;
}

int
main(void *a0, void *a1, void *a2, void *a3)
{
	static unsigned int model0[] = {
		0x000082AC, 0x00008420, 0x000083C4, 0x0000565C,
		0x000026DC, 0x0000274C, 0x00002778, 0x000070F0,
		0x00003798, 0x0000379C, 0x000026D4, 0x00002728,
		0x00002740, 0x00005550, 0x00005554, 0x00005558,
		0x00007388
	};

	static unsigned int model1[] = {
		0x00008374, 0x000084E8, 0x0000848C, 0x00005724,
		0x000027A4, 0x00002814, 0x00002840, 0x000071B8,
		0x00003860, 0x00003864, 0x0000279C, 0x000027F0,
		0x00002808, 0x00005618, 0x0000561C, 0x00005620,
		0x00007450
	};

	unsigned int *pf;

	if (_lw(0x88FB0000) == 0)
		pf = model0;
	else
		pf = model1;

	SAVE_CALL(pf[4], sceBootLfatOpen_Patched); /* replace sceBootLfatOpen */
	SAVE_CALL(pf[5], sceBootLfatRead_Patched); /* replace sceBootLfatRead */
	SAVE_CALL(pf[6], sceBootLfatClose_Patched); /* replace sceBootLfatClose */
	SAVE_CALL(pf[7], sceBootDecryptPSP_Patched); /* replace sceBootDecryptPSP */

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
	sceBootDecryptPSP = (void *) (pf[3] | REBOOT_BASE);

	has_hen_prx = 0;

	ClearCaches();

	return RebootEntry(a0, a1, a2, a3);
}

int __attribute__ ((section (".text.start")))
_start(void *a0, void *a1, void *a2, void *a3)
{
	return main(a0, a1, a2, a3);
}

