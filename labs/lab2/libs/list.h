/* 通用双向链表 */

#ifndef __LIBS_LIST_H__
#define __LIBS_LIST_H__

struct list_entry {
    struct list_entry *prev, *next;
};

typedef struct list_entry list_entry_t;

static inline void list_init(list_entry_t *elm) { elm->prev = elm->next = elm; }

static inline void __list_add(list_entry_t *elm, list_entry_t *prev, list_entry_t *next) {
    prev->next = next->prev = elm;
    elm->next = next;
    elm->prev = prev;
}

static inline void list_add(list_entry_t *listelm, list_entry_t *elm) {
    __list_add(elm, listelm, listelm->next);
}

static inline void list_add_before(list_entry_t *listelm, list_entry_t *elm) {
    __list_add(elm, listelm->prev, listelm);
}

static inline void list_del(list_entry_t *listelm) {
    listelm->prev->next = listelm->next;
    listelm->next->prev = listelm->prev;
}

static inline int list_empty(list_entry_t *list) { return list->next == list; }

#define list_next(elm)  ((elm)->next)
#define list_prev(elm)  ((elm)->prev)

#define offsetof(type, member)  ((size_t)(&((type *)0)->member))
#define to_struct(ptr, type, member)  ((type *)((char *)(ptr) - offsetof(type, member)))

#endif
