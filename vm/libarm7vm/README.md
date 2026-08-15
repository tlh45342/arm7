# libarm7vm

`libarm7vm` is the canonical host-side ARM7 virtual-machine core.

## Layer 3: public API seam

The proven CPU, hardware, memory, and VM implementation is still compiled from
the repository-root `src/` tree.  Layer 3 adds the first library-owned public
API in:

```text
vm/libarm7vm/include/arm7vm/arm7vm.h
vm/libarm7vm/src/arm7vm.c
```

The public machine type is opaque.  New host programs should migrate toward
this interface instead of depending directly on legacy `VM`, `CPU`, or device
structures.

The initial API covers:

```text
create / destroy
reset
load binary
set register / PC
step
run
read memory
clear halt
```

Device-management and richer machine-state inspection are intentionally left
for later layers.  The goal here is to establish a small stable seam, not to
expose every legacy function under a new name.

## Build

```text
make -C vm/libarm7vm clean
make -C vm/libarm7vm info
make -C vm/libarm7vm
```

## Test

```text
make -C vm/libarm7vm test
```

The public-API validation creates a small machine, loads a generated binary,
reads it back through the opaque API, exercises register validation, reset,
halt clearing, and destruction.

## Current layering

```text
arm7-run
    |
    |  (legacy API for the moment)
    v
libarm7vm.a
    |
    +-- public arm7vm API
    |
    +-- legacy src/cpu
    +-- legacy src/hw
    +-- legacy src/vm
    +-- legacy src/util
```

The next layer can migrate `arm7-run` incrementally onto this public API.
