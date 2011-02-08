TARGET = systemctrl
OBJS = main.o stub.o malloc.o kubridge.o systemctrl.o

INCDIR = 
CFLAGS = -O2 -G0 -Wall -fshort-wchar -fno-pic -mno-check-zero-division
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

PSP_FW_VERSION = 371

BUILD_PRX = 1
PRX_EXPORTS = exports.exp

USE_KERNEL_LIBC = 1
USE_KERNEL_LIBS = 1

all: sysctrl_bin.inc

sysctrl_bin.inc: systemctrl.prx
	psp-packer systemctrl.prx sysctrl.psp
	bin2c sysctrl.psp sysctrl_bin.inc sysctrl_bin
	cp sysctrl_bin.inc ..

LIBDIR =
LIBS = 
LDFLAGS = -nostdlib -nodefaultlibs -e module_bootstart

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
