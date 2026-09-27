#ifndef __LIBS_SW_H__
#define __LIBS_SW_H__

/* ====== 汇编文件安全隔离 ====== */
#ifndef __ASSEMBLY__

/* ---------------------------------------------------------------
 * wrent - 注册中断/异常入口地址到 vcpucb
 *   r16 = 入口函数地址, r17 = 入口编号 (0=中断, 3=异常)
 * --------------------------------------------------------------- */
static inline void wrent(void *entry, unsigned long num) {
    register void *__r16 __asm__("$16") = entry;
    register unsigned long __r17 __asm__("$17") = num;
    __asm__ __volatile__(
        "sys_call 0x34"
        : : "r"(__r16), "r"(__r17)
        : "memory", "$0", "$1", "$22", "$23", "$24", "$25"
    );
}

/* ---------------------------------------------------------------
 * wrkgp - 注册内核全局指针
 * --------------------------------------------------------------- */
static inline void wrkgp(unsigned long kgp) {
    register unsigned long __r16 __asm__("$16") = kgp;
    __asm__ __volatile__(
        "sys_call 0x37"
        : : "r"(__r16)
        : "memory", "$0", "$1", "$22", "$23", "$24", "$25"
    );
}

/* ---------------------------------------------------------------
 * wrtimer - 设置时钟中断 (oneshot 模式)
 *   写入非零值启动，触发一次后需重新调用
 * --------------------------------------------------------------- */
static inline void wrtimer(unsigned long delta) {
    register unsigned long __r16 __asm__("$16") = delta;
    __asm__ __volatile__(
        "sys_call 0x3B"
        : : "r"(__r16)
        : "memory", "$0", "$1", "$22", "$23", "$24", "$25"
    );
}

/* ---------------------------------------------------------------
 * swpipl - 设置中断优先级屏蔽，返回旧值
 *   0 = 开放所有, 7 = 屏蔽所有
 * --------------------------------------------------------------- */
static inline unsigned long swpipl(unsigned long mask) {
    register unsigned long __r16 __asm__("$16") = mask;
    register unsigned long __r0 __asm__("$0");
    __asm__ __volatile__(
        "sys_call 0x35"
        : "=r"(__r0)
        : "r"(__r16)
        : "memory", "$1", "$22", "$23", "$24", "$25"
    );
    return __r0;
}

#endif /* !__ASSEMBLY__ */

/* ---------------------------------------------------------------
 * do_div - 除法/取余宏 (SW64 无硬件除法器)
 * --------------------------------------------------------------- */
#define do_div(n, base) ({                                          \
    unsigned long __rem;                                            \
    __rem = ((unsigned long long)(n)) % (unsigned long long)(base); \
    (n) = ((unsigned long long)(n)) / (unsigned long long)(base);   \
    __rem;                                                          \
})

#endif /* __LIBS_SW_H__ */
