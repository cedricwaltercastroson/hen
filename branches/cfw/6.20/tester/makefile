TARGET = tester
OBJS = main.o imports.o rebootex.o

INCDIR =
CFLAGS = -O2 -G0 -Wall -DDEBUG
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LDFLAGS =
LIBS = -lpsppower -lpsputility

PSP_FW_VERSION = 371

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Permanent Installer

BUILD_PRX = 1

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

rebootex.o: rebootex.bin
	bin2o -i $< rebootex.o rebootex

