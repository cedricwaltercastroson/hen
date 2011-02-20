#ifndef _MAIN_H
#define _MAIN_H

#include "psptypes.h"

#include "systemctrl_priv.h"

#define MAKE_CALL(__f) \
	(((((unsigned int)__f) >> 2) & 0x03FFFFFF) | 0x0C000000)

#define MAKE_JMP(__f) \
	(((((u32)__f) & 0x0FFFFFFC) >> 2) | 0x08000000)

#define REDIRECT_FUNCTION(__a, __f) do {\
	_sw(0x08000000 | ((((u32) __f) >> 2) & 0x03FFFFFF), __a);\
	_sw(0, __a + 4);\
} while (0)

#define MAKE_DUMMY_FUNCTION0(__a) do {\
	_sw(0x03E00008, __a);\
	_sw(0x00001021, __a + 4);\
} while (0)

#define MAKE_DUMMY_FUNCTION1(__a) do {\
	_sw(0x03E00008, __a);\
	_sw(0x24020001, __a + 4);\
} while (0)


/* 0x000083E8 */
extern TNConfig g_tnconfig;

extern int g_model; /* 0x00008270 */

extern int g_p2_size; /* 0x00008258 */
extern int g_p8_size; /* 0x0000825C */

extern unsigned int g_timestamp_1; /* 0x000083D4 */
extern unsigned int g_timestamp_2; /* 0x000083DC */

extern int (*VshMenuCtrl) (SceCtrlData *, int); /* 0x000083B0 */

extern int (*DecryptExecutable_HEN)(char *buf, int size, int *compressed_size, int polling); /* 0x00008260 */
extern int (*DecryptPrx_HEN) (int a0, int a1, int a2, char *buf, int size, int *compressed_size, int polling, int t3); /* 0x00008290 */

extern int (*g_scePowerSetClockFrequency) (int, int, int); /* 0x000083AC */

extern char *g_reboot_module; /* 0x000083B8 */
extern void *g_reboot_module_buf; /* 0x000083C8 */
extern int g_reboot_module_size; /* 0x000083D0 */
extern int g_reboot_module_flags; /* 0x000083CC */

extern int *g_apitype_addr; /* 0x00008288 */
extern char **g_init_filename_addr; /* 0x00008284 */
extern int *g_keyconfig_addr; /* 0x0000839C */

extern void (*ModuleStartHandler) (void *); /* 0x000083A4 */

#define find_text_addr_by_name(__name) \
	(u32) _lw((u32) (sceKernelFindModuleByName(__name)) + 108)

extern u32 FindScePowerFunction(u32 nid);

typedef struct {
	u32 oldnid;
	u32 newnid;
} nidentry_t;

typedef struct {
	char *name;
	nidentry_t *nids;
	int cnt;
} nidtable_t;

extern nidtable_t *FindLibNidTable(const char *name);

extern u32 TranslateNid(nidtable_t *t, u32 nid);

extern void ClearCaches(void);

#endif
