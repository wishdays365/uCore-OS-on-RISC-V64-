#include <sw.h>
#include <stdio.h>

void clock_set_next_event(void) {
    wrtimer(100000);
}

void clock_init(void) {
    clock_set_next_event();
    cprintf("++ setup timer interrupts\n");
}
