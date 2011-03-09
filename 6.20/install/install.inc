extern unsigned char sysctrl_start[];
extern int sysctrl_size;
extern unsigned char vshmenu_start[];
extern int vshmenu_size;

typedef struct {
	char *name;
	char *path;
	void *buff;
	int *size;
} install_item_t;

install_item_t iitems[] = {
	{"SystemControl", "flash0:/kd/sysctrl.prx", sysctrl_start, &sysctrl_size},
	{"VshControl", "flash0:/vsh/module/vshmenu.prx", vshmenu_start, &vshmenu_size}
};

#define NUMITEMS (sizeof(iitems) / sizeof(install_item_t))

