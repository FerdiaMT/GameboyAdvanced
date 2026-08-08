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
    sub r0, sp, #64
    mov r1, #11
    mov r2, #22
    mov r3, #33
    mov r4, #44

    stmia r0!, {r1-r4}
    sub r5, r0, #16
    mov r1, #0
    mov r2, #0
    mov r3, #0
    mov r4, #0
    ldmia r5!, {r1-r4}
    ASSERT_EQ r1, 11
    ASSERT_EQ r2, 22
    ASSERT_EQ r3, 33
    ASSERT_EQ r4, 44

    @ Both writeback values must identify the word immediately after the list.
    sub r6, sp, #48
    cmp r0, r6
    bne fail
    cmp r5, r6
    bne fail
    swi #0

fail:
    swi #1
