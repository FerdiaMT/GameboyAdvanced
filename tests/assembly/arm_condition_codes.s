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
    @ Equal operands: EQ, CS, PL, VC, GE, LE, and LS are true.
    mov r0, #5
    cmp r0, #5
    moveq r1, #1
    movne r1, #2
    movcs r2, #1
    movcc r2, #2
    movpl r3, #1
    movmi r3, #2
    movvc r4, #1
    movvs r4, #2
    movge r5, #1
    movlt r5, #2
    movle r6, #1
    movgt r6, #2
    movls r7, #1
    movhi r7, #2
    ASSERT_EQ r1, 1
    ASSERT_EQ r2, 1
    ASSERT_EQ r3, 1
    ASSERT_EQ r4, 1
    ASSERT_EQ r5, 1
    ASSERT_EQ r6, 1
    ASSERT_EQ r7, 1

    @ A signed-negative, unsigned-lower comparison flips complementary paths.
    mov r0, #1
    cmp r0, #2
    movlt r8, #1
    movle r9, #1
    movcc r10, #1
    movls r11, #1
    movmi r1, #1
    movvc r2, #1
    ASSERT_EQ r8, 1
    ASSERT_EQ r9, 1
    ASSERT_EQ r10, 1
    ASSERT_EQ r11, 1
    ASSERT_EQ r1, 1
    ASSERT_EQ r2, 1
    swi #0

fail:
    swi #1
