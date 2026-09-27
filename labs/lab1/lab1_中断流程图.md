# Lab1.0 中断处理全流程图

## 一、中断注册流程

```mermaid
sequenceDiagram
    participant kern_init
    participant idt_init
    participant HMCODE

    kern_init->>idt_init: 调用 idt_init()
    idt_init->>HMCODE: wrkgp($29) — sys_call 0x37
    Note over HMCODE: 保存内核 GP 到 vcpucb
    idt_init->>HMCODE: wrent(entInt, 0) — sys_call 0x34
    Note over HMCODE: 注册中断入口 entInt 到 vcpucb
    idt_init->>HMCODE: wrent(entIF, 3) — sys_call 0x34
    Note over HMCODE: 注册异常入口 entIF 到 vcpucb
```

## 二、中断发生→处理→返回全流程

```mermaid
flowchart TD
    subgraph HMCODE["HMCODE 固件"]
        A["硬件时钟中断触发"] --> B["读取 CSR__INT_STAT"]
        B --> C{"判断中断类型"}
        C -->|TIMER| D["设置 a0(r16)=9<br/>OSF_A0_INT__CLK"]
        D --> E["保存上下文到栈<br/>PS(r30+0x00)<br/>PC(r30+0x08)<br/>GP(r30+0x10)<br/>A0(r30+0x18)<br/>A1(r30+0x20)<br/>A2(r30+0x28)"]
        E --> F["从 vcpucb 取 entInt 地址"]
        F --> G["从 vcpucb 取 KGP"]
        G --> H["跳转 entInt"]
    end

    subgraph entInt["entInt (汇编入口)"]
        H --> I["SAVE_ALL<br/>sp -= 288<br/>保存 r0~r28"]
        I --> J["从 HMCODE 保存区<br/>读取 PS/PC/GP/r16~r18<br/>填入 pushregs"]
        J --> K["cause = 0 (中断)"]
        K --> L["$26 = __trapret (返回地址)"]
        L --> M["a0 = sp (trapframe 指针)"]
        M --> N["call $31, trap"]
    end

    subgraph trap_c["trap.c (C语言)"]
        N --> O["trap_dispatch(tf)"]
        O --> P{"tf->cause == ?"}
        P -->|0| Q["do_entInt(tf)"]
        P -->|3| R["do_entIF(tf)"]
        Q --> S{"a0 == 9 ?"}
        S -->|是| T["clock_set_next_event()<br/>wrtimer(100000)"]
        S -->|否| U["打印 unknown interrupt"]
        T --> V["ticks++, 每100次打印"]
        R --> W{"a0 == ?"}
        W -->|0| X["打印 breakpoint pc"]
        W -->|4| Y["打印 opDEC pc"]
        W -->|其他| Z["print_trapframe"]
        X --> AA["tf->r26 += 4<br/>(跳过异常指令)"]
        Y --> AA
    end

    subgraph ret["返回路径"]
        V --> AB["__trapret"]
        AA --> AB
        U --> AB
        Z --> AB
        AB --> AC["RESTORE_ALL<br/>恢复 r0~r28<br/>写回 HMCODE 保存区<br/>sp += 288"]
        AC --> AD["sys_call 0x3F (rti)"]
        AD --> AE["HMCODE 恢复 PC/PS/GP<br/>继续执行"]
    end

    AE -->|时钟路径| A
```

## 三、时钟中断详细流程

```mermaid
flowchart TD
    subgraph init["初始化阶段"]
        A["kern_init()"] --> B["idt_init()<br/>wrkgp + wrent(entInt,0)"]
        B --> C["clock_init()<br/>wrtimer(100000)"]
        C --> D["intr_enable()<br/>swpipl(0)"]
        D --> E["while(1)"]
    end

    subgraph hmcode["HMCODE 触发"]
        E --> F["时钟硬件中断"]
        F --> G["读 CSR__INT_STAT"]
        G --> H["a0=9 (TIMER)"]
        H --> I["保存上下文<br/>PS/PC/GP/r16~r18"]
        I --> J["跳转 entInt"]
    end

    subgraph asm["entInt (汇编)"]
        J --> K["SAVE_ALL<br/>sp-=288<br/>保存 r0~r28"]
        K --> L["读 HMCODE 保存区"]
        L --> M["cause=0"]
        M --> N["call trap"]
    end

    subgraph c["do_entInt (C)"]
        N --> O["trap_dispatch"]
        O --> P{"a0==9?"}
        P -->|否| Q["忽略"]
        P -->|是| R["clock_set_next_event()"]
        R --> S["ticks++"]
        S --> T{"每100次?"}
        T -->|是| U["cprintf ticks"]
        T -->|否| V["跳过"]
    end

    subgraph ret["返回"]
        U --> W["RESTORE_ALL"]
        V --> W
        Q --> W
        W --> X["sys_call 0x3F (rti)"]
        X --> Y["HMCODE 恢复执行"]
    end

    Y --> E
```

## 四、异常处理流程

```mermaid
flowchart TD
    subgraph bp["断点异常 (a0=0)"]
        A["asm('sys_call 0x80')"] --> B["HMCODE 识别断点"]
        B --> C["a0=0"]
        C --> D["跳转 entIF"]
        D --> E["SAVE_ALL<br/>cause=3"]
        E --> F["trap → do_entIF"]
        F --> G["a0==0<br/>cprintf breakpoint"]
        G --> H["tf->r26 += 4"]
        H --> I["RESTORE_ALL → rti"]
        I --> J["执行下一条指令"]
    end

    subgraph il["非法指令 (a0=4)"]
        J --> K["asm('.long 0x7a000000')"]
        K --> L["HMCODE 识别非法指令"]
        L --> M["a0=4"]
        M --> N["跳转 entIF"]
        N --> O["SAVE_ALL<br/>cause=3"]
        O --> P["trap → do_entIF"]
        P --> Q["a0==4<br/>cprintf opDEC"]
        Q --> R["tf->r26 += 4"]
        R --> S["RESTORE_ALL → rti"]
        S --> T["继续执行"]
    end
```

## 五、关键寄存器约定

| 寄存器 | 别名 | 用途 |
|--------|------|------|
| $0     | v0   | 返回值 |
| $1~$8  | t0~t7 | 临时寄存器 |
| $9~$15 | s0~s6 | 被调用者保存 |
| $16~$21| a0~a5 | 函数参数 |
| $26    | ra   | 返回地址 |
| $27    | pv   | 过程地址 |
| $29    | gp   | 全局指针 |
| $30    | sp   | 栈指针 |
| $31    | zero | 恒为 0 |

## 六、HMCALL 编号速查

| HMCALL | 编号 | 用途 |
|--------|:---:|------|
| wrent  | 0x34 | 注册入口到 vcpucb |
| swpipl | 0x35 | 中断优先级屏蔽 |
| wrkgp  | 0x37 | 注册内核 GP |
| wrtimer| 0x3B | 设置时钟 |
| rti    | 0x3F | 中断返回 |
