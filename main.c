#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"

const char opcds[64][8] = {
	"", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "",
};

struct Processor cpu;
struct Storage code;

long fsize(FILE *fp) {
	long size;
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	return size;
}

dword mem_read32(struct Storage *storage, int offset) {
	byte *p = storage->mem;
	return (
		(p[offset+0] << 24) |
		(p[offset+1] << 16) |
		(p[offset+2] << 8) |
		(p[offset+3]));
}

byte load_opcd(dword code) {
	return ((code >> (31-6+1)) & 0x00000fff);
}

ds_form load_ds_form(dword code) {
	ds_form inst;
	inst.opcd	= ((code >> (31-6 +1)) & 0x00000fff);
	inst.rt		= ((code >> (31-11+1)) & 0x000007ff);
	inst.ra		= ((code >> (31-16+1)) & 0x000007ff);
	inst.ds		= ((code >> (31-30+1)) & 0x0fffffff);
	inst.xo		= ((code >> (31-31+1)) & 0x00000007);
	return inst;
}

int main(int argc, char *argv[]) {
	if(argc != 2) {
		fprintf(stderr, "usage: %s [filename]\n", argv[0]);
		return EXIT_FAILURE;
	}

	FILE *fp = NULL;
	if((fp = fopen(argv[1], "rb")) == NULL) {
		fprintf(stderr, "error: no such file '%s'\n", argv[1]);
		return EXIT_FAILURE;
	}

	code.size = fsize(fp);
	code.mem =(void *) malloc(code.size);
	if(code.mem == NULL) {
		fprintf(stderr, "error: out of memory\n");
		return EXIT_FAILURE;
	} else {
		printf("size = %ld\n", code.size);
		fread(code.mem, sizeof(byte), code.size, fp);
	}

	int i;
	for(i = 0; i*4 < code.size; i++) {
		printf("op: %d\n", load_opcd(mem_read32(&code, i*4)));
	}

	free(code.mem);
	fclose(fp);

	return EXIT_SUCCESS;
}
