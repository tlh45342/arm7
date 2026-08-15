.syntax unified
.arch armv7-a
.arm
.text
.global _start
.equ RESULT,0x0003F100
_start:
 movw sp,#0xA000
 movt sp,#0x001E
 movw r1,#0x1111
 movt r1,#0x1111
 movw r2,#0x2222
 movt r2,#0x2222
 movw r3,#0x3333
 movt r3,#0x3333
 movw r4,#0x4444
 movt r4,#0x4444
 movw r5,#0x5555
 movt r5,#0x5555
 mov r0,#41
 svc #0
 cmp r0,#42
 bne fail
 movw r6,#0x1111
 movt r6,#0x1111
 cmp r1,r6
 bne fail
 movw r6,#0x2222
 movt r6,#0x2222
 cmp r2,r6
 bne fail
 movw r6,#0x3333
 movt r6,#0x3333
 cmp r3,r6
 bne fail
 movw r6,#0x4444
 movt r6,#0x4444
 cmp r4,r6
 bne fail
 movw r6,#0x5555
 movt r6,#0x5555
 cmp r5,r6
 bne fail
 movw r1,#:lower16:RESULT
 movt r1,#:upper16:RESULT
 movw r2,#0x4B4F
 movt r2,#0x4353       @ "SCOK"
 str r2,[r1]
 bkpt #0x140
fail:
 movw r1,#:lower16:RESULT
 movt r1,#:upper16:RESULT
 movw r2,#0x4146
 movt r2,#0x4C49
 str r2,[r1]
 bkpt #0x142
