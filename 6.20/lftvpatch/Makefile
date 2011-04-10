TARGET = lftvpatch
OBJS = main.o log.o stub.o
VPATH = ../lib

INCDIR = ../rebootex/systemctrl ../lib

CFLAGS = -O2 -G0 -Wall -mno-gpopt -Wno-unused
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LDFLAGS =
LIBS =

PSP_FW_VERSION = 371

BUILD_PRX = 1

PRX_EXPORTS = exports.exp

LIBDIR =
LDFLAGS = -nodefaultlibs -nostdlib

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

