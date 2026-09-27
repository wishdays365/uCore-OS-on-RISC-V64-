#ifndef __KERN_TRAP_TRAP_H__
#define __KERN_TRAP_TRAP_H__

#include <defs.h>

struct pushregs {
    uintptr_t r0;
    uintptr_t r1;
    uintptr_t r2;
    uintptr_t r3;
    uintptr_t r4;
    uintptr_t r5;
    uintptr_t r6;
    uintptr_t r7;
    uintptr_t r8;
    uintptr_t r9;
    uintptr_t r10;
    uintptr_t r11;
    uintptr_t r12;
    uintptr_t r13;
    uintptr_t r14;
    uintptr_t r15;
    uintptr_t a0;
    uintptr_t a1;
    uintptr_t a2;
    uintptr_t r19;
    uintptr_t r20;
    uintptr_t r21;
    uintptr_t r22;
    uintptr_t r23;
    uintptr_t r24;
    uintptr_t r25;
    uintptr_t r26;
    uintptr_t r27;
    uintptr_t r28;
    uintptr_t cause;
    uintptr_t ps;
    uintptr_t pc;
    uintptr_t gp;
    uintptr_t r16;
    uintptr_t r17;
    uintptr_t r18;
} __attribute__((aligned(16)));

struct trapframe {
    struct pushregs gpr;
};

void idt_init(void);
void trap(struct trapframe *tf);
void print_trapframe(struct trapframe *tf);
void do_entInt(struct trapframe *tf);
void do_entIF(struct trapframe *tf);

#endif
