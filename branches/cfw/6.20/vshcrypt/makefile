program = vsh-crypt
OBJ = main.o cmac.o vshdata.o
LIBS = -lz -lssl -lcrypto

OPT=-O2
CC=gcc
CFLAGS=${OPT}

.PHONY: build
.PHONY: clean

build: ${program}

clean:
	rm -f *.o ${program}

${program}: ${OBJ}
	${CC} ${OBJ} ${LIBS} -o ${program} ${OPT}

