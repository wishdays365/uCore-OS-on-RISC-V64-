/* First-Fit 物理内存分配器 — 参考实现 */

#include <stdio.h>
#include <pmm.h>
#include <list.h>
#include <string.h>
#include <default_pmm.h>

free_area_t free_area;
#define free_list (free_area.free_list)
#define nr_free  (free_area.nr_free)

/* ---- 初始化 ---- */
static void default_init(void) {
    list_init(&free_list);
    nr_free = 0;
}

/* ---- 初始化空闲页: 将 [base, base+n) 加入空闲链表 ---- */
static void default_init_memmap(struct Page *base, size_t n) {
    struct Page *p;
    for (p = base; p != base + n; p++) {
        p->flags = p->property = 0;
        set_page_ref(p, 0);
    }
    base->property = n;
    SetPageProperty(base);
    nr_free += n;
    list_add(&free_list, &(base->page_link));
}

/* ---- First-Fit 分配 ---- */
static struct Page *default_alloc_pages(size_t n) {
    if (n > nr_free) return NULL;

    struct Page *page = NULL;
    list_entry_t *le = &free_list;

    while ((le = list_next(le)) != &free_list) {
        struct Page *p = le2page(le, page_link);
        if (p->property >= n) {
            page = p;
            break;
        }
    }

    if (page != NULL) {
        list_del(&(page->page_link));
        if (page->property > n) {
            struct Page *p = page + n;
            p->property = page->property - n;
            SetPageProperty(p);
            list_add(&free_list, &(p->page_link));
        }
        nr_free -= n;
        ClearPageProperty(page);
    }
    return page;
}

/* ---- 释放页 (合并相邻空闲块) ---- */
static void default_free_pages(struct Page *base, size_t n) {
    struct Page *p;
    for (p = base; p != base + n; p++) {
        p->flags = 0;
        set_page_ref(p, 0);
    }
    base->property = n;
    SetPageProperty(base);

    list_entry_t *le = list_next(&free_list);
    while (le != &free_list) {
        p = le2page(le, page_link);
        if (base + base->property == p) {
            base->property += p->property;
            ClearPageProperty(p);
            list_del(&(p->page_link));
        } else if (p + p->property == base) {
            p->property += base->property;
            ClearPageProperty(base);
            base = p;
            list_del(&(p->page_link));
        }
        le = list_next(le);
    }

    nr_free += n;
    le = list_next(&free_list);
    while (le != &free_list) {
        p = le2page(le, page_link);
        if (base + base->property <= p) break;
        le = list_next(le);
    }
    list_add_before(le, &(base->page_link));
}

/* ---- 查询空闲页数 ---- */
static size_t default_nr_free_pages(void) { return nr_free; }

/* ---- 自检 ---- */
static void default_check(void) {
    /* 简单分配-释放测试 */
    int count = nr_free / 3;
    struct Page *p0 = default_alloc_pages(count);
    struct Page *p1 = default_alloc_pages(count);
    struct Page *p2 = default_alloc_pages(count);
    cprintf("first_fit check: %d+%d+%d pages allocated\n", count, count, count);

    default_free_pages(p0, count);
    default_free_pages(p1, count);
    default_free_pages(p2, count);
    cprintf("first_fit check: all freed, nr_free=%d\n", nr_free);
}

const struct pmm_manager default_pmm_manager = {
    .name = "default_pmm_manager (first_fit)",
    .init = default_init,
    .init_memmap = default_init_memmap,
    .alloc_pages = default_alloc_pages,
    .free_pages = default_free_pages,
    .nr_free_pages = default_nr_free_pages,
    .check = default_check,
};
