 .section .text
    .global _start

_start:
    mvn     sp, #~0x1ffffffc      @ Set up stack near top of RAM
    ldr     r0, =msg              @ r0 = pointer to message
    bl      put_chars             @ Call put_chars(r0)

    .word   0xDEADBEEF            @ Sentinel to halt execution

@ ---------------------------------------
@ void put_chars(char *s)
@ r0 = pointer to string
@ ---------------------------------------
put_chars:
    push    {fp, lr}              @ Save frame pointer and return address
    mov     fp, sp                @ Set up new frame
    mov     r2, r0                @ Copy pointer to r2 (string walker)

.loop:
    ldrb    r0, [r2], #1          @ Load byte from [r2], increment r2
    cmp     r0, #0                @ Check for null terminator
    beq     .done                 @ Exit loop if null terminator
    bl      crt_putc              @ Call crt_putc(r0)
    b       .loop                 @ Repeat loop

.done:
    pop     {fp, lr}              @ Restore frame pointer and return address
    bx      lr                    @ Return from function

@ ---------------------------------------
@ Stub for crt_putc (needed to link if VM handles it)
@ ---------------------------------------
.global crt_putc
crt_putc:
    bx lr                        @ Stub: does nothing
	
@ ---------------------------------------
@ Message string (null-terminated)
@ ---------------------------------------
msg:
    .asciz  "XYPQ\n"
