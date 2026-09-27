#include <stdio.h>

int kern_init(void) __attribute__((noreturn));
int kern_init(void) {
    int i;
    for (i = 0; i < 50; i++)
        cprintf("hello %d\n", i);
    cprintf("(THU.CST) os is loading ...\n");
    while (1);
}
