#ifndef _SYSTEM_CTRL_H
#define _SYSTEM_CTRL_H

#include "psptypes.h"
#include "pspkernel.h"
#include "psputility.h"
#include "psputilsforkernel.h"
#include "pspinit.h"
#include "pspctrl.h"
#include "psploadexec_kernel.h"
#include "pspmodulemgr_kernel.h"

typedef struct {
	int magic;//offset 0x000083E8 - 0x00    // 0x47434E54 == "TNCG" 
	int vshcpuspeed; //0x000083EC - 0x04
	int vshbusspeed; //0x000083F0 - 0x08
	int umdisocpuspeed; //0x000083F4 - 0x0C
	int umdisobusspeed; //0x000083F8 - 0x10
	int fakeregion;//0x000083FC - 0x14
	int skipgameboot;//0x00008400 - 0x18
	int showmac;//0x00008404 0x1C   //1 show 0 hide
	int notnupdate;//0x00008408 - 0x20    //1 normal 0 tnupdate
	int hidepic;//0x0000840C - 0x24
	int nospoofversion;//0x00008410 - 0x28 //1 normal 0 spoof
	int slimcolor;//0x00008414 - 0x2C
	int fastscroll;//0x00008418 - 0x30 //1 enabled 0 disabled
	int protectflash;//0x0000841C - 0x34
	int fakeindex;//0x00008420 - 0x38
	int unk;//0x00008424 - 0x3C
} TNConfig;

typedef enum {
	FAKE_REGION_DISABLE = 0,
	FAKE_REGION_JAPAN,
	FAKE_REGION_AMERICA,
	FAKE_REGION_EUROPE,
	FAKE_REGION_KOREA,
	FAKE_REGION_UK,
	FAKE_REGION_MEXICO,
	FAKE_REGION_AU_NZ,
	FAKE_REGION_EAST,
	FAKE_REGION_TAIWAN,
	FAKE_REGION_RUSSIA,
	FAKE_REGION_CHINA,
	FAKE_REGION_DEBUGI,
	FAKE_REGION_DEBUGII
} FAKE_REGION_TYPE;

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

#define find_module_by_name(__name) (SceModule2 *) sceKernelFindModuleByName(__name)

#define find_text_addr_by_name(__name) \
	(u32) _lw((u32) (sceKernelFindModuleByName(__name)) + 108)

/* ELF file header */
typedef struct {
	u32		e_magic;		// 0
	u8		e_class;		// 4
	u8		e_data;			// 5
	u8		e_idver;		// 6
	u8		e_pad[9];		// 7
	u16		e_type;			// 10
	u16		e_machine;		// 12
	u32		e_version;		// 14
	u32		e_entry;		// 18
	u32		e_phoff;		// 1C
	u32		e_shoff;		// 20
	u32		e_flags;		// 24
	u16		e_ehsize;		// 28
	u16		e_phentsize;	// 2A
	u16		e_phnum;		// 2C
	u16		e_shentsize;	// 2E
	u16		e_shnum;		// 30
	u16		e_shstrndx;		// 32
} __attribute__((packed)) Elf32_Ehdr;

/* ELF section header */
typedef struct { 
	u32		sh_name;		// 0
	u32		sh_type;		// 4
	u32		sh_flags;		// 8
	u32		sh_addr;		// C
	u32		sh_offset;		// 10
	u32		sh_size;		// 14
	u32		sh_link;		// 18
	u32		sh_info;		// 1C
	u32		sh_addralign;	// 20
	u32		sh_entsize;		// 24
} __attribute__((packed)) Elf32_Shdr;

#define ELF_MAGIC	0x464C457FU


/**
  * Copied from M33 SDK
  *
  * Load a module with the VSH apitype.
  * 
  * @param path - The path to the module to load.
  * @param flags - Unused, always 0 .
  * @param option  - Pointer to a mod_param_t structure. Can be NULL.
  *
  * @returns The UID of the loaded module on success, otherwise one of ::PspKernelErrorCodes.
  */
extern SceUID sceKernelLoadModuleVSH(const char *path, int flags, SceKernelLMOption *option);

extern int sceKernelLoadModuleForLoadExec(int apitype, const char *path, int flags, SceKernelLMOption *option);

extern int sceKernelCheckExecFile(void *, int *);

extern int sceKernelGetSystemStatus(void);

#endif

