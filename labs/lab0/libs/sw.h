#ifndef __LIBS_SW_H__
#define __LIBS_SW_H__

#define do_div(n, base) ({                                          \
    unsigned long __rem;                                            \
    __rem = ((unsigned long long)(n)) % (unsigned long long)(base); \
    (n) = ((unsigned long long)(n)) / (unsigned long long)(base);   \
    __rem;                                                          \
})

#endif /* !__LIBS_SW_H__ */
