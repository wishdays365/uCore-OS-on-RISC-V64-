#define __ASSEMBLY__
#include <sw.h>

#define REGBYTES   8
#define FRAMESIZE  (36 * REGBYTES)

#define OFF_PS     (30 * REGBYTES)
#define OFF_PC     (31 * REGBYTES)
#define OFF_GP     (32 * REGBYTES)
#define OFF_R16    (33 * REGBYTES)
#define OFF_R17    (34 * REGBYTES)
#define OFF_R18    (35 * REGBYTES)
#define OFF_CAUSE  (29 * REGBYTES)

.macro SAVE_ALL
    ldi $sp, -FRAMESIZE($sp)

    stl $0,  0*REGBYTES($sp)
    stl $1,  1*REGBYTES($sp)
    stl $2,  2*REGBYTES($sp)
    stl $3,  3*REGBYTES($sp)
    stl $4,  4*REGBYTES($sp)
    stl $5,  5*REGBYTES($sp)
    stl $6,  6*REGBYTES($sp)
    stl $7,  7*REGBYTES($sp)
    stl $8,  8*REGBYTES($sp)
    stl $9,  9*REGBYTES($sp)
    stl $10, 10*REGBYTES($sp)
    stl $11, 11*REGBYTES($sp)
    stl $12, 12*REGBYTES($sp)
    stl $13, 13*REGBYTES($sp)
    stl $14, 14*REGBYTES($sp)
    stl $15, 15*REGBYTES($sp)
    stl $19, 19*REGBYTES($sp)
    stl $20, 20*REGBYTES($sp)
    stl $21, 21*REGBYTES($sp)
    stl $22, 22*REGBYTES($sp)
    stl $23, 23*REGBYTES($sp)
    stl $24, 24*REGBYTES($sp)
    stl $25, 25*REGBYTES($sp)
    stl $26, 26*REGBYTES($sp)
    stl $27, 27*REGBYTES($sp)
    stl $28, 28*REGBYTES($sp)

    /* HMCODE 在 old_sp-0x30 处保存 OSF 帧, 相对 sp: FRAMESIZE-0x30+offset */
    ldl $1, FRAMESIZE-0x30+0x00($sp)   /* PS  */
    ldl $2, FRAMESIZE-0x30+0x08($sp)   /* PC  */
    ldl $3, FRAMESIZE-0x30+0x10($sp)   /* GP  */
    ldl $4, FRAMESIZE-0x30+0x18($sp)   /* A0=r16 */
    ldl $5, FRAMESIZE-0x30+0x20($sp)   /* A1=r17 */
    ldl $6, FRAMESIZE-0x30+0x28($sp)   /* A2=r18 */

    stl $1, OFF_PS($sp)
    stl $2, OFF_PC($sp)
    stl $3, OFF_GP($sp)
    stl $4, OFF_R16($sp)
    stl $5, OFF_R17($sp)
    stl $6, OFF_R18($sp)
.endm

.macro RESTORE_ALL
    ldl $1, OFF_R16($sp)
    ldl $2, OFF_R17($sp)
    ldl $3, OFF_R18($sp)
    ldl $4, OFF_GP($sp)
    ldl $5, OFF_PC($sp)
    ldl $6, OFF_PS($sp)
    /* 写回 HMCODE 保存区 (old_sp-0x30, 相对 sp: FRAMESIZE-0x30+offset) */
    stl $1, FRAMESIZE-0x30+0x18($sp)   /* A0=r16 */
    stl $2, FRAMESIZE-0x30+0x20($sp)   /* A1=r17 */
    stl $3, FRAMESIZE-0x30+0x28($sp)   /* A2=r18 */
    stl $4, FRAMESIZE-0x30+0x10($sp)   /* GP  */
    stl $5, FRAMESIZE-0x30+0x08($sp)   /* PC  */
    stl $6, FRAMESIZE-0x30+0x00($sp)   /* PS  */

    ldl $0,  0*REGBYTES($sp)
    ldl $1,  1*REGBYTES($sp)
    ldl $2,  2*REGBYTES($sp)
    ldl $3,  3*REGBYTES($sp)
    ldl $4,  4*REGBYTES($sp)
    ldl $5,  5*REGBYTES($sp)
    ldl $6,  6*REGBYTES($sp)
    ldl $7,  7*REGBYTES($sp)
    ldl $8,  8*REGBYTES($sp)
    ldl $9,  9*REGBYTES($sp)
    ldl $10, 10*REGBYTES($sp)
    ldl $11, 11*REGBYTES($sp)
    ldl $12, 12*REGBYTES($sp)
    ldl $13, 13*REGBYTES($sp)
    ldl $14, 14*REGBYTES($sp)
    ldl $15, 15*REGBYTES($sp)
    ldl $19, 19*REGBYTES($sp)
    ldl $20, 20*REGBYTES($sp)
    ldl $21, 21*REGBYTES($sp)
    ldl $22, 22*REGBYTES($sp)
    ldl $23, 23*REGBYTES($sp)
    ldl $24, 24*REGBYTES($sp)
    ldl $25, 25*REGBYTES($sp)
    ldl $26, 26*REGBYTES($sp)
    ldl $27, 27*REGBYTES($sp)
    ldl $28, 28*REGBYTES($sp)

    ldi $sp, FRAMESIZE($sp)
    sys_call 0x3F
.endm

    .text
    .globl entInt
    .align 4
entInt:
    SAVE_ALL
    ldi $16, 0($31)
    stl $16, OFF_CAUSE($sp)
    ldi $26, __trapret
    mov $sp, $16
    call $31, trap

    .globl entIF
    .align 4
entIF:
    SAVE_ALL
    ldi $16, 3($31)
    stl $16, OFF_CAUSE($sp)
    ldi $26, __trapret
    mov $sp, $16
    call $31, trap

    .globl __trapret
    .align 4
__trapret:
    RESTORE_ALL
