#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "main.h"

struct Processor cpu;
struct Storage code;
struct Storage stack;

int fexists(const char *filename) {
	struct stat st;
	return stat(filename, &st);
}

long fsize(FILE *fp) {
	long size;
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	return size;
}

word mem_read32(struct Storage *storage, int offset) {
	byte *p = storage->mem;
	return (
		(p[offset+0] << 0x18) |
		(p[offset+1] << 0x10) |
		(p[offset+2] << 0x08) |
		(p[offset+3]));
}

dword mem_read64(struct Storage *storage, int offset) {
	byte *p = storage->mem;
	return (
		((dword)p[offset+0] << 0x38) |
		((dword)p[offset+1] << 0x30) |
		((dword)p[offset+2] << 0x28) |
		((dword)p[offset+3] << 0x20) |
		((dword)p[offset+4] << 0x18) |
		((dword)p[offset+5] << 0x10) |
		((dword)p[offset+6] << 0x08) |
		((dword)p[offset+7]));
}

void mem_write64(struct Storage *storage, int offset, dword v) {
	byte *p = storage->mem;
	p[offset+0] = ((v >> 56) & 0xff);
	p[offset+1] = ((v >> 48) & 0xff);
	p[offset+2] = ((v >> 40) & 0xff);
	p[offset+3] = ((v >> 32) & 0xff);
	p[offset+4] = ((v >> 24) & 0xff);
	p[offset+5] = ((v >> 16) & 0xff);
	p[offset+6] = ((v >> 8 ) & 0xff);
	p[offset+7] = ((v >> 0 ) & 0xff);
}

void set_nia(struct Processor *cpu, dword addr) {
	if(addr != 0) // for debug
		cpu->nip = addr;
}

byte load_opcd(word code) {
	return ((code >> (31-6+1)) & 0x00000fff);
}

