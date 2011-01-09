TARGET = systemctrl
OBJS = main.o

INCDIR = 
CFLAGS = -O2 -G0 -Wall -fshort-wchar -fno-pic -mno-check-zero-division
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

PSP_FW_VERSION = 371

BUILD_PRX = 1
PRX_EXPORTS = exports.exp

USE_KERNEL_LIBC = 1
USE_KERNEL_LIBS = 1


LIBDIR =
LIBS = 
LDFLAGS = -nostdlib -nodefaultlibs

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
