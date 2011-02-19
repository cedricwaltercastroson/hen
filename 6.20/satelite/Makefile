TARGET = satelite
OBJS = main.o stub.o

INCDIR = ../rebootex/systemctrl

CFLAGS = -O2 -G0 -Wall -mno-gpopt
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LDFLAGS =
LIBS =

PSP_FW_VERSION = 371

BUILD_PRX = 1

PRX_EXPORTS = exports.exp

LIBDIR =
LDFLAGS =

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

