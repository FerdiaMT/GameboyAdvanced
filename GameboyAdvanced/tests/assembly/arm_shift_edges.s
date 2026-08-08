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
    ldr r0, =0x80000001

    @ Immediate zero encodes LSR #32 and ASR #32 in ARM state.
    movs r1, r0, lsr #32
    bcs 1f
    b fail
1:
    ASSERT_EQ r1, 0

    movs r2, r0, asr #32
    bcs 2f
    b fail
2:
    ASSERT_EQ r2, 0xffffffff

    @ ROR #0 is RRX and consumes the CPSR carry bit.
    mov r3, #0
    cmp r3, #0
    rrxs r4, r0
    bcs 3f
    b fail
3:
    ASSERT_EQ r4, 0xc0000000

    @ Register shifts distinguish exactly 32 from values greater than 32.
    mov r5, #32
    movs r6, r0, lsl r5
    bcs 4f
    b fail
4:
    ASSERT_EQ r6, 0

    mov r5, #33
    movs r6, r0, lsr r5
    bcc 5f
    b fail
5:
    ASSERT_EQ r6, 0
    swi #0

fail:
    swi #1
