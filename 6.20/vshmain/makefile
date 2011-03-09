TARGET = fakevsh
OBJS = main.o kgtext.o menu.o imports.o rebootex.o font.o

INCDIR =
CFLAGS = -O2 -G0 -Wall -DDEBUG
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LDFLAGS =
LIBS = -lpspreg

PSP_FW_VERSION = 371

BUILD_PRX = 1

all: vshmain.prx

# lol lol, but it works
vshmain.prx: fakevsh.prx
	cp fakevsh.prx vshmain.p0
	cat vshmain.p* > vshmain.bin
	split -b 391096 vshmain.bin vshmain.
	gzip -9 -n vshmain.aa
	vsh-crypt vshmain.aa.gz vshmain.prx
	rm vshmain.a*
	rm vshmain.p0
	rm vshmain.bin
	cp vshmain.prx ../install

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

rebootex.o: rebootex.bin
	bin2o -i $< rebootex.o rebootex

font.o: font.bin
	bin2o -i $< font.o font

