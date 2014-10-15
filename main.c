#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"

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

ds_form_t load_ds_form(dword code) {
	ds_form_t inst;
	inst.opcd	= ((code >> (31-6 +1)) & 0x0000007f);
	inst.rt		= ((code >> (31-11+1)) & 0x0000001f);
	inst.ra		= ((code >> (31-16+1)) & 0x0000001f);
	inst.ds		= ((code >> (31-30+1)) & 0x00003fff);
	inst.xo		= ((code >> (31-31+1)) & 0x00000003);
	return inst;
}

char *disas(struct Storage *storage, int offset, char *asmcode) {
	dword code = mem_read32(storage, offset);
	byte opcd = load_opcd(code);
	ds_form_t inst;
	
	switch(opcd) {
		case 14:
			inst = load_ds_form(code);
			if(inst.ra == 0) {
				sprintf(asmcode, "li r%d,%d", inst.rt, inst.ds<<2|inst.xo);
			} else {
				sprintf(asmcode, "addi r%d,r%d,%d", inst.rt, inst.ra, inst.ds<<2|inst.xo);
			}
			break;
		case 15:
			inst = load_ds_form(code);
			sprintf(asmcode, "addis r%d,r%d,%d", inst.rt, inst.ra, inst.ds<<2|inst.xo);
			break;
		case 32:
			inst = load_ds_form(code);
			sprintf(asmcode, "lwz r%d,%d(r%d)", inst.rt, inst.ds<<2|inst.xo, inst.ra);
			break;
		case 58:
			inst = load_ds_form(code);
			if(inst.xo == 0) {
				sprintf(asmcode, "ld r%d,%d(r%d)", inst.rt, inst.ds, inst.ra);
			} else if(inst.xo == 1) {
				sprintf(asmcode, "ldu r%d,%d(r%d)", inst.rt, inst.ds, inst.ra);
			}
			break;
		case 62:
			inst = load_ds_form(code);
			if(inst.xo == 0) {
				sprintf(asmcode, "std r%d,%d(r%d)", inst.rt, inst.ds, inst.ra);
			} else if(inst.xo == 1) {
				sprintf(asmcode, "stdu r%d,%d(r%d)", inst.rt, inst.ds, inst.ra);
			}
			break;
		default:
			strcpy(asmcode, "?");
	}

	return asmcode;
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
		char asmcode[32] = {0};
		disas(&code, i*4, asmcode);
		printf("%x: %s\n", mem_read32(&code, i*4), asmcode);
	}

	free(code.mem);
	fclose(fp);

	return EXIT_SUCCESS;
}
