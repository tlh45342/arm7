# 107_test_strb

Dedicated behavioral test for A32 `STRB`.

Coverage:

```text
STRB [Rn]
STRB [Rn, #imm]
STRB [Rn], #imm
STRB [Rn, #imm]!
STRB [Rn, Rm]
```

The test verifies both stored byte values and addressing-mode writeback.

`LDRB` is used only as the observation mechanism; `106_test_ldrb` should be
green independently.

Run:

```text
make clean
make test
```
