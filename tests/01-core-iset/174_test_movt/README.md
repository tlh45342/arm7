# 174_test_movt

Dedicated behavioral test for A32 `MOVT`.

`MOVT` writes the upper halfword of the destination register and preserves the
existing lower halfword.

The test covers:

```text
low=0x1234, high=0xABCD -> 0xABCD1234
low=0xFFFF, high=0x0000 -> 0x0000FFFF
low=0x0000, high=0xFFFF -> 0xFFFF0000
low=0x5678, high=0x1234 -> 0x12345678
```

The last case also validates the common `MOVW` + `MOVT` pattern used throughout
the ARM7 firmware and test programs.

Run:

```text
make clean
make test
```
