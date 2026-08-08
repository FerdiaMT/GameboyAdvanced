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
    @ Only the flag byte is modified; the supervisor mode bits stay intact.
    mov r0, #0
    msr cpsr_f, r0
    movne r1, #1
    ASSERT_EQ r1, 1

    ldr r0, =0xa0000000
    msr cpsr_f, r0
    bmi 1f
    b fail
1:
    bcs 2f
    b fail
2:
    mrs r2, cpsr
    ldr r3, =0xf0000000
    and r2, r2, r3
    ASSERT_EQ r2, 0xa0000000
    swi #0

fail:
    swi #1
