# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

---

## 2025-11-22

### Changed
- Continued tightening of `exec_ctx` structure after migration away from `e->cpu`.
- Adjusted memory usage inside context-sensitive helpers to guarantee that all reads/writes consistently route through the VM’s unified memory model.
- Split tests into **smoke-tests** (basic execution validation) and **01-core-iset** (strict ISA correctness tests).
- It runs again - the ctx migration took a lot of work.  I can now get back on truck with instructions.

### Added
- Completed and stabilized K12-based disassembly for:
  - `AND`, `BIC`, `CMP`, `CMN`, `EOR`, `ORR`
  - full register-shift operand2 forms (LSL/LSR/ASR/ROR/RRX)
- Improved cycle-count logging and BKPT-based deterministic program termination.

---

## 2025-11-xx (November Overview)

### Added
- **Full ORR/ORRS instruction family support**, including:
  - rotated-immediate encodings (`#0x80000000`)
  - shifted register forms (LSL/LSR/ASR/ROR)
  - ORRS flag-setting semantics validated
  - conditional execution (`ORREQ`, `MOVEQ`) confirmed correct
- **Extended disassembler coverage**:
  - Implemented missing ORR/EOR key cases (`0x03A`, `0x03C`, `0x198`, `0x18E`)
  - Corrected K12 key extraction to match execution engine masks
  - Added clean Operand2 formatting for shift-based ORR/EOR patterns
- **New comprehensive tests added**:
  - `010_test_orr` — deep coverage of ORR, ORRS, rotated immediates, shifter carry, ROR forms  
  - Updated earlier tests (EOR/CMP/CMN/etc.) to conform to new CHECKS system

### Changed
- All disassembly lines now consistently populate output (`out[]`) instead of silently falling through to `.word`.
- Cleaned decode switch cases; removed non-matching keys due to older incorrect key generation.
- Normalized logging for STR pre-imm, CPSR snapshots, and PC step tracing.

### Fixed
- Longstanding issue where disasm’s K12 computation used incorrect bit slice (`>>20` instead of `>>16`) causing many valid instructions to appear as `.word`.
- Eliminated final leftover paths referencing `e->cpu` (all now use `e->vm->cpu`).
- Ensured that ORR/EOR reg+shift forms correctly propagate shifter carry into flag-setting instructions.

---

## 2025-10-xx

### Changed
- **Major `exec_ctx` refactor**:
  - Removed `cpu` pointer from `exec_ctx_t`; CPU is owned solely by the VM.
  - Updated all components (`memops`, `logic`, `media`, `operand`, `system`, `execute`) to reference CPU through the VM.
  - Eliminated last implicit global CPU references.

### Added
- **Disassembler expansion**:
  - Support for `CMN`, `CMP`, `AND/ANDS`, `BIC/BICS`, `EOR/EORS`, `ORR/ORRS`
  - Full register shift support (LSL/LSR/ASR/ROR/RRX)
- **Unified memory consistency pass**:
  - Ensured all memory operations, MMIO or raw, funnel through unified VM memory.
  - Corrected inconsistencies between VM-level and backend load/store operations.
- **Test harness overhaul**:
  - Introduced `run_tests.py` CHECKS-based validator.
  - Standardized test structure under `tests/01-core-iset`.
  - Added cycle-count checking and deterministic halts via BKPT.
  - Added instruction tests in:
    - `003_test_and`
    - `004_test_bic`
    - `005_test_cmn`
    - `006_test_cmp`
    - `008_test_eor`
    - `010_test_orr`
- **Decode-table improvements**:
  - Cleaned K12 generation
  - Ensured unknown-instruction detection gracefully halts VM with diagnostic context

---

## 2025-08-24

### Added
- Support for `ADC/ADCS`, `SBC/SBCS`, and `TST`.
- New regression tests:
  - `36_test_adcs/`
  - `37_test_sbcs/`
  - `38_test_tst/`

### Fixed
- Unified arithmetic flag-setting logic.
- Removed duplicate helpers and unused prototypes.
- Resolved CPS decode warning in `execute()`.

---

## 2025-08-23

### Added
- Support for `BIC/BICS`.
- Implemented SPSR restore behavior for `BICS pc, ...`
- Added `35_test_bics`.

---

## [0.0.56] - 2025-07-27
### Added
- Correct handling of `LDRB` post-indexed addressing.

### Fixed
- Instruction decoding improvements.

---

## [0.0.47] - 2025-07-27
### Added
- Support for `LDM` instruction variants  
- `bt` backtrace command  
- VM device support: CRT display, UART output  
- Error reporting on invalid memory access and unknown instruction

### Changed
- Improved stack bounds checking  
- Improved `execute()` decoding  
- WFI treated as NOP

---

## [0.0.39] - 2025-07-22
### Added
- `handle_ldrb_reg`
- Unknown instruction halt reporting

---

## [0.0.32] - 2025-07-20
### Added
- `do` script command in REPL

### Changed
- Removed “VM halted” banner
- Improved STR pre-indexed address decoding

---

## [0.0.29] - 2025-07-20
### Added
- Stack safety checks  
- Backtrace support

---

## [0.0.21] - 2025-07-20
### Added
- First functional ARM VM emulator  
- Core instruction decode: MOV, ADD, SUB, LDR, STR, BL  
- Flat 512MB virtual memory  
- REPL shell with commands (`load`, `run`, `regs`, `set`)  
- Debug flags (`DBG_INSTR`, `DBG_MEM`, etc.)  
- Binary loader using PC=0x8000

---

## [0.0.1] - 2025-07-?? (inception)
### Added
- First experimental build of ARM VM project

