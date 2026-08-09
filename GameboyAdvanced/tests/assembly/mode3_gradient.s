// Minimal GBA display bring-up ROM.
// It selects Mode 3 (240x160 15-bit direct colour), then fills VRAM with a
// continuously changing colour.  No BIOS calls or interrupt setup are needed.

	.syntax unified
	.arm
	.text
	.global _start

_start:
	ldr r0, =0x04000000       // DISPCNT
	mov r1, #3                // Mode 3, BG2 framebuffer
	strh r1, [r0]

	ldr r0, =0x06000000       // VRAM base
	ldr r1, =0x0000001f       // first pixel: red
	ldr r2, =38400            // 240 * 160 pixels
fill:
	strh r1, [r0], #2
	add r1, r1, #1
	subs r2, r2, #1
	bne fill
hang:
	b hang
