# ARM7 Flat CRT Hello Test

This is a deliberately tiny bare-metal ARM flat binary.

Its only job is to prove the shortest possible path:

```text
ARM instructions
    -> guest memory writes
    -> CRT memory at 0x0A000000
    -> arm7-run / arm7-runx display
```

It does **not** use:

- BIOS
- SVC
- stack
- C runtime
- UART
- disk
- filesystem

## Instructions intentionally used

The program stays inside instruction families already exercised by the ARM7
core tests:

```text
MOV
MOVW / MOVT
STRB
ADD
BKPT
```

## CRT contract

```text
CRT base:      0x0A000000
Columns:       80
Rows:          25
Bytes/cell:    2

cell + 0       character
cell + 1       attribute
```

The test writes `HELLO WORLD` beginning at row 0, column 0 with attribute `0x07`.

## Build

```text
make
```

Expected artifacts:

```text
hello-crt.bin
hello-crt.elf
hello-crt.lst
```

## Test with arm7-run

```text
arm7-run < hello-crt.script
```

Expected CRT content:

```text
HELLO WORLD
```

## Test with arm7-runx

1. Start `arm7-runx`.
2. Choose `Firmware -> Load Flat Binary at 0x8000...`.
3. Select `hello-crt.bin`.
4. Use `Machine -> Run 100 Instructions` or press `F9`.
5. Refresh with `F5` if needed.

`HELLO WORLD` should appear at the upper-left corner of the CRT window.

This is intentionally a stepping-stone test. Once this works, the next layer
can replace the hard-coded character stores with a small string loop / putc
primitive.
