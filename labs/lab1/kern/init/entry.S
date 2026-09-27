#include <mmu.h>
#include <memlayout.h>

.section .text,"ax",%progbits
.globl kern_entry
.type kern_entry,@function
kern_entry:
    br  $27, 1f
1:  ldgp $29, 0($27)
    ldi $30, bootstacktop

    jmp $31, kern_init

.section .data
.align PGSHIFT
.global bootstack
bootstack:
.space KSTACKSIZE
.global bootstacktop
bootstacktop:
