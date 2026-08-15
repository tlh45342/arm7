.syntax unified
.arch armv7-a
.arm
.equ RESULT, 0x0003F100
.section .vectors,"ax"
.global _start
_start:
 b reset
 b bad
 b svc_entry
 b bad
 b bad
 b bad
 b bad
 b bad
.section .text,"ax"
reset:
 movw sp,#0xC000
 movt sp,#0x001F
 movw r0,#0
 movt r0,#1
 bx r0
svc_entry:
 push {r1-r6,lr}
 mov r6,sp
 bl svc_dispatch
 cmp sp,r6
 bne bad
 pop {r1-r6,lr}
 movs pc,lr
svc_dispatch:
 push {r4,lr}
 mov r4,r0
 bl helper
 add r0,r4,r0
 pop {r4,lr}
 bx lr
helper:
 push {lr}
 mov r0,#1
 pop {lr}
 bx lr
bad:
 movw r0,#0xBAD
 movw r1,#:lower16:RESULT
 movt r1,#:upper16:RESULT
 str r0,[r1]
 bkpt #0x141
