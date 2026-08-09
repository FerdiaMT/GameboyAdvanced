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

    @ Post-indexing and pre-indexing with writeback use different addresses.
    str r1, [r0], #4
    ldr r2, [r0, #-4]
    ASSERT_EQ r2, 0x11223344

    str r1, [r0, #4]!
    ldr r2, [r0]
    ASSERT_EQ r2, 0x11223344

    @ Byte transfers and scaled register offsets share the single-data path.
    mov r3, #0x5a
    strb r3, [r0, #1]
    ldrb r4, [r0, #1]
    ASSERT_EQ r4, 0x5a

    mov r5, #8
    str r1, [r0, r5, lsl #1]
    ldr r6, [r0, r5, lsl #1]
    ASSERT_EQ r6, 0x11223344
    swi #0

fail:
    swi #1
