TARGET = install
OBJS = main.o imports.o kgtext.o font.o vshmain.o sysctrl.o vshmenu.o

INCDIR =
CFLAGS = -O2 -G0 -Wall -DDEBUG
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LDFLAGS =
LIBS =

PSP_FW_VERSION = 371

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = 6.20a CFW Installer
PSP_EBOOT_ICON = icon0.png

BUILD_PRX = 1

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

font.o: font.bin
	bin2o -i $< font.o font

vshmain.o: vshmain.prx
	bin2o -i $< vshmain.o vshmain

sysctrl.o: sysctrl.prx
	bin2o -i $< sysctrl.o sysctrl

vshmenu.o: vshmenu.prx
	bin2o -i $< vshmenu.o vshmenu

