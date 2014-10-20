#ifndef __MAIN_H__
#define __MAIN_H__

typedef unsigned long dword;
typedef unsigned int word;
typedef unsigned short hword;
typedef unsigned char byte;

#define PROMPT "> "

#define STACK_SIZE 0xffff

#define EXEC_UNDEFINED	-1
#define EXEC_SUCCESS	0
#define EXEC_EXIT		(1 << 8)


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
	// offset address of memory
	// ex) offset_addr = 0x80000000
	// mem[0x1a] stands for the address of 0x8000001a in virtual memory space
	long offset_addr;
};

extern struct Processor cpu;
extern struct Storage code;

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
