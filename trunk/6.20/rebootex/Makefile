PSPSDK=$(shell psp-config --pspsdk-path)
PSPDEV=$(shell psp-config --pspdev-path)

INCLUDE =

CC = psp-gcc
CFLAGS := -I. -W -Wall -O2 -G0 -fno-pic -mno-abicalls -w -fomit-frame-pointer

clean:
	rm -rf *.bin *.elf *.o rebootex.s

all: rebootex.bin

rebootex.bin: rebootex.c linker.x
	$(CC) $(CFLAGS) -c rebootex.c
	$(PSPDEV)/bin/psp-ld -T linker.x rebootex.o -o rebootex.elf
	$(PSPDEV)/bin/psp-strip -s rebootex.elf
	$(PSPDEV)/bin/psp-objcopy -O binary rebootex.elf rebootex.bin
	prxtool -o rebootex.s -b -i m -w rebootex.bin
