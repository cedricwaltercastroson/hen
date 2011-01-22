TARGET = hen
OBJS = main.o

INCDIR =
CFLAGS = -O2 -G0 -Wall -DDEBUG -mno-gpopt
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LDFLAGS =
LIBS = -lpsppower -lpsputility

PSP_FW_VERSION = 371

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = HEN

BUILD_PRX = 1
ENCRYPT = 1

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

