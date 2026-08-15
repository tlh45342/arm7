# 105_test_str

Dedicated behavioral test for A32 `STR`.

Coverage:

```text
STR [Rn]
STR [Rn, #imm]
STR [Rn], #imm
STR [Rn, #imm]!
STR [Rn, Rm]
```

The test also verifies post-index and pre-index writeback.

`LDR` is used only to observe the resulting memory contents. The dedicated
`104_test_ldr` test should be green before relying on this test.

Run:

```text
make clean
make test
```
