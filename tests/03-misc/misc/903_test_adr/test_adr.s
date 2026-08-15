.syntax unified
    .arch armv7-a
    .text
    .align 2
    .global _start

_start:
    // Base pointer for scratch memory @ 0x0010_0000
    movw    r6, #0
    movt    r6, #0x0010

back:                       // <— backward target

    // ADR forward: place 'fwd' 0x0C bytes ahead of the PC used here
    adr     r0, fwd         // expect r0 = address of 'fwd' label

    // ADR backward: from here PC==(this addr + 8); back is 0x10 behind
    adr     r1, back        // expect r1 = address of 'back' label (above)

    // pad so 'fwd' ends up exactly 0x0C ahead of the PC at the ADR above
    mov     r12, r12
    mov     r3,  r3
    mov     r2,  r2

fwd:                        // <— forward target

    // Store computed addresses so we can assert them
    str     r0, [r6, #0]
    str     r1, [r6, #4]

    // Stop
    bkpt    0x1234
	