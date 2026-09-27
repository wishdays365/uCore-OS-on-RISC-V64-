# Lab1.0 中断机制实验报告

**实验名称**：Lab1.0 — SW64 中断与异常处理移植  
**实验日期**：2026 年 7 月  
**实验环境**：Ubuntu 20.04（192.168.33.129）、SW64 交叉编译器 swgcc710、QEMU v6.2.0-sw-2206  
**实验者**：钞方贺

---

## 一、实验目的

在 lab0.5 的基础上让内核能响应中断和异常。具体要搞懂：SW64 怎么通过 HMCODE 处理中断、怎么注册中断入口（`wrent`/`wrkgp`）、保存和恢复寄存器（SAVE_ALL/RESTORE_ALL）、中断返回（rti）、时钟中断怎么触发（`wrtimer`）、异常（断点和非法指令）怎么捕获和处理。

---

## 二、实验内容

给 lab0.5 加上中断处理能力。涉及的新文件：

| 文件 | 干嘛的 |
|------|--------|
| `kern/trap/trapentry.S` | entInt/entIF 两个汇编入口 + SAVE_ALL/RESTORE_ALL |
| `kern/trap/trap.c` | idt_init 注册、trap_dispatch 分发、do_entInt/do_entIF 处理 |
| `kern/trap/trap.h` | pushregs（36 字段寄存器结构体） |
| `kern/driver/clock.c` | 时钟初始化（oneshot wrtimer） |
| `kern/driver/intr.c` | 开关中断（swpipl） |
| `libs/sw.h` | 新增 HMCALL 封装（wrent/wrkgp/wrtimer/swpipl） |

修改的文件：`kern/init/init.c` 加了 `idt_init → clock_init → intr_enable → sys_call 0x80 → .long 0x7a000000`。

---

## 三、实验步骤

### 3.1 封装 HMCALL

从 Linux `hmcall.h` 查准编号：`wrent=0x34`、`swpipl=0x35`、`wrkgp=0x37`、`wrtimer=0x3B`。用内联汇编包成 C 函数。sw.h 用 `#ifndef __ASSEMBLY__` 包住 C 代码——这是 lab1.0 说明文档里专门提醒过的坑，不然 trapentry.S include 进来就报错。

### 3.2 寄存器结构体

pushregs 36 个字段：r0~r28（29 个通用寄存器）、cause（自己填，entInt=0/entIF=3）、ps/pc/gp（HMCODE 在栈上保存的）、r16~r18（HMCODE 传入的参数，a0=中断类型）。

### 3.3 汇编入口

entInt 和 entIF 流程一样：SAVE_ALL 保存寄存器 → 设置 cause 值 → `call $31, trap` 调 C → `__trapret` 里 RESTORE_ALL → `sys_call 0x3F`（rti 返回）。

SAVE_ALL 里一个坑：HMCODE 在 `old_sp - 0x30` 位置保存了 PS/PC/GP/r16~r18，所以读的时候是 `FRAMESIZE - 0x30 + offset`，不是 `FRAMESIZE + offset`。另外 r16~r18 直接从寄存器取更可靠——HMCODE 跳过来时就在寄存器里，直接存进 pushregs。

### 3.4 中断分发

`idt_init` 做三件事：`wrkgp`（告诉 HMCODE 内核的 GP）、`wrent(entInt, 0)`（注册中断入口）、`wrent(entIF, 3)`（注册异常入口）。

trap_dispatch 用 switch-case：cause=0 → do_entInt，cause=3 → do_entIF。

do_entInt 里 a0=9 是时钟中断（OSF_A0_INT__CLK），调 `clock_set_next_event()` 重新设置定时器（oneshot 模式），计数 + 每 100 次打印。

do_entIF 里 a0=0 是断点、a0=4 是非法指令，分别打印。这里踩了好一阵子坑。

### 3.5 断点死循环的调试

`sys_call 0x80` 触发断点 → HMCODE 跳 entIF → 处理完 → rti 返回 → 又回到同一条指令 → 无限循环。

原因是 HMCODE 用 `$26`（ra 寄存器）保存返回地址，rti 返回的是 `$26` 指向的指令。如果不推进 `$26`，它就永远回到那条断点指令。试了好几种方案：改 SAVE_ALL 里的 pc 写回、改 RESTORE_ALL 里 hmcode 保存区的 pc……都没用。