void load_inst(union inst_t *inst, word code) {
	byte opcd = load_opcd(code);
	switch(opcd) {
		case 16:
			// B-Form
			inst->b.opcd	= ((code >> (32-6 )) & 0x0000007f);
			inst->b.bo		= ((code >> (32-11)) & 0x0000001f);
			inst->b.bi		= ((code >> (32-16)) & 0x0000001f);
			inst->b.bd		= ((code >> (32-30)) & 0x00003fff);
			inst->b.aa		= ((code >> (32-31)) & 0x00000001);
			inst->b.lk		= ((code >> (32-32)) & 0x00000001);
			break;
		case 17:
			// SC-Form
			inst->sc.opcd	= ((code >> (32-6 )) & 0x0000007f);
			inst->sc.lev	= ((code >> (32-27)) & 0x0000007f);
			inst->sc.one	= ((code >> (32-31)) & 0x00000001);
			break;
		case 14:
		case 15:
		case 32:
			// D-Form
			inst->d.opcd	= ((code >> (32-6 )) & 0x0000007f);
			inst->d.rt		= ((code >> (32-11)) & 0x0000001f);
			inst->d.ra		= ((code >> (32-16)) & 0x0000001f);
			inst->d.d		= ((code >> (32-32)) & 0x0000ffff);
			inst->d.d = (inst->d.d << 16) >> 16;
			break;
		case 58:
		case 62:
			// DS-Form
			inst->ds.opcd	= ((code >> (32-6 )) & 0x0000007f);
			inst->ds.rt		= ((code >> (32-11)) & 0x0000001f);
			inst->ds.ra		= ((code >> (32-16)) & 0x0000001f);
			inst->ds.ds		= ((code >> (32-30)) & 0x00003fff);
			inst->ds.xo		= ((code >> (32-32)) & 0x00000003);
			inst->ds.ds = (inst->ds.ds << 18) >> 18;
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
	word code = mem_read32(storage, offset);
	byte opcd = load_opcd(code);
	union inst_t inst;
	load_inst(&inst, code);
	
	switch(opcd) {
		case 14:
			if(inst.d.ra == 0) {
				sprintf(asmcode, "li\t\tr%d,%d", inst.d.rt, inst.d.d);
			} else if(inst.d.d < 0) {
				sprintf(asmcode, "subi\tr%d,r%d,%d", inst.d.rt, inst.d.ra, -inst.d.d);
			} else {
				sprintf(asmcode, "addi\tr%d,r%d,%d", inst.d.rt, inst.d.ra, inst.d.d);
			}
			break;
		case 15:
			if(inst.d.ra == 0) {
				sprintf(asmcode, "lis\tr%d,%d", inst.d.rt, inst.d.d);
			} else if(inst.d.d < 0) {
				sprintf(asmcode, "subis\tr%d,r%d,%d", inst.d.rt, inst.d.ra, -inst.d.d);
			} else {
				sprintf(asmcode, "addis\tr%d,r%d,%d", inst.d.rt, inst.d.ra, inst.d.d);
			}
			break;
		case 16:
			if(inst.b.bo == 12 && inst.b.bi == 0) {
				sprintf(asmcode, "blt\t%x", !inst.b.aa ? (((inst.b.bd << 18) >> 16) + offset) : ((inst.b.bd << 18) >> 16));
			} else if(inst.b.bo == 4 && inst.b.bi == 10) {
				sprintf(asmcode, "bne\t%x", !inst.b.aa ? (((inst.b.bd << 18) >> 16) + offset) : ((inst.b.bd << 18) >> 16));
			} else if(inst.b.bo == 16 && inst.b.bi == 0) {
				sprintf(asmcode, "bdnz\t%x", !inst.b.aa ? (((inst.b.bd << 18) >> 16) + offset) : ((inst.b.bd << 18) >> 16));
			} else if(inst.b.aa == 0 && inst.b.lk == 0) {
				sprintf(asmcode, "bc\t%d,%d,%x", inst.b.bo, inst.b.bi, ((inst.b.bd << 18) >> 16) + offset);
			} else if(inst.b.aa == 0 && inst.b.lk == 1) {
				sprintf(asmcode, "bca\t%d,%d,%x", inst.b.bo, inst.b.bi, ((inst.b.bd << 18) >> 16) + offset);
			} else if(inst.b.aa == 1 && inst.b.lk == 0) {
				sprintf(asmcode, "bcl\t%d,%d,%x", inst.b.bo, inst.b.bi, ((inst.b.bd << 18) >> 16));
			} else if(inst.b.aa == 1 && inst.b.lk == 1) {
				sprintf(asmcode, "bcla\t%d,%d,%x", inst.b.bo, inst.b.bi, ((inst.b.bd << 18) >> 16));
			}
			break;
		case 17:
			if(inst.sc.one == 1) {
				sprintf(asmcode, "sc");
			} else {
				sprintf(asmcode, "? ; syscall");
			}
			break;
		case 19:
			if(inst.xl.bt == 12) {
				sprintf(asmcode, "bltlr");
			} else if(inst.xl.bt == 4 && inst.xl.ba == 10) {
				sprintf(asmcode, "bnelr\tcr2");
			} else if(inst.xl.bt == 16) {
				sprintf(asmcode, "bdnzlr");
			} else if(inst.xl.lk == 0) {
				sprintf(asmcode, "bclr");
			} else if(inst.xl.lk == 1) {
				sprintf(asmcode, "bclrl");
			}
			break;
		case 31:
			switch(inst.x.xo) {
				case 444:
					if(inst.x.rt == inst.x.rb) {
						sprintf(asmcode, "mr\t\tr%d,r%d", inst.x.ra, inst.x.rt);
					} else {
						sprintf(asmcode, "or\t\tr%d,r%d,r%d", inst.x.ra, inst.x.rt, inst.x.rb);
					}
					break;
				case 467:
					switch(inst.x.rb << 5 | inst.x.ra) {
						case 1:
							sprintf(asmcode, "mtxer\tr%d", inst.x.rt);
							break;
						case 8:
							sprintf(asmcode, "mtlr\tr%d", inst.x.rt);
							break;
						case 9:
							sprintf(asmcode, "mtctr\tr%d", inst.x.rt);
							break;
					}
					break;
			}
			break;
		case 32:
			sprintf(asmcode, "lwz\tr%d,%d(r%d)", inst.d.rt, inst.d.d, inst.d.ra);
			break;
		case 58:
			if(inst.ds.xo == 0) {
				sprintf(asmcode, "ld\t\tr%d,%d(r%d)", inst.ds.rt, inst.ds.ds << 2, inst.ds.ra);
			} else if(inst.ds.xo == 1) {
				sprintf(asmcode, "ldu\tr%d,%d(r%d)", inst.ds.rt, inst.ds.ds << 2, inst.ds.ra);
			}
			break;
		case 62:
			if(inst.ds.xo == 0) {
				sprintf(asmcode, "std\tr%d,%d(r%d)", inst.ds.rt, inst.ds.ds << 2, inst.ds.ra);
			} else if(inst.ds.xo == 1) {
				sprintf(asmcode, "stdu\tr%d,%d(r%d)", inst.ds.rt, inst.ds.ds << 2, inst.ds.ra);
			}
			break;
		default:
			strcpy(asmcode, "?");
	}

	return asmcode;
}

int exec(struct Storage *storage, int offset) {
	const int mode = 64;
	word code = mem_read32(storage, offset);
	byte opcd = load_opcd(code);
	union inst_t inst;
	load_inst(&inst, code);
	
	switch(opcd) {
		case 14:
			// Add-Immediate
			if(inst.d.ra == 0) {
				cpu.gpr[inst.d.rt] = inst.d.d;
			} else {
				cpu.gpr[inst.d.rt] = cpu.gpr[inst.d.ra] + inst.d.d;
			}
			break;
		case 15:
			// Add-Immediate-Shifted
			if(inst.d.ra == 0) {
				cpu.gpr[inst.d.rt] = inst.d.d << 16;
			} else {
				cpu.gpr[inst.d.rt] = inst.d.ra + (inst.d.d << 16);
			}
			break;
		case 16:
			{
				// Branch
				dword mask = (mode == 64) ? 0xffffffffffffffff : 0x00000000ffffffff;
				if(!((inst.b.bo >> 2) & 0x01)) { cpu.ctr--; }
				int ctr_ok = ((inst.b.bo >> 2) & 0x01) | (((cpu.ctr & mask) != 0) ^ ((inst.b.bo >> 1) & 0x01));
				int cond_ok = ((inst.b.bo >> 4) & 0x01) | (((cpu.cr >> (63-32-inst.b.bi)) & 0x01) == ((inst.b.bo >> 3) & 0x01));
				if(ctr_ok && cond_ok) {
					if(inst.b.aa) {
						set_nia(&cpu, mem_read64(&stack, inst.b.bd << 2));
					} else {
						int cia = cpu.nip; // Current-Instruction-Address
						set_nia(&cpu, mem_read64(&stack, cia + (inst.b.bd << 2)));
					}
				}
				if(inst.b.lk) { cpu.lr = mem_read64(&stack, cpu.nip + 4); }
				break;
			}
		case 19:
			{
				// Branch-Conditional
				int nia = cpu.nip + 4;
				if((inst.xl.bt & 0x04) != 0x04) { cpu.ctr--; }
				dword mask = (mode == 64) ? 0xffffffffffffffff : 0x00000000ffffffff;
				int ctr_ok = ((inst.xl.bt & 0x04) == 0x04) || (((cpu.ctr & mask) != 0) ^ ((inst.xl.bt >> 1) & 0x01));
				int cond_ok = ((inst.xl.bt >> 4) & 0x01) || (((cpu.cr >> (63-32-inst.xl.ba)) & 0x01) == ((inst.xl.bt >> 3) & 0x01));
				if(ctr_ok && cond_ok) { set_nia(&cpu, ((cpu.lr >> 2) << 2)); }
				if(inst.xl.lk == 1) { cpu.lr = nia; }
				break;
			}
		case 31:
			switch(inst.x.xo) {
				case 444:
					// OR
					cpu.gpr[inst.x.ra] = cpu.gpr[inst.x.rt] | cpu.gpr[inst.x.rb];
					break;
				case 467:
					// Move-To-Special-Purpose-Register
					switch(inst.x.rb << 5 | inst.x.ra) {
						case 1:
							cpu.xer = cpu.gpr[inst.x.rt];
							break;
						case 8:
							cpu.lr = cpu.gpr[inst.x.rt];
							break;
						case 9:
							cpu.ctr = cpu.gpr[inst.x.rt];
							break;
					}
					break;
			}
			break;
		case 32:
			// Load-Word
			cpu.gpr[inst.d.rt] = mem_read32(&stack, ((inst.d.ra == 0) ? 0 : inst.d.ra) + inst.d.d);
			break;
		case 58:
			// Load-Doubleword
			if(inst.ds.xo == 0) {
				if(inst.ds.ra == 0) {
					cpu.gpr[inst.ds.rt] = mem_read64(&stack, inst.ds.ds << 2);
				} else {
					cpu.gpr[inst.ds.rt] = mem_read64(&stack, cpu.gpr[inst.ds.ra] + (inst.ds.ds << 2));
				}
			} else if(inst.ds.xo == 1) {
				if(inst.ds.ra == 0 || inst.ds.ra == inst.ds.rt) { return -1; }
				cpu.gpr[inst.ds.rt] = mem_read64(&stack, cpu.gpr[inst.ds.ra] + (inst.ds.ds << 2));
				cpu.gpr[inst.ds.ra] = cpu.gpr[inst.ds.ra] + (inst.ds.ds << 2);
			}
			break;
		case 62:
			// STore-Doubleword
			mem_write64(&stack, cpu.gpr[inst.ds.ra] + (inst.ds.ds << 2), cpu.gpr[inst.ds.rt]);
			if(inst.ds.xo == 1) {
				cpu.gpr[inst.ds.ra] = cpu.gpr[inst.ds.ra] + (inst.ds.ds << 2);
			}
			break;
		default:
			return -1;
	}
	return 0;
}

int main(int argc, char *argv[]) {
	char filename[128];
	if((argc == 2 && fexists(argv[1]))
	|| (argc == 3 && (strncmp(argv[1], "-d", 2) == 0) && fexists(argv[2]))) {
		fprintf(stderr, "execute\t: %s [filename]\n", argv[0]);
		fprintf(stderr, "disas\t: %s -d [filename]\n", argv[0]);
		return EXIT_FAILURE;
	} else {
		strncpy(filename, argv[argc-1], 127);
	}

	FILE *fp = NULL;
	if((fp = fopen(filename, "rb")) == NULL) {
		fprintf(stderr, "error: no such file '%s'\n", filename);
		return EXIT_FAILURE;
	}

	code.size = fsize(fp);
	code.mem = (void *)malloc(code.size);
	code.offset_addr = 0;
	if(code.mem == NULL) {
		fprintf(stderr, "error: out of memory\n");
		fclose(fp);
		return EXIT_FAILURE;
	} else {
		printf("code size = %ld\n", code.size);
		fread(code.mem, sizeof(byte), code.size, fp);
	}

	if(argc == 2) {
		// exec-mode
		stack.size = STACK_SIZE;
		stack.mem = (void *)malloc(stack.size);
		stack.offset_addr = 0;
		if(stack.mem == NULL) {
			fprintf(stderr, "error: out of memory\n");
			fclose(fp);
			free(code.mem);
			return EXIT_FAILURE;
		} else {
			printf("stack size = %ld\n", stack.size);
			cpu.gpr[1] = STACK_SIZE / 2;
		}

		// execution loop
		for(cpu.nip = 0; cpu.nip < code.size; cpu.nip += 4) {
			word c = mem_read32(&code, cpu.nip);
			if(exec(&code, cpu.nip) == -1) {
				char asmcode[32] = {0};
				disas(&code, cpu.nip, asmcode);
				fprintf(stderr, "warning: undefined instruction `%s`\n", asmcode);
			}
		}

		// REPL-mode
		char command[32];
		printf(PROMPT);
		while(fgets(command, 31, stdin) != 0) {
			if(command[0] == '%') {
				if(command[1] == 'r') {
					int reg_n = atoi(command + 2);
					printf("r%d=0x%016lx\n", reg_n, cpu.gpr[reg_n]);
				}
			} else if(strncmp(command, "exit", 4) == 0 || strncmp(command, "quit", 4) == 0) {
				break;
			} else {
				fprintf(stderr, "error: unknown command\n");
			}
			printf(PROMPT);
		}

		free(stack.mem); stack.mem = NULL;
	} else {
		// disassemble-mode
		for(cpu.nip = 0; cpu.nip < code.size; cpu.nip += 4) {
			word c = mem_read32(&code, cpu.nip);
			char asmcode[32] = {0};
			disas(&code, cpu.nip, asmcode);
			printf("%4lx: %02x %02x %02x %02x    %s\n",
					cpu.nip,
					(c >> 24) & 0xff,
					(c >> 16) & 0xff,
					(c >> 8) & 0xff,
					c & 0xff, asmcode);
		}
	}

	free(code.mem); code.mem = NULL;
	fclose(fp);

	return EXIT_SUCCESS;
}
