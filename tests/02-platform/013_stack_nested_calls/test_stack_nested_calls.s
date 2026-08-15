.syntax unified
.arch armv7-a
.arm
.text
.global _start
_start:
    movw sp, #0xD000
    movt sp, #0x001D
    mov r8, sp
    mov r0, #5
    bl level1
    cmp r0, #8
    bne fail
    cmp sp, r8
    bne fail
    movw r11, #0x4B4F
    movt r11, #0x434E      @ "NCOK"
    bkpt #0x130
fail:
    movw r11, #0x4146
    movt r11, #0x4C49
    bkpt #0x131

level1:
    push {r4, lr}
    mov r4, #1
    bl level2
    add r0, r0, r4
    pop {r4, lr}
    bx lr

level2:
    push {r4, lr}
    mov r4, #2
    bl level3
    add r0, r0, r4
    pop {r4, lr}
    bx lr

level3:
    push {lr}
    add r0, r0, #0
    pop {lr}
    bx lr
