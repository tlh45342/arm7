# 104_test_ldr

Dedicated behavioral test for A32 `LDR`.

The test seeds a small RAM region and validates:

```text
LDR [Rn]
LDR [Rn, #imm]
LDR [Rn], #imm
LDR [Rn, #imm]!
LDR [Rn, Rm]
```

It also verifies post-index and pre-index writeback.

This deliberately exercises several common addressing modes because firmware,
runtime code, and a future monitor will rely heavily on them.

Run:

```text
make clean
make test
```
