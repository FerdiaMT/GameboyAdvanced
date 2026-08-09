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
    @ Signed overflow must set N and V without setting C.
    ldr r0, =0x7fffffff
    adds r1, r0, #1
    bmi 1f
    b fail
1:
    bvs 2f
    b fail
2:
    bcc 3f
    b fail
3:
    ASSERT_EQ r1, 0x80000000

    @ Carry input to ADC/SBC is produced by CMP, not assumed.
    mov r2, #0
    cmp r2, #0
    adc r3, r2, #0
    ASSERT_EQ r3, 1

    mov r2, #2
    cmp r2, #3
    sbc r3, r2, #1
    ASSERT_EQ r3, 0

    @ Exercise the remaining logical/reverse arithmetic datapaths.
    mvn r4, #0
    and r5, r4, #0xff
    eor r5, r5, #0x55
    bic r5, r5, #0x0f
    orr r5, r5, #0x0a
    rsb r6, r5, #0x100
    rsc r7, r6, #0x200
    ASSERT_EQ r5, 0xaa
    ASSERT_EQ r6, 0x56
    ASSERT_EQ r7, 0x1aa
    swi #0

fail:
    swi #1
