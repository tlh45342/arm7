    .syntax unified
    .thumb
    .thumb_func
    .global _start
.text
_start:
    movs    r1, #0        @ init so we know what "no-then" leaves behind

    movs    r0, #0
    cmp     r0, #0        @ Z=1
    ite     eq            @ THEN, ELSE
      moveq  r1, #1       @ allowed (Thumb-16)
      movne  r1, #2       @ allowed (Thumb-16)
    bkpt    #0xAB         @ r1 == 1 here; change r0 to 1 to see r1 == 2