#ifndef _SYSTEM_CTRL_PRIV_H
#define _SYSTEM_CTRL_PRIV_H

#include "psptypes.h"
#include "psploadexec_kernel.h"
#include "pspiofilemgr_kernel.h"

#include "systemctrl.h"

#if 0
static const char *__func_tag;
#define ASM_FUNC_TAG() __func_tag = __FUNCTION__
#else
#define ASM_FUNC_TAG()
#endif

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

#ifndef sceKernelApplicationType
#define sceKernelApplicationType InitForKernel_7233B5BC
#endif

#endif

