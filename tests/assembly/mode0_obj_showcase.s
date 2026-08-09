// Phase-1 PPU visual acceptance ROM.
// Blue tiled BG0 with regular red/green OBJ sprites and an affine green OBJ.

	.syntax unified
	.arm
	.text
	.global _start

_start:
	ldr r0, =0x04000000
	ldr r1, =0x00001100       // Mode 0, BG0 + OBJ
	strh r1, [r0]
	ldr r0, =0x04000008
	ldr r1, =0x00001f01       // BG0 screen base 31, priority 1
	strh r1, [r0]

	ldr r0, =0x05000002
	ldr r1, =0x00007c00       // BG palette[1] blue
	strh r1, [r0]
	ldr r0, =0x05000202
	ldr r1, =0x000003e0       // OBJ palette[1] green
	strh r1, [r0], #2
	ldr r1, =0x0000001f       // OBJ palette[2] red
	strh r1, [r0]

	// BG tile 1: solid palette index 1.
	ldr r0, =0x06000020
	ldr r1, =0x11111111
	mov r2, #8
bg_tile:
	str r1, [r0], #4
	subs r2, r2, #1
	bne bg_tile

	// Fill BG0's 32x32 map with tile 1.
	ldr r0, =0x0600f800
	mov r1, #1
	ldr r2, =1024
map:
	strh r1, [r0], #2
	subs r2, r2, #1
	bne map

	// OBJ tile 0 = green, tile 1 = red.
	ldr r0, =0x06010000
	ldr r1, =0x11111111
	mov r2, #8
obj_green:
	str r1, [r0], #4
	subs r2, r2, #1
	bne obj_green
	ldr r1, =0x22222222
	mov r2, #8
obj_red:
	str r1, [r0], #4
	subs r2, r2, #1
	bne obj_red

	// OAM 0: regular green at (0,0).  OAM 1: regular red at (16,0).
	ldr r0, =0x07000000
	mov r1, #0
	strh r1, [r0]
	strh r1, [r0, #2]
	strh r1, [r0, #4]
	strh r1, [r0, #8]
	mov r1, #16
	strh r1, [r0, #10]
	mov r1, #1
	strh r1, [r0, #12]

	// OAM 2: affine green at (32,0), affine matrix 0 = identity.
	mov r1, #0x100
	strh r1, [r0, #16]
	mov r1, #32
	strh r1, [r0, #18]
	mov r1, #0
	strh r1, [r0, #20]
	mov r1, #0x100
	strh r1, [r0, #6]         // PA
	strh r1, [r0, #30]        // PD
hang:
	b hang
