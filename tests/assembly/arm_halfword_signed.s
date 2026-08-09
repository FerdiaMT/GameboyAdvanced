.syntax unified
.cpu arm7tdmi
.arm

.global _start
.type _start, %function

.macro ASSERT_EQ register, value
    ldr r12, =\value
    cmp \register, r12
    bne fail
.endm

_start:
    mov r0, sp
    ldr r1, =0x80ff7f01
    str r1, [r0]

    ldrsb r2, [r0]
    ASSERT_EQ r2, 1
    ldrsb r2, [r0, #1]
    ASSERT_EQ r2, 0x7f
    ldrsb r2, [r0, #2]
    ASSERT_EQ r2, 0xffffffff
    ldrsb r2, [r0, #3]
    ASSERT_EQ r2, 0xffffff80

    ldrh r3, [r0]
    ASSERT_EQ r3, 0x7f01
    ldrsh r3, [r0, #2]
    ASSERT_EQ r3, 0xffff80ff

    ldr r4, =0xabcd
    strh r4, [r0, #4]
    ldrh r5, [r0, #4]
    ASSERT_EQ r5, 0xabcd
    swi #0

fail:
    swi #1
