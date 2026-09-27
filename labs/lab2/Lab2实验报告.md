# Lab2 物理内存管理实验报告

**实验名称**：Lab2 — SW64 物理内存管理移植  
**实验日期**：2026 年 7 月  
**实验环境**：Ubuntu 20.04（192.168.33.129）、SW64 交叉编译器 swgcc710、QEMU v6.2.0-sw-2206  
**实验者**：钞方贺

---

## 一、实验目的

给内核加上物理内存管理。主要三件事：搞懂 SW64 的地址空间是怎么编址的（和 RISC-V 不一样）、实现 PADDR/KADDR 地址转换、写一个 best_fit 分配器替换原来的 first_fit。还要移植 list.h（通用链表）和 atomic.h（位操作）。

---

## 二、实验内容

在 lab1 的中断处理基础上加物理内存管理。涉及的文件：

| 文件 | 干嘛的 |
|------|--------|
| `kern/mm/pmm.h` | PADDR/KADDR 宏、Page 结构体、pmm_manager 接口 |
| `kern/mm/pmm.c` | page_init（探测物理内存、初始化 Page 数组） |
| `kern/mm/default_pmm.c` | first_fit 参考实现 |
| `kern/mm/best_fit_pmm.c` | best_fit 实现（核心） |
| `kern/mm/memlayout.h` | SW64 四段地址常量（覆盖 lab1 版本） |
| `libs/list.h` | 通用双向链表 |
| `libs/atomic.h` | 位操作 |

---

## 三、实验步骤

### 3.1 SW64 内核地址编址

这个和 RISC-V 完全不同。SW64 的内核地址由 HMCODE 固件约定好了四段（写在 `host_dtbm_single_k.S` 的注释里）：

| 地址范围 | 容量 | 用途 |
|---------|------|------|
| `fff0000000000000` ~ `fff07fffffffffff` | 128TB | 物理内存直接映射 |
| `ffffffff80000000` ~ `ffffffff9fffffff` | 512MB | 内核代码映射 |

我们主要用这两段。内核自己（`.text`/`.data`/`.bss`）在内核代码映射区（`0xffffffff8xxxxxxx`），但管理物理页的 pages 数组最好切换到直接映射区（`0xfff0000xxxxxxxxx`），方便后续按物理地址操作。

### 3.2 PADDR 和 KADDR

在这两段地址之间切换需要 PADDR（虚拟→物理）和 KADDR（物理→虚拟）。

```c
#define PAGE_OFFSET          0xfff0000000000000UL
#define __START_KERNEL_map   0xffffffff80000000UL

#define PADDR(kva)  (kva >= __START_KERNEL_map ? \
                     kva - __START_KERNEL_map : kva - PAGE_OFFSET)
#define KADDR(pa)   ((void *)((uintptr_t)(pa) | PAGE_OFFSET))
```

物理地址加上 `PAGE_OFFSET` 就到了直接映射区。内核代码映射区的地址减去 `__START_KERNEL_map` 能回物理地址。

### 3.3 page_init

物理内存从 `0x910000`（内核加载地址）开始，大小硬编码 512MB。pages 数组紧跟在 `end[]` 后面：

```c
pages = (struct Page *)ROUNDUP((void *)end, PGSIZE);
pages = (struct Page *)KADDR(PADDR(pages));
```

第二行把 pages 从内核代码映射区切到直接映射区。然后标记内核占用的页为 reserved（不能分配），剩下的页交给 pmm_manager 管。

### 3.4 first_fit vs best_fit

first_fit 的做法：维护一个按地址从小到大排列的空闲块链表，分配时遍历，找到第一个 `property >= n` 的块就用。简单、快，但容易碎片化——比如链表里有个 2 页的块和一个 1 页的块，先申请 1 页（从 2 页块里拿，剩 1 页），再申请 2 页就失败了（两个块各自只有 1 页）。

best_fit 的做法：和 first_fit 一样维护按地址排序的链表，但分配时不拿第一个满足的，而是遍历整个链表，记录 `property >= n` 且 `property` 最小的那个块。同样上面的例子，best_fit 会从 1 页块里拿走，留 2 页块给后面的请求。

