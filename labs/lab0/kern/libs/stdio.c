#include <defs.h>
#include <stdio.h>

#define CONSBUF     0xffffffff80700000UL
#define CONSBUFSZ   0x100000

static int cons_cursor = 0;

static void cputch(int c, int *cnt) {
    if (c == '\n') {
        *(volatile char *)(CONSBUF + cons_cursor) = '\r';
        cons_cursor++;
    }
    *(volatile char *)(CONSBUF + cons_cursor) = (char)c;
    cons_cursor++;
    if (cons_cursor >= CONSBUFSZ)
        cons_cursor = 0;
    (*cnt)++;
}

int vcprintf(const char *fmt, va_list ap) {
    int cnt = 0;
    vprintfmt((void *)cputch, &cnt, fmt, ap);
    return cnt;
}

int cprintf(const char *fmt, ...) {
    va_list ap;
    int cnt;
    va_start(ap, fmt);
    cnt = vcprintf(fmt, ap);
    va_end(ap);
    return cnt;
}

void cputchar(int c) {
    int cnt;
    cputch(c, &cnt);
}

int cputs(const char *str) {
    int cnt = 0;
    char c;
    while ((c = *str++) != '\0') {
        cputch(c, &cnt);
    }
    cputch('\n', &cnt);
    return cnt;
}

int getchar(void) {
    return 0;
}
