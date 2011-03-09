/*
	kgsws's universal fake encrypter
	(without relocation type 7 removal)
*/

#include <stdio.h>
#include <openssl/aes.h>
#include <openssl/sha.h>
#include "cmac.h"

typedef unsigned char byte;

// original ~PSP header and kirk data stored in one buffer
extern int vshdata_size;
extern byte vshdata_start[];

typedef struct {
	byte key[16];
	byte ckey[16];
	byte head_hash[16];
	byte data_hash[16];
	byte unused[32];
	int unk1; // 1
	int unk2; // 0
	int unk3[2];
	int datasize;
	int dataoffset;
	int unk4[6];
} kirk1head_t;

// secret kirk command 1 key
byte kirk_key[] = {
	0x98, 0xc9, 0x40, 0x97, 0x5c, 0x1d, 0x10, 0xe8, 0x7f, 0xe6, 0x0e, 0xa3, 0xfd, 0x03, 0xa8, 0xba
};

int main(int argc, char **argv)
{
	int i, j, size, fullsize, blocks, datasize;
	kirk1head_t *kirk = vshdata_start;
	byte iv[16];
	byte cmac[32];
	byte subk[32];
	byte *psph = &vshdata_start[0x90];
	byte *datablob;
	byte *filebuff;
	FILE *f;
	AES_KEY aesKey;

	if(argc != 3) {
		printf("Usage: %s in.prx out.prx\n", argv[0]);
		return 1;
	}

	datasize = kirk->datasize;
	if(datasize % 16) datasize += 16 - (datasize % 16);

	// file to encrypt
	f = fopen(argv[1], "rb");
	if(!f) {
		printf("failed to open %s\n", argv[1]);
		return 1;
	}
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	if(size > datasize - 16) {
		fclose(f);
		printf("%s is too big\n", argv[1]);
		return 1;
	}
	printf("%s : %i\n", argv[1], size);
	fseek(f, 0, SEEK_SET);

	fullsize = datasize + 0x30 + kirk->dataoffset;

	// datablob holds everything needed to calculate data HASH
	datablob = malloc(fullsize);
	if(!datablob) {
		fclose(f);
		printf("failed to allocate memory for blob\n");
		return 1;
	}
	memset(datablob, 0, fullsize);
	memcpy(datablob, &kirk->unk1, 0x30);
	memcpy(datablob + 0x30, psph, kirk->dataoffset);
	filebuff = datablob + 0x30 + kirk->dataoffset;

	fread(filebuff, 1, size, f);
	fclose(f);

	// get AES/CMAC key
	AES_set_decrypt_key(kirk_key, 128, &aesKey);
	memset(iv, 0, 16);
	AES_cbc_encrypt(kirk->key, kirk->key, 32, &aesKey, iv, AES_DECRYPT);

	// check header hash, optional
	// if you take correct kirk header, hash is always correct
/*	AES_CMAC(kirk->ckey, datablob, 0x30, cmac);
	if(memcmp(cmac, kirk->head_hash, 16)) {
		free(datablob);
		printf("header hash invalid\n");
		return 1;
	}
*/

	// encrypt input file
	AES_set_encrypt_key(kirk->key, 128, &aesKey);
	memset(iv, 0, 16);
	AES_cbc_encrypt(filebuff, filebuff, datasize, &aesKey, iv, AES_ENCRYPT);

	// make CMAC correct
	generate_subkey(kirk->ckey, subk, subk + 16);
	AES_set_encrypt_key(kirk->ckey, 128, &aesKey);
	blocks = fullsize / 16;
	memset(cmac, 0, 16);
	for(i = 0; i < blocks - 1; i++) {
		xor_128(cmac, &datablob[16 * i], cmac + 16);
		AES_encrypt(cmac + 16, cmac, &aesKey);
	}

	AES_set_decrypt_key(kirk->ckey, 128, &aesKey);
	AES_decrypt(kirk->data_hash, iv, &aesKey);
	xor_128(cmac, iv, iv);
	xor_128(iv, subk, &datablob[16 * (blocks-1)]);
	// check it, optional
	// it works, this is only if you want to change something
/*	AES_CMAC(kirk->ckey, datablob, fullsize, cmac);
	if(memcmp(cmac, kirk->data_hash, 16)) {
		fclose(f);
		free(datablob);
		printf("data hash calculation error\n");
		return 1;
	}
*/
	f = fopen(argv[2], "wb");
	if(!f) {
		free(datablob);
		printf("failed to write out.prx\n");
		return 1;
	}
	printf("saving ...\n");
	// save ~PSP header
	fwrite(psph, 1, 0x150, f);
	// save encrypted file
	fwrite(filebuff, 1, fullsize - 0x30 - kirk->dataoffset, f);
	fclose(f);
	free(datablob);
	printf("everything done\n");
	return 0;
}

