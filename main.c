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

void load_inst(union inst_t *inst, dword code) {
	byte opcd = load_opcd(code);
	switch(opcd) {
		case 14:
		case 15:
		case 32:
			// D-Form
			inst->d.opcd	= ((code >> (32-6 )) & 0x0000007f);
			inst->d.rt		= ((code >> (32-11)) & 0x0000001f);
			inst->d.ra		= ((code >> (32-16)) & 0x0000001f);
			inst->d.d		= ((code >> (32-32)) & 0x0000ffff);
			break;
		case 58:
		case 62:
			// DS-Form
			inst->ds.opcd	= ((code >> (32-6 )) & 0x0000007f);
			inst->ds.rt		= ((code >> (32-11)) & 0x0000001f);
			inst->ds.ra		= ((code >> (32-16)) & 0x0000001f);
			inst->ds.ds		= ((code >> (32-30)) & 0x00003fff);
			inst->ds.xo		= ((code >> (32-31)) & 0x00000003);
			break;
		case 31:
			// X-Form
			inst->x.opcd	= ((code >> (32-6 )) & 0x0000007f);
			inst->x.rt		= ((code >> (32-11)) & 0x0000001f);
			inst->x.ra		= ((code >> (32-16)) & 0x0000001f);
			inst->x.rb		= ((code >> (32-21)) & 0x0000001f);
			inst->x.xo		= ((code >> (32-31)) & 0x000003ff);
			inst->x.eh		= ((code >> (32-32)) & 0x00000001);
			break;
		case 19:
			// XL-Form
			inst->xl.opcd	= ((code >> (32-6 )) & 0x0000007f);
			inst->xl.bt		= ((code >> (32-11)) & 0x0000001f);
			inst->xl.ba		= ((code >> (32-16)) & 0x0000001f);
			inst->xl.bb		= ((code >> (32-21)) & 0x0000001f);
			inst->xl.xo		= ((code >> (32-31)) & 0x000003ff);
			inst->xl.lk		= ((code >> (32-32)) & 0x00000001);
			break;
	}
}

char *disas(struct Storage *storage, int offset, char *asmcode) {
	dword code = mem_read32(storage, offset);
	byte opcd = load_opcd(code);
	union inst_t inst;
	load_inst(&inst, code);
	
	switch(opcd) {
		case 14:
			if(inst.d.ra == 0) {
				sprintf(asmcode, "li\tr%d,%d", inst.d.rt, inst.d.d);
			} else {
				sprintf(asmcode, "addi\tr%d,r%d,%d", inst.d.rt, inst.d.ra, inst.d.d);
			}
			break;
		case 15:
			sprintf(asmcode, "addis\tr%d,r%d,%d", inst.d.rt, inst.d.ra, inst.d.d);
			break;
		case 19:
			if(inst.xl.lk == 0) {
				sprintf(asmcode, "bclr");
			} else if(inst.xl.lk == 1) {
				sprintf(asmcode, "bclrl");
			}
			break;
		case 31:
			switch(inst.x.xo) {
				case 444:
					if(inst.x.rt == inst.x.rb) {
						sprintf(asmcode, "mr\tr%d,r%d", inst.x.ra, inst.x.rt);
					} else {
						sprintf(asmcode, "or\tr%d,r%d,r%d", inst.x.ra, inst.x.rt, inst.x.rb);
					}
					break;
			}
			break;
		case 32:
			sprintf(asmcode, "lwz\tr%d,%d(r%d)", inst.d.rt, inst.d.d, inst.d.ra);
			break;
		case 58:
			if(inst.ds.xo == 0) {
				sprintf(asmcode, "ld\tr%d,%d(r%d)", inst.ds.rt, inst.ds.ds, inst.ds.ra);
			} else if(inst.ds.xo == 1) {
				sprintf(asmcode, "ldu\tr%d,%d(r%d)", inst.ds.rt, inst.ds.ds, inst.ds.ra);
			}
			break;
		case 62:
			if(inst.ds.xo == 0) {
				sprintf(asmcode, "std\tr%d,%d(r%d)", inst.ds.rt, inst.ds.ds, inst.ds.ra);
			} else if(inst.ds.xo == 1) {
				sprintf(asmcode, "stdu\tr%d,%d(r%d)", inst.ds.rt, inst.ds.ds, inst.ds.ra);
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
		dword c = mem_read32(&code, i*4);
		disas(&code, i*4, asmcode);
		printf(" %02x %02x %02x %02x    %s\n",
				(c >> 24) & 0xff,
				(c >> 16) & 0xff,
				(c >> 8) & 0xff,
				c & 0xff, asmcode);
	}

	free(code.mem);
	fclose(fp);

	return EXIT_SUCCESS;
}