分配的代码核心长这样：

```c
static struct Page *best_fit_alloc_pages(size_t n) {
    struct Page *page = NULL;
    size_t min_size = nr_free + 1;
    list_entry_t *le = &free_list;
    while ((le = list_next(le)) != &free_list) {
        struct Page *p = le2page(le, page_link);
        if (p->property >= n && p->property < min_size) {
            page = p;
            min_size = p->property;  // 记下当前最小的
        }
    }
    // 找到后分裂、移除
    ...
}
```

释放的时候和 first_fit 一样要做前后合并——检查释放的块和链表里前后块是否相邻，相邻就合成一个大块。

### 3.5 atomic.h 和 list.h

`atomic.h` 参考 Linux `arch/sw_64/include/asm/bitops.h`，实现了 `set_bit`、`clear_bit`、`test_bit`。Page 的 `flags` 字段用这些函数标记是 reserved 还是 free。

`list.h` 就是双向链表的基本操作，内核里用得太多了。`le2page` 这个宏——从链表节点指针算出包含它的 Page 结构体指针——是 `container_of` 的标准模式。

---

## 四、实验结果

```
pmm_manager: best_fit_pmm_manager
pages @ kernel addr = 0xffffffff8091c000    ← 初始在内核代码映射区
pages @ direct map = 0xfff000000091c000     ← KADDR(PADDR()) 切过来了
physical pages: 64376, mem: 0x910000 ~ 0x20000000
free pages: 64370, starting at page[6]
best_fit check: alloc 16092+32185+16092 pages, nr_free=1
best_fit check: all freed, nr_free=64370 (should=64370)
```

关键两条：`pages @ direct map = 0xfff000000091c000` 说明 PADDR/KADDR 切换正确；`nr_free=64370 (should=64370)` 说明分配和释放完全正确，页面没有泄漏或丢失。

额外跑的边界测试：
- `alloc(0)` → 不崩溃
- `alloc(999999)` → 返回 NULL
- 连续 `alloc(1)` 100 次再全 free，nr_free 完全恢复
- `alloc(4)` + `alloc(8)` + `alloc(16)` → 全成功

---

## 五、遇到的问题

### PADDR 判断逻辑

SW64 有两套虚拟地址格式，PADDR 里需要用 `__START_KERNEL_map` 做阈值区分。一开始只处理了内核代码映射区的情况，结果 pages 切到直接映射区后，再算 PADDR 就出错了——因为直接映射区的地址也满足 `>= __START_KERNEL_map` 的条件。改了判断顺序：内核代码映射区减 `__START_KERNEL_map`，其他的减 `PAGE_OFFSET`。

### best_fit 链表插入顺序

分配后如果剩余空间 > 0，要把剩余块插回链表。第一次写的时候直接 `list_add` 插到链表头，结果链表不按地址排序了，free 的时候合并逻辑就炸。改成遍历找到"第一个地址 >= 剩余块地址"的位置，用 `list_add_before` 插入。

### Makefile 误编译备份文件

切换测试用的 `init_clock.c`、`init_exception.c` 放在了 `kern/init/` 目录下，Makefile 的 `listf_cc` 会编译该目录下所有 `.c` 文件，报了 `multiple definition of kern_init`。解决方案是把备份放到 lab 根目录，编译前 `cp` 过去。

---

## 六、小结

lab2 整体比 lab1 顺利。核心难点在理解 SW64 的双地址体系——内核代码映射区和直接映射区什么时候用哪个、怎么切换。PADDR/KADDR 写对了后面就顺了。

best_fit 和 first_fit 的区别在于选择策略：first_fit 追求速度（找到即停），best_fit 追求减少碎片（找最合适的）。best_fit 的实现多了一步遍历，但链表的合并/分裂/排序逻辑和 first_fit 是一样的。

做边界测试的时候确认了一个事：`alloc_pages(0)` 不会返回 NULL——这是 uCore 的设计选择，不是 bug。超量分配能正确返回 NULL。

*报告完成日期：2026 年 7 月*
