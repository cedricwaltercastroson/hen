PSPSDK=$(shell psp-config --pspsdk-path)
PSPDEV=$(shell psp-config --pspdev-path)

INCLUDE = -I. -I$(PSPSDK)/include

CC = psp-gcc
CFLAGS = -W -Wall -O2 -G0 -fno-pic -mno-abicalls -w -fomit-frame-pointer
CFLAGS += $(INCLUDE)

all: rebootex.bin

clean:
	rm -rf *.bin *.elf *.o rebootex.s

rebootex.bin: rebootex.c linker.x
	$(CC) $(CFLAGS) -c rebootex.c
	$(PSPDEV)/bin/psp-ld -T linker.x rebootex.o -o rebootex.elf
	$(PSPDEV)/bin/psp-strip -s rebootex.elf
	$(PSPDEV)/bin/psp-objcopy -O binary rebootex.elf rebootex.bin
	$(PSPDEV)/bin/bin2c rebootex.bin rebootex_bin.inc rebootex_bin
	cp rebootex_bin.inc ..
