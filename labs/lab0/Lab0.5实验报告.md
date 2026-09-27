# Lab0.5 最小内核实验报告

**实验名称**：Lab0.5 — 基于申威（SW64）架构的最小内核移植  
**实验日期**：2026 年 7 月  
**实验环境**：Ubuntu 20.04 虚拟机（192.168.33.129）、SW64 交叉编译器 swgcc710、QEMU v6.2.0-sw-2206  
**实验者**：钞方贺

---

## 一、实验目的

把 uCore 从 RISC-V64 搬到 SW64 上，跑通一个"打印一句话就死循环"的最小内核。要做的事：配交叉编译、写 SW64 汇编入口、搞定内核打印（不用 OpenSBI）、让内核在 QEMU 上跑起来。

具体学：swgcc 怎么用、git 本地管理、Makefile 规则、SW64 链接脚本怎么写、内核地址映射、GP 指针和 `ldgp`、栈的作用、entry.S 移植、内存打印区、`pmemsave` 命令。

---

## 二、实验内容

RISC-V 版 uCore 搬 SW64，改的东西：

- 删 RISC-V 相关的：`sbi.c`、`sbi.h`、`riscv.h`、`kern/driver/`
- 页大小 4KB → 8KB
- Makefile 全换 SW64 工具链
- entry.S 用 SW64 汇编写
- 打印不走 OpenSBI，直接写内存缓冲区

改完的文件结构：

```
lab0/
├── Makefile              ├── kern/init/entry.S     ├── libs/printfmt.c
├── tools/kernel.ld       ├── kern/init/init.c      ├── libs/string.c
├── kern/mm/mmu.h         ├── kern/libs/stdio.c     ├── libs/sw.h
├── kern/mm/memlayout.h   ├── libs/__div*.o (4个)
```

---

## 三、实验步骤

### 3.1 Makefile

编译器前缀换 `sw_64sw6b-sunway-linux-gnu-`，链接 `-m elf64sw_64`。删 `-mcmodel=medany`（SW64 不认识）。交叉编译器缺 `libmpfr.so.4`，看 lab0 参考 Makefile 加了 `export LD_LIBRARY_PATH` 解决。SW64 无硬件除法，`printfmt` 里做除法和取余会调 `__divlu` 等函数，从 Linux 内核 `arch/sw_64/lib/divide.S` 拿四个预编译 `.o` 放到 `libs/`，加进 `KOBJS`。

### 3.2 entry.S

RISC-V 原版 `la sp, bootstacktop; tail kern_init`，SW64 完全不是这套。对着 Linux 的 `head.S` 写：

```asm
kern_entry:
    br  $27, 1f           # PC→$27，算GP用
1:  ldgp $29, 0($27)      # 初始化GP
    ldi $30, bootstacktop  # 设栈
    jmp $31, kern_init     # 跳C
```

三条缺一不可：不设 GP → 全局变量全挂，不设 SP → C 函数直接崩。

### 3.3 打印输出

RISC-V 版 `cputchar` 调 OpenSBI。SW64 的做法是写到一个约定的内存地址，然后用 QEMU 的 `pmemsave` 读。地址：虚拟 `0xffffffff80700000`、物理 `0x700000`。把 `cputchar` 改成向 `CONSBUF` 写字符，加了 `volatile` 防止编译器优化掉。

### 3.4 链接脚本 + 编译运行

`kernel.ld` 换成 SW64 格式，`BASE_ADDRESS = 0xffffffff80910000`。编译跑通后 `pmemsave 0x700000 1000 p.log` 查看输出。

---

## 四、实验结果

```
(THU.CST) os is loading ...
```

连续打印 50 次 `hello %d` 测试，0 到 49 全部正确，没乱码没覆盖。

---

## 五、遇到的问题

### 交叉编译器动态库缺失

`libmpfr.so.4` 不在系统路径里。参考 Makefile 加 `export LD_LIBRARY_PATH` 解决。

### 管道自动化 pmemsave 全零

一开始用 `echo | timeout qemu` 方式自动测，p.log 全是 `00`。原因是 SSH 管道和 QEMU monitor 的 stdin 有竞态。之后所有验证都用手动 `make qemu` 交互操作。

---

## 六、小结

lab0.5 主要是熟悉 SW64 的工具链和基本约定。GP 寻址、HMCODE 加载、内存打印缓冲区是后面 lab1 的基础。坑不多，但每个都要搞清楚才能继续。

*报告完成日期：2026 年 7 月*
