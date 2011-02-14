TARGET = satelite
OBJS = main.o

INCDIR = ../
CFLAGS = -O2 -G0 -Wall -DDEBUG -mno-gpopt
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LDFLAGS =
LIBS =

PSP_FW_VERSION = 371

BUILD_PRX = 1

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

