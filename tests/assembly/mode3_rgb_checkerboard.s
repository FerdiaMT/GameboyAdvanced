// GBA Mode 3 demonstration ROM: 8x8 tiles cycling red, green, blue.
// The phase advances on every group of eight scanlines, producing a proper
// checkerboard rather than relying on incidental RGB555 bit patterns.

	.syntax unified
	.arm
	.text
	.global _start

_start:
	ldr r0, =0x04000000       // DISPCNT
	mov r1, #3                // Mode 3, BG2 framebuffer
	strh r1, [r0]

	ldr r0, =0x06000000       // VRAM base
	ldr r8, =0x0000001f       // red
	ldr r9, =0x000003e0       // green
	ldr r10, =0x00007c00      // blue
	mov r6, #160              // rows remaining
	mov r4, #0                // first row's colour phase

row:
	mov r5, #30               // 240 pixels / 8 pixels per tile
	mov r3, r4
tile:
	cmp r3, #0
	beq red
	cmp r3, #1
	beq green
	mov r1, r10
	b paint
red:
	mov r1, r8
	b paint
green:
	mov r1, r9
paint:
	mov r2, #8
pixel:
	strh r1, [r0], #2
	subs r2, r2, #1
	bne pixel
	add r3, r3, #1
	cmp r3, #3
	moveq r3, #0
	subs r5, r5, #1
	bne tile

	subs r6, r6, #1
	beq hang
	and r2, r6, #7            // advance phase every 8 rows
	cmp r2, #0
	addeq r4, r4, #1
	cmpeq r4, #3
	moveq r4, #0
	b row
hang:
	b hang
