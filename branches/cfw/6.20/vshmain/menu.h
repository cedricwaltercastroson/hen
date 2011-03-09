#include <stdio.h>

#ifndef __RMENU__H_
#define __RMENU__H_

typedef struct {
	char* text;
	int x;
	void (*func)(int);
} menuitem_t;

typedef struct menu_s {
	char *name;
	int nx;
	void (*draw)(void);
	menuitem_t *items;
	int numitems;
	struct menu_s *prev;
	int curitem;
} menu_t;

void SelectMain(int way);
void GoBack(int way);
void ChangeOption(int way);
void EnableFlash(int way);
void EnableWMA(int way);
void SwapButtons(int way);
void SelectPluginMenu(int way);
void TogglePlugin(int way);
void FlashPlugin(int way);
void FlashUpdate(int way);

void DrawOptions();
void DrawPlugins();
void DrawFlash();

int ReadRegistry(const char *dir, const char *name, void *val, int len);
int WriteRegistry(const char *dir, const char *name, void *val, int len);

#endif
