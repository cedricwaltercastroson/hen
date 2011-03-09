#ifndef __KGTEXT_H__
#define  __KGTEXT_H__

// #define KGT_DRAW_BG
// #define KGT_USE_PRINTF

extern unsigned char font_start[];

void kgt_init();
#ifdef KGT_DRAW_BG
void kgt_PutChar(int in, int px, int py, int color , int back);
void kgt_write(char *text, int px, int py, int color, int back);
#ifdef KGT_USE_PRINTF
void kgt_printf(int px, int py, int color, int back, char *text, ...);
#endif
#else
void kgt_PutChar(int in, int px, int py, int color);
void kgt_write(char *text, int px, int py, int color);
#ifdef KGT_USE_PRINTF
void kgt_printf(int px, int py, int color, char *text, ...);
#endif
#endif
void kgt_clear(int color);
void kgt_clearbox(int x, int y, int w, int h, int color);

#endif
