#ifndef __MAIN_H__
#define __MAIN_H__

typedef unsigned long qword;
typedef unsigned int dword;
typedef unsigned short word;
typedef unsigned char byte;

struct Processor {
	// general registers
	qword r0, r1, r2, r3, r4, r5, r6, r7;
	qword r8, r9, r10, r11, r12, r13, r14, r15;
	qword r16, r17, r18, r19, r20, r21, r22, r23;
	qword r24, r25, r26, r27, r28, r29, r30, r31;
	// floating point registers
	/*
	double f0, f1, f2, f3, f4, f5, f6, f7;
	double f8, f9, f10, f11, f12, f13, f14, f15;
	double f16, f17, f18, f19, f20, f21, f22, f23;
	double f24, f25, f26, f27, f28, f29, f30, f31;
	*/
	// condition register
	qword cr;
	// link register
	qword lr;
	// counter register
	qword ctr;
	// integer exception register
	qword xer;
};

struct Storage {
	void *mem;
	long size;
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

union inst_t {
	i_form_t i;
	b_form_t b;
	sc_form_t sc;
	d_form_t d;
	ds_form_t ds;
	x_form_t x;
};

#endif // __MAIN_H__
