/* SW64 物理内存管理器 — 主模块 */

#include <pmm.h>
#include <defs.h>
#include <stdio.h>
#include <string.h>

/* ---- 全局变量 ---- */
const struct pmm_manager *pmm_manager;
struct Page *pages;          /* Page 结构体数组首地址 */
size_t npage;                /* 物理页总数 */
uintptr_t va_pa_offset;      /* 内核代码映射偏移 */

/* ---- 当前使用 best_fit ---- */
extern const struct pmm_manager best_fit_pmm_manager;

/* ================================================================
 * page_init — 初始化物理内存管理
 *
 * 内存布局 (由低到高):
 *   [kernel .text/.data/.bss]  [空闲物理页]  [pages数组]
 *   ^                          ^              ^
 *   &end                       free_start     pages
 *
 * 步骤:
 *   1. 从 &end 后取 pages 数组起始位置
 *   2. 用 KADDR(PADDR(pages)) 切换到直接映射地址
 *   3. 物理内存从 PHYSICAL_MEMORY_BASE (0x910000) 开始
 *   4. 内核占用的页标记为 reserved
 *   5. 剩余页交给 pmm_manager 管理
 * ================================================================ */
static void page_init(void) {
    extern char end[];

    /* pages 紧跟在 end[] 之后, 页对齐 */
    pages = (struct Page *)ROUNDUP((void *)end, PGSIZE);
    cprintf("pages @ kernel addr = %p\n", pages);

    /* 切换到直接映射地址 (0xfff0000...开头) */
    pages = (struct Page *)KADDR(PADDR(pages));
    cprintf("pages @ direct map = %p\n", pages);

    /* 物理内存从 0x910000 开始 (lab0 kernel.ld 的加载地址) */
    uintptr_t mem_begin = 0x910000;
    uintptr_t mem_end = PHYSICAL_MEMORY_END;  /* 512MB */

    npage = (mem_end - mem_begin) / PGSIZE;
    cprintf("physical pages: %d, mem: 0x%lx ~ 0x%lx\n", npage, mem_begin, mem_end);

    /* 计算内核占用的页数 (从 0x910000 到 pages 物理地址) */
    uintptr_t kernel_end_pa = PADDR(pages);
    size_t kernel_pages = (kernel_end_pa - mem_begin) / PGSIZE;

    /* 标记内核占用页为 reserved */
    struct Page *p;
    for (p = pages; p < pages + kernel_pages; p++) {
        SetPageReserved(p);
    }

    /* 空闲页起始 = pages 数组之后 */
    p = pages + kernel_pages;
    size_t free_pages = npage - kernel_pages;

    /* 交给 pmm_manager 初始化空闲页 */
    cprintf("free pages: %d, starting at page[%d]\n", free_pages, kernel_pages);
    pmm_manager->init_memmap(p, free_pages);
}

/* ================================================================
 * alloc_pages — 分配连续 n 个物理页
 * ================================================================ */
struct Page *alloc_pages(size_t n) {
    struct Page *page;
    if (n > nr_free_pages()) return NULL;
    page = pmm_manager->alloc_pages(n);
    return page;
}

/* ================================================================
 * free_pages — 释放连续 n 个物理页
 * ================================================================ */
void free_pages(struct Page *base, size_t n) {
    pmm_manager->free_pages(base, n);
}

/* ================================================================
 * nr_free_pages — 查询当前空闲页数
 * ================================================================ */
size_t nr_free_pages(void) {
    return pmm_manager->nr_free_pages();
}

/* ================================================================
 * pmm_init — 物理内存管理初始化 (由 kern_init 调用)
 * ================================================================ */
void pmm_init(void) {
    /* 计算内核代码映射偏移 */
    va_pa_offset = __START_KERNEL_map - 0;

    /* 使用 best_fit 分配器 */
    pmm_manager = &best_fit_pmm_manager;
    cprintf("pmm_manager: %s\n", pmm_manager->name);

    /* 初始化 pmm_manager */
    pmm_manager->init();

    /* 初始化物理页管理 */
    page_init();

    /* 自检 */
    pmm_manager->check();
}
