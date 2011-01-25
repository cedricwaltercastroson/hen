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
	int fakeregion;//0x000083FC - 0x14                 //13 Debug II
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

typedef struct SceModule2
{
	struct SceModule2	*next; // 0
	u16					attribute; // 4
	u8					version[2]; // 6
	char				modname[27]; // 8
	char				terminal; // 0x23
	char				mod_state;	// 0x24
	char				unk1;    // 0x25
	char				unk2[2]; // 0x26
	u32					unk3;	// 0x28
	SceUID				modid; // 0x2C
	u32					unk4; // 0x30
	SceUID				mem_id; // 0x34
	u32					mpid_text;	// 0x38
	u32					mpid_data; // 0x3C
	void *				ent_top; // 0x40
	unsigned int		ent_size; // 0x44
	void *				stub_top; // 0x48
	u32					stub_size; // 0x4C
	u32					entry_addr_; // 0x50
	u32					unk5[4]; // 0x54
	u32					entry_addr; // 0x64
	u32					gp_value; // 0x68
	u32					text_addr; // 0x6C
	u32					text_size; // 0x70
	u32					data_size;	// 0x74
	u32					bss_size; // 0x78
	u32					nsegment; // 0x7C
	u32					segmentaddr[4]; // 0x80
	u32					segmentsize[4]; // 0x90
} SceModule2;

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
	u16		e_type;			// 16
	u16		e_machine;		// 18
	u32		e_version;		// 20
	u32		e_entry;		// 24
	u32		e_phoff;		// 28
	u32		e_shoff;		// 32
	u32		e_flags;		// 36
	u16		e_ehsize;		// 40
	u16		e_phentsize;	// 42
	u16		e_phnum;		// 44
	u16		e_shentsize;	// 46
	u16		e_shnum;		// 48
	u16		e_shstrndx;		// 50
} __attribute__((packed)) Elf32_Ehdr;

/* ELF section header */
typedef struct { 
	u32		sh_name;		// 0
	u32		sh_type;		// 4
	u32		sh_flags;		// 8
	u32		sh_addr;		// 12
	u32		sh_offset;		// 16
	u32		sh_size;		// 20
	u32		sh_link;		// 24
	u32		sh_info;		// 28
	u32		sh_addralign;	// 32
	u32		sh_entsize;		// 36
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


#endif

