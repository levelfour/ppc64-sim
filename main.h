#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdio.h>

typedef unsigned long dword;
typedef unsigned int word;
typedef unsigned short hword;
typedef unsigned char byte;

byte mem_read8(byte *p, int offset);
hword mem_read16(byte *p, int offset);
word mem_read32(byte *p, int offset);
dword mem_read64(byte *p, int offset);
void mem_write64(byte *p, int offset, dword v);

#define PROMPT "> "

#define EI_NIDENT	16
#define SHT_SYMTAB	2
#define SHT_STRTAB	3
#define SHT_RELA	4

#define SEGMENT_SIZE	0xffffff
#define TEXT_OFFSET		0x000000
#define DATA_OFFSET		0x080000
#define STACK_OFFSET	0x200000

#define EXEC_UNDEFINED	-1
#define EXEC_SUCCESS	0
#define EXEC_EXIT		(1 << 8)

typedef struct {
	byte	e_idnet[EI_NIDENT];	/* ELF identification */
	hword	e_type;				/* Object file type */
	hword	e_machine;			/* Machine type */
	word	e_version;			/* Object file version */
	dword	e_entry;			/* Entry point address */
	dword	e_phoff;			/* Program header offset */
	dword	e_shoff;			/* Section header offset */
	word	e_flags;			/* Processor-specific flags */
	hword	e_ehsize;			/* ELF header size */
	hword	e_phentsize;		/* Size of program header entry */
	hword	e_phnum;			/* Number of program header entries */
	hword	e_shentsize;		/* Size of section header entry */
	hword	e_shnum;			/* Number of section header entries */
	hword	e_shstrndx;			/* Section name string table index */
} Elf64_header;

typedef struct {
	word	sh_name;        /* section name */
	word	sh_type;        /* section type */
	dword	sh_flags;       /* section flags */
	dword	sh_addr;        /* virtual address */
	dword	sh_offset;      /* file offset */
	dword	sh_size;        /* section size */
	word	sh_link;        /* link to another */
	word	sh_info;        /* misc info */
	dword	sh_addralign;   /* memory alignment */
	dword	sh_entsize;     /* table entry size */
} Elf64_sh ; 

// symbol data
typedef struct {
	word	st_name;	/* index for symbol name table */
	byte	st_info;	/* misc info of symbol */
	byte	st_other;	/* reserved */
	hword	st_shndx;	/* section index */
	dword	st_value;
	dword	st_size;
} Elf64_Sym;

// relocation info
typedef struct {
	dword	r_offset;
	dword	r_info;
} Elf64_Rel;

// relocation info with addend
typedef struct {
	dword	r_offset;
	dword	r_info;
	dword	r_addend;
} Elf64_Rela;

typedef struct {
	Elf64_header header;	/* ELF header */
	Elf64_sh *sec_h;		/* section header */
	Elf64_Rela *rels;		/* relocation data */
	int sec_name_i;			/* section name table index */
	int rel_n;				/* num of reloc info */
	Elf64_Sym *syms;		/* symbol table */
	int sym_n;				/* num of symbols */
	FILE *fp;
} Exefile;

struct Processor {
	// general registers
	dword gpr[32];
	// floating point registers
	double fpr[32];
	// condition register
	dword cr;
	// link register
	dword lr;
	// counter register
	dword ctr;
	// integer exception register
	dword xer;
	// next instruction pointer
	dword nip;
};

struct Storage {
	// the instance of memory storage
	void *mem;
	// size of storage
	long size;
	// size of text segment
	long text_size;
	// offset address of memory
	// ex) offset_addr = 0x80000000
	// mem[0x1a] stands for the address of 0x8000001a in virtual memory space
	long offset_addr;
};

extern struct Processor cpu;
extern struct Storage page;

/* 
 * I-FORM Instruction
 * |0    |6                   |30|31|
 * |OPCD |LI                  |AA|LK|
 *
 */
typedef struct {
	int opcd;
	int li;
	int aa;
	int lk;
} i_form_t;

/* 
 * B-FORM Instruction
 * |0    |6   |11  |16        |30|31|
 * |OPCD |BO  |BI  |BD        |AA|LK|
 *
 */
typedef struct {
	int opcd;
	int bo;
	int bi;
	int bd;
	int aa;
	int lk;
} b_form_t;

/* 
 * SC-FORM Instruction
 * |0    |6   |11  |16|20    |  |30|31|
 * |OPCD |    |    |  |LEV   |  |1 |  |
 *
 */
typedef struct {
	int opcd;
	int lev;
	int one;
} sc_form_t;

/* 
 * D-FORM Instruction
 * |0    |6   |11  |16              |31
 * |OPCD |RT  |RA  |D               |
 *
 */
typedef struct {
	int opcd;
	int rt;
	int ra;
	int d;
} d_form_t;

/* 
 * DS-FORM Instruction
 * |0    |6   |11  |16           |30|31
 * |OPCD |RT  |RA  |DS           |XO|
 * 
 */
typedef struct {
	int opcd;
	int rt;
	int ra;
	int ds;
	int xo;
} ds_form_t;

/* 
 * X-FORM Instruction
 * |0    |6   |11  |16  |21      |31|
 * |OPCD |RT  |RA  |RB  |XO      |EH|
 * 
 */
typedef struct {
	int opcd;
	int rt;
	int ra;
	int rb;
	int xo;
	int eh;
} x_form_t;

/* 
 * XL-FORM Instruction
 * |0    |6   |11  |16  |21      |31|
 * |OPCD |BT  |BA  |BB  |XO      |LK|
 * 
 */
typedef struct {
	int opcd;
	int bt;
	int ba;
	int bb;
	int xo;
	int lk;
} xl_form_t;

union inst_t {
	i_form_t	i;
	b_form_t	b;
	sc_form_t	sc;
	d_form_t	d;
	ds_form_t	ds;
	x_form_t	x;
	xl_form_t	xl;
};

#endif // __MAIN_H__
