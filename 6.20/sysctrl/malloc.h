#ifndef _MALLOC_H
#define _MALLOC_H

#include "psptypes.h"

#define malloc oe_malloc
#define free oe_free

extern int mallocinit(void);

/* 0x00003054 */
/* SystemCtrlForKernel_F9584CAD */
extern void *oe_malloc(u32 size);

/* 0x000030A4 */
/* SystemCtrlForKernel_A65E8BC4 */
extern void oe_free(void *ptr);

#endif
