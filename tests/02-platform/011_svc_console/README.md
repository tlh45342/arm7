# 011_svc_console

First tiny kernel/SVC boundary proof.

Layout:
- kernel + vector table at 0x00000000
- user payload at 0x00010000
- CRT MMIO at 0x0A000000

ABI v0.1:
- r7 = service number
- r0 = first argument / return value
- 1 = putc
- 2 = puts
- 3 = exit

The kernel owns CRT access. User code prints only through SVC.

`printf` is intentionally not a syscall; formatting belongs in libc later.

The kernel `putc` includes newline handling, wrapping, and bottom-row scrolling.

Run:
    make -C tests\02-platform\011_svc_console test

If this fails around the first `svc #0`, that is useful: the VM's SVC
instruction/exception semantics are the next implementation slice, and this
test becomes the regression contract for that work.
