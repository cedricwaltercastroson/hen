#include "pspiofilemgr.h"
#include "pspiofilemgr_fcntl.h"

/* from HBL */

int
my_strlen(const char *s)
{
	int i = 0;

	while (*s) {
		s++;
		i++;
	}

	return i;
}

void
my_strcpy(char *d, char *s)
{
	while (*s) {
		*d = *s;
		d++;
		s++;
	}

	*d = '\0';
}

void
numtohex8(char *dst, int n)
{
	static char hex[] = "0123456789ABCDEF";

	int i;

	for (i = 0; i < 8; i++)
		dst[i] = hex[(n>>((7-i)*4))&15];
}

static SceUID
openlog(void)
{
	return sceIoOpen("ms0:/MYLOG.TXT", PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0666);
}

static void
closelog(SceUID fd)
{
	sceIoClose(fd);
}

void
logint(int val)
{
	char buf[512] = {0};
	SceUID fd = openlog();

	numtohex8(buf, val);
	buf[8] = '\r';
	buf[9] = '\n';
	buf[10] = '\0';
	sceIoWrite(fd, buf, my_strlen(buf));
	closelog(fd);
}

void
logstr(char *s)
{
	char buf[512] = {0};
	SceUID fd = openlog();
	int len = my_strlen(s);

	my_strcpy(buf, s);
	buf[len] = '\r';
	buf[len + 1] = '\n';
	buf[len + 2] = '\0';
	sceIoWrite(fd, buf, my_strlen(buf));
	closelog(fd);
}
