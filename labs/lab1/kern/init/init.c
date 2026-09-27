#include <string.h>
#include <clock.h>
#include <defs.h>
#include <intr.h>
#include <stdio.h>
#include <trap.h>
#include <sw.h>

int kern_init(void) __attribute__((noreturn));

int kern_init(void) {
    extern char edata[], end[];
    memset(edata, 0, end - edata);
    cprintf("(THU.CST) os is loading ...\n\n");

    idt_init();
    clock_init();
    intr_enable();

    /* 在 callee-saved 寄存器中存入特征值 */
    register long sv9  __asm__("$9")  = 0xDEAD0001;
    register long sv10 __asm__("$10") = 0xDEAD0002;
    register long sv14 __asm__("$14") = 0xDEAD0003;

    /* 等几个时钟中断触发 SAVE_ALL/RESTORE_ALL */
    volatile int w;
    for (w = 0; w < 100000000; w++) { asm volatile("" ::: "memory"); }

    cprintf("saved regs: s0=%lx s1=%lx s5=%lx\n", sv9, sv10, sv14);

    while (1);
}
