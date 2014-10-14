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
 * DS-FORM Instruction
 * |0    |6   |11  |16           |30|
 * |OPCD |RT  |RA  |DS           |XO|
 *
 * ld
 */
typedef struct {
	byte opcd;
	byte rt;
	byte ra;
	word ds;
	byte xo;
} ds_form;

#endif // __MAIN_H__
