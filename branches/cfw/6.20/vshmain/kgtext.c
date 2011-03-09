#include <stdlib.h>
#include <stdarg.h>
#include <pspdisplay.h>
#include "kgtext.h"

#define VRAM 0x44000000

void kgt_init()
{
	sceDisplaySetFrameBuf((void *)(VRAM & 0x7FFFFFFF), 512, PSP_DISPLAY_PIXEL_FORMAT_8888, 1);
	kgt_clear(0);
}

#ifdef KGT_DRAW_BG
void kgt_PutChar(int in, int px, int py, int color, int back)
#else
void kgt_PutChar(int in, int px, int py, int color)
#endif
{
	int x, y;
	int *pixel;

	unsigned char *buff = font_start;
	buff += 16 * in;

	for(y = 0; y < 16; y++) {
		pixel = (int *)VRAM;
		pixel += (py + y) * 512 + px;
		for(x = 0; x < 8; x++) {
			if(*buff & (128 >> x)) *pixel = color;
#ifdef KGT_DRAW_BG
			else *pixel = back;
#endif
			pixel++;
		}
#ifdef KGT_DRAW_BG
		*pixel = back;
#endif
		buff++;
	}
}

#ifdef KGT_DRAW_BG
void kgt_write(char *text, int px, int py, int color, int back)
#else
void kgt_write(char *text, int px, int py, int color)
#endif
{
	int ox = px;
	while(*text) {
		if(*text == '\n') {
			py += 16;
			px = ox;
			text++;
			continue;
		}
		if(*text == '\t') {
			px += 9 * 2;
			text++;
			continue;
		}
#ifdef KGT_DRAW_BG
		kgt_PutChar((unsigned char)*text, px, py, color, back);
#else
		kgt_PutChar((unsigned char)*text, px, py, color);
#endif
		px += 9;
		text++;
	}
}

#ifdef KGT_USE_PRINTF
#ifdef KGT_DRAW_BG
void kgt_printf(int px, int py, int color, int back, char *text, ...)
#else
void kgt_printf(int px, int py, int color, char *text, ...)
#endif
{
	va_list argptr;
	char outtext[512];

	va_start(argptr, text);
	vsprintf(outtext, text, argptr);
	va_end(argptr);
#ifdef KGT_DRAW_BG
	kgt_write(outtext, px, py, color, back);
#else
	kgt_write(outtext, px, py, color);
#endif
}
#endif

void kgt_clear(int color)
{
	int *pixel = (unsigned int *)VRAM;
	int i;

	for(i = 0; i < 512 * 272; i++) {
		*pixel = color;
		pixel++;
	}
}

void kgt_clearbox(int x, int y, int w, int h, int color)
{
	int cx, cy;
	unsigned int *pixel;

	for(cy = y; cy < y + h; cy++) {
		pixel = (unsigned int *)VRAM;
		pixel += cy * 512 + x;
		for(cx = x; cx < x + w; cx++) {
			*pixel = color;
			pixel++;
		}
	}
}

