#ifndef __KERN_MM_MEMLAYOUT_H__
#define __KERN_MM_MEMLAYOUT_H__

/* ================================================================
 * SW64 内核地址空间编址 (来自 HMCODE host_dtbm_single_k.S)
 *
 * VA[52]=1 为合法内核地址
 * fff0000000000000 ~ fff07fffffffffff (128TB): 物理内存直接映射
 * ffffffff80000000 ~ ffffffff9fffffff (512MB): 内核代码映射
 * ================================================================ */

#define PAGE_OFFSET          0xfff0000000000000UL  /* __va(x)=x|PAGE_OFFSET */
#define __START_KERNEL_map   0xffffffff80000000UL
#define KERNEL_IMAGE_SIZE    0x20000000UL           /* 512MB */

/* 实验用: 物理内存 512MB 硬编码 */
#define PHYSICAL_MEMORY_END  0x20000000UL

/* 内核栈 */
#define KSTACKPAGE           2
#define KSTACKSIZE           (KSTACKPAGE * PGSIZE)  /* 16KB */

#endif
