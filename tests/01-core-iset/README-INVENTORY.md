# ARM7 ISA Inventory Runner v2

Windows UTF-8 fix for the inventory runner.

The individual ARM7 instruction tests use Unicode status glyphs such as the
green check mark. When their output is captured by another Python process on
Windows, Python can otherwise fall back to cp1252 and raise
`UnicodeEncodeError`.

v2 forces child Python processes to UTF-8 with:

```text
PYTHONIOENCODING=utf-8
PYTHONUTF8=1
```

It also preserves each test's complete output under:

```text
inventory-logs/
```

Run from the repository root:

```text
python tests\01-core-iset\run_all_tests.py
```

The summary is written to:

```text
tests\01-core-iset\ISA-STATUS.txt
```
