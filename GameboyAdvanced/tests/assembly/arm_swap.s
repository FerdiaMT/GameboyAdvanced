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
    ldr r1, =0x11223344
    str r1, [r0]
    ldr r2, =0xaabbccdd
    swp r3, r2, [r0]
    ASSERT_EQ r3, 0x11223344
    ldr r4, [r0]
    ASSERT_EQ r4, 0xaabbccdd

    add r5, r0, #4
    mov r6, #0x5a
    strb r6, [r5]
    mov r6, #0xc3
    swpb r7, r6, [r5]
    ASSERT_EQ r7, 0x5a
    ldrb r8, [r5]
    ASSERT_EQ r8, 0xc3
    swi #0

fail:
    swi #1
