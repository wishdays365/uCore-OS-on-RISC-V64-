/* SW64 物理内存管理器 — 头文件 */

#ifndef __KERN_MM_PMM_H__
#define __KERN_MM_PMM_H__

#include <defs.h>
#include <list.h>
#include <atomic.h>
#include <memlayout.h>
#include <mmu.h>

/* ================================================================
 * PADDR / KADDR — 内核地址 ↔ 物理地址 转换
 *
 * SW64 两套内核地址：
 *   - kernel text 映射: 0xffffffff8xxxxxxx → pa = va - 0xffffffff80000000
 *   - 直接映射:         0xfff0000xxxxxxxxx → pa = va - 0xfff0000000000000
 *
 * __phys_addr() 正确处理两种情况
 * ================================================================ */

/* va_pa_offset: 内核代码映射区偏移, 在 pmm_init 中计算 */
extern uintptr_t va_pa_offset;

static inline uintptr_t __phys_addr(uintptr_t x) {
    if (x >= __START_KERNEL_map)
        return x - __START_KERNEL_map;
    else
        return x - PAGE_OFFSET;
}

#define __va(x)    ((void *)((uintptr_t)(x) | PAGE_OFFSET))
#define __pa(x)    __phys_addr((uintptr_t)(x))

#define PADDR(kva) ({                                          \
    uintptr_t __m_kva = (uintptr_t)(kva);                      \
    __m_kva >= __START_KERNEL_map ?                            \
        __m_kva - __START_KERNEL_map :                         \
        __m_kva - PAGE_OFFSET;                                 \
})

#define KADDR(pa) ({                                           \
    uintptr_t __m_pa = (uintptr_t)(pa);                        \
    (void *)(__m_pa | PAGE_OFFSET);                            \
})

/* ================================================================
 * Page 结构体 — 每个物理页对应一个
 * ================================================================ */
struct Page {
    int ref;                 /* 页表引用计数 */
    uint32_t flags;          /* PG_reserved / PG_property */
    unsigned int property;   /* 空闲块中连续页数 (仅头页有效) */
    list_entry_t page_link;  /* 空闲链表节点 */
};

#define PG_reserved  0       /* 保留页 (内核占用等), 不可分配 */
#define PG_property  1       /* 空闲块头页标记 */

#define SetPageReserved(page)   set_bit(PG_reserved, &((page)->flags))
#define ClearPageReserved(page) clear_bit(PG_reserved, &((page)->flags))
#define IsPageReserved(page)    test_bit(PG_reserved, &((page)->flags))

#define SetPageProperty(page)   set_bit(PG_property, &((page)->flags))
#define ClearPageProperty(page) clear_bit(PG_property, &((page)->flags))
#define IsPageProperty(page)    test_bit(PG_property, &((page)->flags))
#define set_page_ref(page, val) ((page)->ref = (val))

/* ================================================================
 * free_area_t — 空闲页管理
 * ================================================================ */
typedef struct {
    list_entry_t free_list;   /* 空闲块链表 (按地址排序) */
    unsigned int nr_free;     /* 空闲页总数 */
} free_area_t;

/* ================================================================
 * pmm_manager — 物理内存分配器接口 (面向对象风格)
 * ================================================================ */
struct pmm_manager {
    const char *name;
    void (*init)(void);
    void (*init_memmap)(struct Page *base, size_t n);
    struct Page *(*alloc_pages)(size_t n);
    void (*free_pages)(struct Page *base, size_t n);
    size_t (*nr_free_pages)(void);
    void (*check)(void);
};

extern const struct pmm_manager *pmm_manager;
extern struct Page *pages;       /* Page 结构体数组首地址 */
extern size_t npage;             /* 物理页总数 */

/* ================================================================
 * 全局函数
 * ================================================================ */
void pmm_init(void);

struct Page *alloc_pages(size_t n);
void free_pages(struct Page *base, size_t n);
size_t nr_free_pages(void);

/* 页号 ↔ 地址 转换 */
static inline ppn_t page2ppn(struct Page *page) { return page - pages; }
static inline uintptr_t page2pa(struct Page *page) { return page2ppn(page) << PGSHIFT; }
static inline struct Page *pa2page(uintptr_t pa) { return &pages[pa >> PGSHIFT]; }
static inline void *page2kva(struct Page *page) { return KADDR(page2pa(page)); }
static inline struct Page *kva2page(void *kva) { return pa2page(__pa(kva)); }

/* 链表 ↔ Page 转换 */
#define le2page(le, member)  to_struct((le), struct Page, member)

#endif
