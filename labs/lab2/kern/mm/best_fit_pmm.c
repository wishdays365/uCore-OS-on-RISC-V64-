/* Best-Fit 物理内存分配器 — lab2 核心实现
 *
 * 与 First-Fit 的区别:
 *   First-Fit: 找到第一个满足大小的空闲块即分配
 *   Best-Fit:  遍历所有空闲块，选择"最接近需求大小"的块分配
 *              即 property >= n 且 property 最小的块
 *
 * 优点: 减少内存碎片
 * 缺点: 需要遍历整个链表
 */

#include <stdio.h>
#include <pmm.h>
#include <list.h>
#include <string.h>
#include <best_fit_pmm.h>

free_area_t free_area;
#define free_list (free_area.free_list)
#define nr_free  (free_area.nr_free)

/* ---- 初始化 ---- */
static void best_fit_init(void) {
    list_init(&free_list);
    nr_free = 0;
}

/* ---- 初始化空闲页: 将 [base, base+n) 加入空闲链表 ---- */
static void best_fit_init_memmap(struct Page *base, size_t n) {
    struct Page *p;
    for (p = base; p != base + n; p++) {
        p->flags = p->property = 0;
        set_page_ref(p, 0);      /* 宏: (p)->ref = val */
    }
    base->property = n;
    SetPageProperty(base);
    nr_free += n;

    /* 按地址从小到大插入链表 */
    list_entry_t *le = list_next(&free_list);
    while (le != &free_list) {
        p = le2page(le, page_link);
        if (base + base->property <= p) break;
        le = list_next(le);
    }
    list_add_before(le, &(base->page_link));
}

/* ---- Best-Fit 分配 ---- */
static struct Page *best_fit_alloc_pages(size_t n) {
    if (n > nr_free) return NULL;

    struct Page *page = NULL;
    size_t min_size = nr_free + 1;  /* 记录最小满足条件的 property */
    list_entry_t *le = &free_list;

    /* 遍历整个空闲链表, 找 property >= n 且最小的块 */
    while ((le = list_next(le)) != &free_list) {
        struct Page *p = le2page(le, page_link);
        if (p->property >= n && p->property < min_size) {
            page = p;
            min_size = p->property;
        }
    }

    if (page != NULL) {
        list_del(&(page->page_link));
        /* 如果剩余空间 >= 1 页, 分裂 */
        if (page->property > n) {
            struct Page *p = page + n;
            p->property = page->property - n;
            SetPageProperty(p);
            /* 按地址插入剩余块 */
            list_entry_t *le2 = list_next(&free_list);
            while (le2 != &free_list) {
                struct Page *pp = le2page(le2, page_link);
                if (p + p->property <= pp) break;
                le2 = list_next(le2);
            }
            list_add_before(le2, &(p->page_link));
        }
        nr_free -= n;
        ClearPageProperty(page);
    }
    return page;
}

/* ---- 释放页 (合并相邻空闲块) ---- */
static void best_fit_free_pages(struct Page *base, size_t n) {
    struct Page *p;
    for (p = base; p != base + n; p++) {
        p->flags = 0;
        set_page_ref(p, 0);
    }
    base->property = n;
    SetPageProperty(base);

    /* 查找插入位置并合并相邻块 */
    list_entry_t *le = list_next(&free_list);
    while (le != &free_list) {
        p = le2page(le, page_link);
        le = list_next(le);
        /* 前合并: base 紧接在 p 之后 → p + p->property == base */
        if (p + p->property == base) {
            p->property += base->property;
            ClearPageProperty(base);
            base = p;
            list_del(&(p->page_link));
        }
        /* 后合并: p 紧接在 base 之后 → base + base->property == p */
        else if (base + base->property == p) {
            base->property += p->property;
            ClearPageProperty(p);
            list_del(&(p->page_link));
        }
    }

    nr_free += n;
    /* 按地址插入 */
    le = list_next(&free_list);
    while (le != &free_list) {
        p = le2page(le, page_link);
        if (base + base->property <= p) break;
        le = list_next(le);
    }
    list_add_before(le, &(base->page_link));
}

/* ---- 查询空闲页数 ---- */
static size_t best_fit_nr_free_pages(void) { return nr_free; }

/* ---- 自检 ---- */
static void best_fit_check(void) {
    int total = nr_free;
    int n1 = total / 4;
    int n2 = total / 2;
    int n3 = n1;

    struct Page *p0, *p1, *p2;

    /* 分配 3 块 */
    p0 = best_fit_alloc_pages(n1);
    p1 = best_fit_alloc_pages(n2);
    p2 = best_fit_alloc_pages(n3);

    cprintf("best_fit check: alloc %d+%d+%d pages, nr_free=%d\n",
            n1, n2, n3, nr_free);

    /* 释放并验证合并 */
    best_fit_free_pages(p0, n1);
    best_fit_free_pages(p2, n3);
    best_fit_free_pages(p1, n2);

    cprintf("best_fit check: all freed, nr_free=%d (should=%d)\n",
            nr_free, total);
}

const struct pmm_manager best_fit_pmm_manager = {
    .name = "best_fit_pmm_manager",
    .init = best_fit_init,
    .init_memmap = best_fit_init_memmap,
    .alloc_pages = best_fit_alloc_pages,
    .free_pages = best_fit_free_pages,
    .nr_free_pages = best_fit_nr_free_pages,
    .check = best_fit_check,
};
