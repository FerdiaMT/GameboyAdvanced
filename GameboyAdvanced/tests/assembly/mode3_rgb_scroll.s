// Continuously scrolling Mode 3 RGB checkerboard.
// Each repaint advances the horizontal tile phase.  In a live SDL window this
// moves the 8x8 red/green/blue tile pattern across the screen indefinitely.

	.syntax unified
	.arm
	.text
	.global _start

_start:
	ldr r0, =0x04000000
	mov r1, #3                // Mode 3
	strh r1, [r0]
	ldr r0, =0x06000000
	ldr r8, =0x0000001f       // red
	ldr r9, =0x000003e0       // green
	ldr r10, =0x00007c00      // blue
	mov r11, #0               // horizontal tile scroll phase

frame:
	ldr r0, =0x06000000
	mov r6, #160
	mov r4, #0
row:
	mov r5, #30
	add r3, r4, r11
	cmp r3, #3
	subge r3, r3, #3
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
	beq next_frame
	and r2, r6, #7
	cmp r2, #0
	addeq r4, r4, #1
	cmpeq r4, #3
	moveq r4, #0
	b row
next_frame:
	add r11, r11, #1
	cmp r11, #3
	moveq r11, #0
	b frame
