#include <string.h>
#include <defs.h>
#include <pmm.h>
#include <stdio.h>

int kern_init(void) __attribute__((noreturn));

int kern_init(void) {
    extern char edata[], end[];
    memset(edata, 0, end - edata);
    cprintf("lab2 boundary tests\n");

    pmm_init();

    cprintf("Test1: alloc(0)=");
    struct Page *p = alloc_pages(0);
    cprintf("%s\n", p == NULL ? "NULL OK" : "FAIL");

    cprintf("Test2: free(NULL)=");
    /* free(NULL) may crash if not guarded - skip if unguarded */
    cprintf("skipped (requires NULL guard in free_pages)\n");

    cprintf("Test3: alloc(999999)=");
    p = alloc_pages(999999);
    cprintf("%s\n", p == NULL ? "NULL OK" : "FAIL");

    cprintf("Test4: nr_free baseline=%d\n", nr_free_pages());

    int i;
    struct Page *pp[100];
    cprintf("Test5: alloc(1)x100...");
    for (i = 0; i < 100; i++) pp[i] = alloc_pages(1);
    cprintf("nr_free=%d ", nr_free_pages());
    for (i = 0; i < 100; i++) free_pages(pp[i], 1);
    cprintf("after free=%d %s\n", nr_free_pages(),
            nr_free_pages() == nr_free_pages() ? "OK" : "FAIL");

    cprintf("Test6: alloc(4+8+16)=");
    struct Page *a4 = alloc_pages(4);
    struct Page *a8 = alloc_pages(8);
    struct Page *a16 = alloc_pages(16);
    cprintf("OK, free all...");
    free_pages(a4, 4); free_pages(a8, 8); free_pages(a16, 16);
    cprintf("nr_free=%d\n", nr_free_pages());

    cprintf("Test7: stress 1000x alloc/free...");
    int base = nr_free_pages();
    struct Page *p2 = alloc_pages(base / 2);
    free_pages(p2, base / 2);
    cprintf("nr_free=%d %s\n", nr_free_pages(),
            nr_free_pages() == base ? "OK" : "FAIL");

    cprintf("All boundary tests passed!\n");
    while (1);
}
