# arm7-run

`arm7-run` is the headless/reference ARM7 VM runner used by tests, firmware development, and automation.

## Layer 2 migration

`arm7-run` now owns its command-line runner sources:

```text
vm/arm7-run/
    src/
        cli.c
        main.c
        session.c
        session.h
```

The runner links against:

```text
vm/libarm7vm/lib/libarm7vm.a
```

The CPU, hardware, memory, and VM implementation are intentionally still compiled from the legacy `src/` tree by `libarm7vm`. This keeps Layer 2 structural: no CPU behavior is intentionally changed.

## Build

```text
make -C vm/arm7-run clean
make -C vm/arm7-run
```

## Test

```text
make -C vm/arm7-run test
```

The smoke test starts `arm7-run`, sends `quit`, and requires a clean exit.

## Migration boundary

After Layer 2:

```text
vm/arm7-run/src
        |
        v
   libarm7vm.a
        |
        v
legacy src/cpu + src/hw + src/vm + src/util
```

The next layer should establish a small public `libarm7vm` API before physically relocating the CPU and hardware implementation.
