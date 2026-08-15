# 108_test_ldrh

Dedicated behavioral test for A32 `LDRH`.

Coverage:

```text
LDRH [Rn]
LDRH [Rn, #imm]
LDRH [Rn], #imm
LDRH [Rn, #imm]!
LDRH [Rn, Rm]
```

The test also verifies writeback for post-index and pre-index forms.

Loaded halfwords must be zero-extended to 32 bits.

Run:

```text
make clean
make test
```
