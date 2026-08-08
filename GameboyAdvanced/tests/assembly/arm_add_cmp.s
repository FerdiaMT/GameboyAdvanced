.syntax unified
.cpu arm7tdmi
.arm

.global _start
.type _start, %function

_start:
    mov r0, #0x12
    mov r1, #0x30
    add r2, r0, r1
    cmp r2, #0x42
    bne fail

pass:
    swi #0                  @ test pass: recognized by GBA --test-swi

fail:
    swi #1                  @ test fail: recognized by GBA --test-swi
