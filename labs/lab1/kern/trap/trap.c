#include <defs.h>
#include <stdio.h>
#include <sw.h>
#include <trap.h>
#include <clock.h>
#include <intr.h>

extern void entInt(void);
extern void entIF(void);

static void trap_dispatch(struct trapframe *tf);

void idt_init(void) {
    register unsigned long gptr __asm__("$29");
    wrkgp(gptr);
    wrent(entInt, 0);
    wrent(entIF, 3);
}

void trap(struct trapframe *tf) {
    trap_dispatch(tf);
}

static void trap_dispatch(struct trapframe *tf) {
    switch (tf->gpr.cause) {
        case 0:
            do_entInt(tf);
            break;
        case 3:
            do_entIF(tf);
            break;
        default:
            cprintf("unexpected trap cause=%ld\n", tf->gpr.cause);
            while (1);
    }
}

static volatile size_t ticks = 0;

void do_entInt(struct trapframe *tf) {
    if (tf->gpr.a0 == 9) {
        clock_set_next_event();
        if (++ticks % 100 == 0) {
            cprintf("%d ticks\n", ticks);
        }
    } else {
        cprintf("unknown interrupt a0=%ld\n", tf->gpr.a0);
    }
}

void do_entIF(struct trapframe *tf) {
    tf->gpr.r26 += 4;
    switch (tf->gpr.a0) {
        case 0:
            cprintf("breakpoint pc = %#lx\n", tf->gpr.pc - 4);
            break;
        case 4:
            cprintf("opDEC pc = %#lx\n", tf->gpr.pc - 4);
            break;
        default:
            cprintf("unknown exception a0=%ld\n", tf->gpr.a0);
            print_trapframe(tf);
            break;
    }
}

void print_trapframe(struct trapframe *tf) {
    cprintf("trapframe at %p\n", tf);
    cprintf("  cause=%ld pc=%#lx ps=%#lx gp=%#lx\n",
            tf->gpr.cause, tf->gpr.pc, tf->gpr.ps, tf->gpr.gp);
    cprintf("  a0=%ld a1=%ld a2=%ld\n",
            tf->gpr.a0, tf->gpr.a1, tf->gpr.a2);
}
