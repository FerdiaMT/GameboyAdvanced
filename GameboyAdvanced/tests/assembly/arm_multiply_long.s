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
    ldr r0, =0xffffffff
    mov r1, #2

    mul r8, r0, r1
    mov r10, #3
    mla r9, r0, r1, r10
    ASSERT_EQ r8, 0xfffffffe
    ASSERT_EQ r9, 1

    umull r2, r3, r0, r1
    ASSERT_EQ r2, 0xfffffffe
    ASSERT_EQ r3, 1

    umlal r2, r3, r0, r1
    ASSERT_EQ r2, 0xfffffffc
    ASSERT_EQ r3, 3

    smull r4, r5, r0, r1
    ASSERT_EQ r4, 0xfffffffe
    ASSERT_EQ r5, 0xffffffff

    mov r6, #4
    mov r7, #0
    smlal r6, r7, r0, r1
    ASSERT_EQ r6, 2
    ASSERT_EQ r7, 0
    swi #0

fail:
    swi #1
