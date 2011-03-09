#ifndef _KUBRIDGE_H
#define _KUBRIDGE_H

#include "pspsdk.h"
#include "pspmodulemgr_kernel.h"


/* 0x00003264 */
/* KUBridge_4C25EA72 */
extern SceUID kuKernelLoadModule(const char *path, int flags, SceKernelLMOption *option);

/* 0x000032D0 */
/* KUBridge_1E9F0498 */
extern SceUID kuKernelLoadModuleWithApitype2(int apitype, const char *path, int flags, SceKernelLMOption *option);

/* 0x0000334C */
/* KUBridge_8E5A4057 */
extern int kuKernelInitApitype(void);

/* 0x00003354 */
/* KUBridge_1742445F */
extern int kuKernelInitFileName(char *fname);

/* 0x000033A4 */
/* KUBridge_60DDB4AE */
extern int kuKernelBootFrom(void);

/* 0x000033AC */
/* KUBridge_B0B8824E */
extern int kuKernelInitKeyConfig(void);

/* 0x000033B4 */
/* KUBridge_A2ABB6D3 */
extern int kuKernelGetUserLevel(void);

/* 0x000033F8 */
/* KUBridge_C4AF12AB */
extern int kuKernelSetDdrMemoryProtection(void *addr, int size, int prot);

/* 0x00003464 */
/* KUBridge_24331850 */
extern int kuKernelGetModel(void);

#endif
