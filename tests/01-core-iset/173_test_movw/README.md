# 173_test_movw

Dedicated behavioral test for A32 `MOVW`.

The test verifies that several 16-bit immediates are written into the low
halfword of a destination register with the upper halfword cleared.

Cases:

```text
0x1234 -> 0x00001234
0xABCD -> 0x0000ABCD
0x0000 -> 0x00000000
0xFFFF -> 0x0000FFFF
```

Results are written to guest RAM and checked from the emulator log.

Run:

```text
make clean
make test
```
