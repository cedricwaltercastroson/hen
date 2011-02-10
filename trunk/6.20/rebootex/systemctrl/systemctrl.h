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

#if 1
static const char *__func_tag;
#define ASM_FUNC_TAG() __func_tag = __FUNCTION__
#else
#define ASM_FUNC_TAG()
#endif

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


/* 0x000003F4 */
/* SystemCtrlForKernel_826668E9 */
extern void PatchSyscall(u32 fp, void *func);

/* 0x000003D4 */
extern void *SystemCtrlForKernel_AC0E84D1(void *func);

/* 0x000003E4 */
extern void *SystemCtrlForKernel_1F3037FB(void *func);

/* 0x00000364 */
/* SystemCtrlForUser_1C90BECB SystemCtrlForKernel_1C90BECB */
extern void *sctrlHENSetStartModuleHandler(void *handler);

/* 0x000030F4 */
/* SystemCtrlForUser_8E426F09 SystemCtrlForKernel_8E426F09 */
extern int sctrlSEGetConfigEx(void *buf, SceSize size);

/* 0x0000319C */
/* SystemCtrlForUser_16C3B7EE SystemCtrlForKernel_16C3B7EE */
extern int sctrlSEGetConfig(void *buf);

/* 0x000031A4 */
/* SystemCtrlForUser_AD4D5EA5 SystemCtrlForKernel_AD4D5EA5 */
extern int sctrlSESetConfigEx(TNConfig *config, int size);

/* 0x0000325C */
/* SystemCtrlForUser_1DDDAD0C SystemCtrlForKernel_1DDDAD0C */
extern int sctrlSESetConfig(TNConfig *config);

/* 0x000034A8 */
/* SystemCtrlForUser_D339E2E9 SystemCtrlForKernel_D339E2E9 */
extern int sctrlHENIsSE(void);

/* 0x000034B0 */
/* SystemCtrlForUser_2E2935EF SystemCtrlForKernel_2E2935EF */
extern int sctrlHENIsDevhook(void);

/* 0x000034B8 */
/* SystemCtrlForUser_1090A2E1 SystemCtrlForKernel_1090A2E1 */
extern int sctrlHENGetVersion(void);

/* 0x000034C0 */
/* SystemCtrlForUser_B47C9D77 SystemCtrlForKernel_B47C9D77 */
extern int sctrlSEGetVersion(void);

/* 0x000034C8 */
/* SystemCtrlForUser_D8FF9B99 SystemCtrlForKernel_D8FF9B99 */
extern int sctrlKernelSetDevkitVersion(int ver);

/* 0x0000353C */
/* SystemCtrlForUser_EB74FE45 SystemCtrlForKernel_EB74FE45 */
extern int sctrlKernelSetUserLevel(int level);

/* 0x000035B8 */
/* SystemCtrlForUser_2D10FB28 SystemCtrlForKernel_2D10FB28 */
extern int sctrlKernelLoadExecVSHWithApitype(int apitype, const char *file, struct SceKernelLoadExecVSHParam *param);

/* 0x00003650 */
/* SystemCtrlForUser_78E46415 SystemCtrlForKernel_78E46415 */
extern PspIoDrv *sctrlHENFindDriver(char* drvname);

/* 0x000036C4 */
/* SystemCtrlForUser_577AF198 SystemCtrlForKernel_577AF198 */
extern int sctrlKernelLoadExecVSHDisc(const char *file, struct SceKernelLoadExecVSHParam *param);

/* 0x00003720 */
/* SystemCtrlForUser_94FE5E4B SystemCtrlForKernel_94FE5E4B */
extern int sctrlKernelLoadExecVSHDiscUpdater(const char *file, struct SceKernelLoadExecVSHParam *param);

/* 0x0000377C */
/* SystemCtrlForUser_75643FCA SystemCtrlForKernel_75643FCA */
extern int sctrlKernelLoadExecVSHMs1(const char *file, struct SceKernelLoadExecVSHParam *param);

/* 0x000037D8 */
/* SystemCtrlForUser_ABA7F1B0 SystemCtrlForKernel_ABA7F1B0 */
extern int sctrlKernelLoadExecVSHMs2(const char *file, struct SceKernelLoadExecVSHParam *param);

/* 0x00003834 */
/* SystemCtrlForUser_7B369596 SystemCtrlForKernel_7B369596 */
extern int sctrlKernelLoadExecVSHMs3(const char *file, struct SceKernelLoadExecVSHParam *param);

/* 0x00003890 */
/* SystemCtrlForUser_D690750F SystemCtrlForKernel_D690750F */
extern int sctrlKernelLoadExecVSHMs4(const char *file, struct SceKernelLoadExecVSHParam *param);

/* 0x000038EC */
/* SystemCtrlForUser_2794CCF4 SystemCtrlForKernel_2794CCF4 */
extern int sctrlKernelExitVSH(struct SceKernelLoadExecVSHParam *param);

/* 0x00000BC4
 * SystemCtrlForKernel_159AF5CC
 */
extern u32 sctrlHENFindFunction(char *module_name, char *lib_name, u32 nid);

/* 0x00001D50 */
/* SystemCtrlForKernel_CE0A654E */
extern void sctrlHENLoadModuleOnReboot(char *module_after, void *buf, int size, int flags);

/* 0x00001D74 */
extern void SystemCtrlForKernel_B86E36D1(void);

/* 0x00002324 */
/* SystemCtrlForKernel_2F157BAF */
extern void SetConfig(TNConfig *config);

/* 0x000023BC */
/* SystemCtrlForKernel_CC9ADCF8 */
extern int sctrlHENSetSpeed(int cpuspd, int busspd);

/* 0x00002664 */
/* SystemCtrlForUser_745286D1 SystemCtrlForKernel_745286D1 */
extern int sctrlHENSetMemory(int p2, int p8);

/* 0x000027EC */
/* SystemCtrlForKernel_98012538 */
extern void SetSpeed(int cpuspd, int busspd);

/* 0x00003EA8 */
/* SystemCtrlForUser_CB76B778 SystemCtrlForKernel_CB76B778 */
extern int sctrlKernelSetInitKeyConfig(int key);

/* 0x00003F04 */
/* SystemCtrlForUser_128112C3 SystemCtrlForKernel_128112C3 */
extern int sctrlKernelSetInitFileName(char *file);

/* 0x00003F44 */
/* SystemCtrlForUser_8D5BE1F0 SystemCtrlForKernel_8D5BE1F0 */
extern int sctrlKernelSetInitApitype(int apitype);

/* 0x00002620 */
/* VshCtrlLib_FD26DA72 */
extern int vctrlVSHRegisterVshMenu(void *ctrl);

/* 0x00002C90 */
/* VshCtrlLib_CD6B3913 */
extern int vctrlVSHExitVSHMenu(TNConfig *conf);

#endif

