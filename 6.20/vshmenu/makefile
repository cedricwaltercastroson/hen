TARGET = satelite
OBJS = main.o stub.o font.o

INCDIR = ../rebootex/systemctrl

CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LDFLAGS =
LIBS =

PSP_FW_VERSION = 371

BUILD_PRX = 1

PRX_EXPORTS = exports.exp

all: vshmenu.prx

vshmenu.prx: satelite.prx
	cp satelite.prx ../install/vshmenu.prx

LIBDIR =
LDFLAGS = -nodefaultlibs -nostdlib

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

