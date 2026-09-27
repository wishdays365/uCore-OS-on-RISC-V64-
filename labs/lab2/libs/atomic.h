/* 原子操作 — SW64 移植自 Linux arch/sw_64/include/asm/bitops.h */

#ifndef __LIBS_ATOMIC_H__
#define __LIBS_ATOMIC_H__

/* ---- 位操作 ---- */
static inline void set_bit(int nr, volatile void *addr) {
    int *m = ((int *)addr) + (nr >> 5);
    int mask = 1 << (nr & 31);
    *m |= mask;
}

static inline void clear_bit(int nr, volatile void *addr) {
    int *m = ((int *)addr) + (nr >> 5);
    int mask = 1 << (nr & 31);
    *m &= ~mask;
}

static inline int test_bit(int nr, volatile void *addr) {
    int *m = ((int *)addr) + (nr >> 5);
    int mask = 1 << (nr & 31);
    return (*m & mask) != 0;
}

/* ---- 原子类型 ---- */
typedef struct { volatile int counter; } atomic_t;

static inline int atomic_read(atomic_t *v)  { return v->counter; }
static inline void atomic_set(atomic_t *v, int i) { v->counter = i; }
static inline void atomic_add(atomic_t *v, int i) { v->counter += i; }
static inline void atomic_sub(atomic_t *v, int i) { v->counter -= i; }
static inline int atomic_sub_return(atomic_t *v, int i) { return v->counter -= i; }

#endif