最后发现在 `do_entIF` 里直接 `tf->gpr.r26 += 4` 就解决了。因为 RESTORE_ALL 从 pushregs 恢复 `$26`，pushregs 里的 r26 被加了 4，恢复出来的 `$26` 就指向下一条指令，rti 自然就跳过去了。

### 3.6 时钟和中断控制

`clock_init` 里 `wrtimer(100000)` 启动定时器。SW64 的 timer 是 oneshot，触发一次就停了，所以每次中断处理里要重新 `wrtimer`。

`intr_enable` 是 `swpipl(0)`——0 表示开放所有中断优先级，7 是全部屏蔽。

### 3.7 中断处理流程（画成图）

```
[注册]  idt_init() → wrkgp + wrent(entInt,0) + wrent(entIF,3)
           ↓
[触发]  HMCODE检测时钟 → a0=9 → 保存PS/PC/GP/r16~r18 → 跳entInt
           ↓
[汇编]  SAVE_ALL(sp-=288,存r0~r28) → cause=0 → call trap
           ↓
[C代码]  trap_dispatch → do_entInt(a0=9) → wrtimer + ticks++ + cprintf
           ↓
[返回]  __trapret → RESTORE_ALL(恢复r0~r28) → sys_call 0x3F(rti) → HMCODE恢复执行 → 循环
```

异常路径类似，只是入口是 entIF、cause=3、do_entIF 里根据 a0 分类（0 断点/4 非法指令），返回前 r26+=4 跳过异常指令。

---

## 四、实验结果

分两块验证。因为断点后面如果跟非法指令会导致 HMCODE 路径的 rti 行为复杂，分开跑更清晰。

**时钟中断测试**：`100 ticks → 200 → 300 → ... → 7000 ticks`，说明完整链路（注册 → 触发 → 处理 → 返回 → 再触发）持续跑了 7000 轮没问题。

**异常处理测试**：`breakpoint pc = ffffffff809100a8`（a0=0 正确识别）、`opDEC pc = ffffffff809100b0`（a0=4 正确识别），且中间没有死循环。

额外跑了 SAVE_ALL 验证：在 `$9`/`$10`/`$14` 里存 `0xDEAD0001`/`0002`/`0003`，经过多次时钟中断后值不变——说明 SAVE_ALL/RESTORE_ALL 没有漏寄存器。

---

## 五、遇到的问题

### sw.h 在 trapentry.S 编译报错

`libs/sw.h` 里的 C 内联汇编被汇编器当指令解析。按文档说的加 `#ifndef __ASSEMBLY__` 包住 C 部分，trapentry.S 开头加 `#define __ASSEMBLY__`。

### `jmp $31, ($0), trap` 调用失败

一开始用这种间接跳转方式，但 `($0)` 是解引用地址 0，不是取 `$0` 寄存器的值。改成 `call $31, trap` 伪指令。查了 SW64 调用约定确认 `$26`=ra、`$16`=a0、`$31`=zero 用法正确。

### HMCODE 保存区读出来全是垃圾

a0/r16 读出来是 `-2137948224`，ps 和 gp 是 0。对了好久 Linux 的 `entry_c3.S` 才反应过来：HMCODE 先 `ldi r30, -0x30(r30)` 把栈往下扩了 48 字节再保存，所以保存区在 `old_sp - 0x30`。但 SAVE_ALL 从 `old_sp + offset` 去读了。改偏移方向解决，同时 r16~r18 直接从寄存器取，两条路兜底。

### 断点死循环

上面 3.5 详细写了。核心是 HMCODE 用的 `$26` 存返回地址，不进就卡死。`tf->gpr.r26 += 4` 解决。

---

## 六、小结

lab1.0 是三个实验里最难啃的——中断处理链路长（HMCODE → 汇编 → C → 汇编 → HMCODE），任何一环出错都崩。调试过程被迫仔细读了 HMCODE 的保存机制、SW64 的调用约定、`entry_c3.S` 的 SAVE_ALL 实现。

几个关键认知：SW64 中断不走 `stvec` 而是 HMCODE 分发；HMCODE 保存的 OSF 帧在 `old_sp - 0x30`；断点/异常返回时 HMCODE 用 `$26`(ra) 而不是 OSF 帧里的 PC 字段。这些光看文档做不到，得实际调过才知道。

*报告完成日期：2026 年 7 月*
