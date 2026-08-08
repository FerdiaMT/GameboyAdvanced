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
    mov r0, #0
    @ BL is emitted directly to keep this flat binary relocation-free.
    @ From 0x08000004, PC is 0x0800000c and the helper is at 0x08000034.
    .word 0xeb00000a
    ASSERT_EQ r0, 3

    @ A backward conditional branch is used for the complete loop body.
    mov r1, #5
1:
    add r0, r0, r1
    subs r1, r1, #1
    bne 1b
    ASSERT_EQ r0, 18
    swi #0

2:
    add r0, r0, #3
    bx lr

fail:
    swi #1
