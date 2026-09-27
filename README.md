# uCore 操作系统移植 —— RISC-V64 → 申威 SW64

将 uCore 教学操作系统从 **RISC-V64** 移植到国产 **申威 SW64** 架构，作为「申威卓越创新班」校企协同专项的暑期实习实践内容。

**实践时间**：2026 年 7 月 6 日 – 7 月 16 日
**实践单位**：无锡先进技术研究院

---

## 一、实践内容与结果

| 实验 | 内容 | 关键结果 |
|------|------|----------|
| **Lab0.5** | 最小内核移植（RISC-V → SW64） | QEMU 成功输出 `(THU.CST) os is loading ...` |
| **Lab1.0** | 中断 / 异常处理移植 | 时钟中断累计 100 → 7000 ticks；断点与非法指令异常可捕获 |
| **Lab2** | 物理内存管理移植 | Best-Fit 分配器，评分 **50/50 满分**（pmm / page table / ticks 三项全通过） |

---

## 二、技术栈与环境

| 项目 | 内容 |
|------|------|
| 目标架构 | 申威 SW64（国产自主指令集） |
| 交叉编译工具链 | `sw_64sw6b-sunway-linux-gnu-gcc` / `ld` / `objdump`（swgcc710） |
| 模拟器 | QEMU v6.2.0-sw-2206（`qemu-system-sw64`） |
| 链接格式 | `-m elf64sw_64` |
| 开发环境 | Ubuntu 20.04（虚拟机） |
| 调试手段 | QEMU monitor `pmemsave`、GDB、分步隔离法 |

---

## 三、主要移植工作

### 3.1 内核入口（SW64 汇编）

RISC-V 的 `la sp, bootstacktop; tail kern_init` 在 SW64 上完全不适用，重写为：

```asm
kern_entry:
    br  $27, 1f           # PC → $27，用于计算 GP
1:  ldgp $29, 0($27)      # 初始化全局指针 GP
    ldi $30, bootstacktop # 设置栈指针 SP
    jmp $31, kern_init    # 跳转到 C 入口
```

GP 与 SP 必须显式初始化，否则全局变量访问与 C 函数调用都会崩溃。

### 3.2 HMCODE 固件调用封装（`libs/sw.h`）

SW64 使用 HMCODE 固件而非 OpenSBI，中断不走 `stvec` 而是通过 `wrent` 注册入口。
用 GCC 内联汇编封装了四个 HMCODE 调用：

| 封装函数 | `sys_call` 编号 | 作用 |
|---------|----------------|------|
| `wrent` | `0x34` | 注册中断 / 异常入口地址 |
| `swpipl` | `0x35` | 设置中断优先级屏蔽 |
| `wrkgp` | `0x37` | 注册内核全局指针 |
| `wrtimer` | `0x3B` | 设置时钟中断（oneshot 模式） |

### 3.3 中断上下文与地址空间

- 定义 `pushregs` 结构体保存 36 个寄存器字段，实现 `SAVE_ALL` / `RESTORE_ALL`
- 处理 HMCODE 在 `old_sp - 0x30` 处保存 PS/PC/GP/r16~r18 的约定
- SW64 为**四段地址空间**（不同于 RISC-V 的单一地址空间），重写 `PADDR` / `KADDR` 宏：

```c
#define PAGE_OFFSET          0xfff0000000000000UL
#define __START_KERNEL_map   0xffffffff80000000UL

#define PADDR(kva)  (kva >= __START_KERNEL_map ? \
                     kva - __START_KERNEL_map : kva - PAGE_OFFSET)
#define KADDR(pa)   ((void *)((uintptr_t)(pa) | PAGE_OFFSET))
```

### 3.4 物理内存管理（`kern/mm/best_fit_pmm.c`）

实现 Best-Fit 页面分配器，替换原 First-Fit 参考实现：
- 空闲链表按物理地址升序维护
- 分配时遍历链表，选取满足需求且 `property` 最小的块
- 释放时向前 / 向后合并相邻空闲块
- 配套 `best_fit_check()` 自检函数

### 3.5 其他移植改动

- 页大小由 4KB 调整为 **8KB**
- 内核打印不再依赖 OpenSBI，改为写入约定内存缓冲区（虚拟 `0xffffffff80700000` / 物理 `0x700000`），通过 QEMU `pmemsave` 读取
- Makefile 全面切换至 SW64 工具链，移除 `-mcmodel=medany`

---

## 四、实验报告

| 报告 | 链接 |
|------|------|
| Lab0.5 最小内核实验报告 | [`labs/lab0/Lab0.5实验报告.md`](labs/lab0/Lab0.5实验报告.md) |
| Lab1.0 中断机制实验报告 | [`labs/lab1/Lab1.0实验报告.md`](labs/lab1/Lab1.0实验报告.md) |
| Lab2 物理内存管理实验报告 | [`labs/lab2/Lab2实验报告.md`](labs/lab2/Lab2实验报告.md) |
| 暑期实习实践反馈 | [`labs/lab2/暑期实习反馈.md`](labs/lab2/暑期实习反馈.md) |

各实验目录下的 `.score` 文件为自动评分记录（**Lab2 为 50/50 满分**）。

> Lab1 的评分记录为 10/40：评分脚本仍按 x86 架构检查 `cs` / `ds` / `ring` 等分段寄存器，
> 而 SW64 架构不存在这些概念，故仅 `ticks` 一项可通过。此为评分脚本架构不匹配所致，非实现缺陷。

---

## 五、目录结构

```
labs/
├── lab0/   # 最小内核移植（交叉编译环境、SW64 汇编入口、内存缓冲区输出）
├── lab1/   # 中断与异常（HMCODE 封装、SAVE_ALL/RESTORE_ALL、rti 返回）
└── lab2/   # 物理内存管理（PADDR/KADDR 四段地址、Best-Fit 分配器）
```

---

## 六、说明

- 基础代码来自 uCore 教学操作系统（RISC-V64 版本）
- Lab1 的起始框架参考了 [ring00/bbl-ucore](https://github.com/ring00/bbl-ucore) 的 lab1 代码，在其基础上完成 SW64 移植
- SW64 架构相关的入口汇编、HMCODE 封装、地址映射与内存分配器为本次实践移植实现
