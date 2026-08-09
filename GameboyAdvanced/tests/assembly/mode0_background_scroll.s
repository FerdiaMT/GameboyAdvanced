// Hardware-style background scroll demonstration.
// Sets up a 256x256 Mode 0 tiled BG0, then advances BG0HOFS and BG0VOFS.

	.syntax unified
	.arm
	.text
	.global _start

_start:
	// Mode 0 with BG0 visible; BG0 uses character base 0 and screen base 31.
	ldr r0, =0x04000000
	ldr r1, =0x00000100
	strh r1, [r0]
	ldr r0, =0x04000008
	ldr r1, =0x00001f00
	strh r1, [r0]

	// Palette entries 1, 2 and 3: red, green and blue.
	ldr r0, =0x05000002
	ldr r1, =0x0000001f
	strh r1, [r0], #2
	ldr r1, =0x000003e0
	strh r1, [r0], #2
	ldr r1, =0x00007c00
	strh r1, [r0]

	// Three solid 8x8 4bpp tiles (palette indexes 1, 2 and 3).
	ldr r0, =0x06000020
	ldr r1, =0x11111111
	mov r2, #8
tile1:
	str r1, [r0], #4
	subs r2, r2, #1
	bne tile1
	ldr r1, =0x22222222
	mov r2, #8
tile2:
	str r1, [r0], #4
	subs r2, r2, #1
	bne tile2
	ldr r1, =0x33333333
	mov r2, #8
tile3:
	str r1, [r0], #4
	subs r2, r2, #1
	bne tile3

	// 32x32 tile map, with the RGB sequence phase-shifted on each row.
	ldr r0, =0x0600f800
	mov r6, #32
	mov r4, #0
map_row:
	mov r5, #32
	mov r3, r4
map_tile:
	add r1, r3, #1
	strh r1, [r0], #2
	add r3, r3, #1
	cmp r3, #3
	moveq r3, #0
	subs r5, r5, #1
	bne map_tile
	add r4, r4, #1
	cmp r4, #3
	moveq r4, #0
	subs r6, r6, #1
	bne map_row

	ldr r0, =0x04000010       // BG0HOFS
	ldr r1, =0x04000012       // BG0VOFS
	mov r7, #0
	mov r12, #0
scroll:
	strh r7, [r0]
	strh r12, [r1]
	ldr r2, =70000             // update about twice per rendered frame
delay:
	subs r2, r2, #1
	bne delay
	add r7, r7, #4             // four pixels horizontally per update
	cmp r7, #256
	moveq r7, #0
	add r12, r12, #2           // two pixels vertically per update
	cmp r12, #256
	moveq r12, #0
	b scroll
