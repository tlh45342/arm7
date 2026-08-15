# ARM-VM / libvm
### _A small, readable ARMv7-A virtual machine core — built for learning, experimentation, and fun._

## 📦 Overview

**libvm** is a compact ARMv7-A virtual machine core implemented in C.  
It is intentionally **small**, **readable**, and **educational**, offering a clear view into how CPU decoding, execution, and virtual devices work.

This project is *not* trying to replace QEMU or become a full-system emulator.  
Instead, its purpose is:

- To be **simple enough** that a single person can understand the whole codebase  
- To be **accurate enough** to run real ARM assembly tests  
- To be **modular enough** to embed inside tools or experiments  
- To be **honest enough** to admit what it doesn’t do (yet)

If you’ve ever wanted to read the source of a CPU emulator and actually understand it, you are in the right place.

## ✨ What ARM-VM Does Today

### ✔ K12 Table-Driven Instruction Decode
Clean, tightly-scoped decode engine mapping ARM instructions → handler functions.

### ✔ Context-Based Execution Model
All CPU state is explicit and clearly separated from decode and execution logic.

### ✔ Core Instruction Coverage
Many ARM instructions implemented: data-processing, loads/stores, branch, BX/BL, CLZ, RBIT, bitfield ops, shifts, etc.

### ✔ Unified Memory Model
One RAM block for instruction fetch, data access, and MMIO trampoline.

### ✔ Basic Virtual Devices
UART console, CRT text device, disk front-end backed by a clean image manager.

### ✔ Growing Test Suite
Organized tests under `00-smoke`, `01-core-iset`, `03-misc`, and more.

## ⚠️ What ARM-VM Does *Not* Do Yet

- ❌ Not instruction-complete  
- ❌ No MMU / virtual memory  
- ❌ No exception model or banked registers  
- ❌ No SMP (yet)  
- ❌ No timing model  
- ❌ Cannot boot Linux or a full OS  

This is intentional: clarity before complexity.

## 📌 Project Status

| Component                     | Status |
|------------------------------|--------|
| Core CPU execution           | 🟡 Early but functional |
| K12 decode engine            | 🟢 Solid and extendable |
| Memory subsystem             | 🟢 Working |
| MMIO devices                 | 🟡 Basic |
| Instruction coverage         | 🟡 Partial |
| Debug tracing                | 🟢 Working |
| Test suite                   | 🟡 Growing |
| Documentation                | 🔵 In progress |

## 💡 Why This Project Exists

Most ARM emulators are either:

- huge and industrial (QEMU),  
- old,  
- closed or incomplete,  
- or too small/toy-like to be useful.

ARM-VM is meant to fill the gap:

✔ Small enough to understand  
✔ Accurate enough to be meaningful  
✔ A great platform for experimentation  
✔ Clean architecture for embedding  
✔ Educational and transparent  

There are few projects building ARMv7-A decode engines from scratch using a table-driven K12 model — this one is unusual in a good way.

## 🔧 Who This Is For

- Students learning CPU and ISA internals  
- OS developers wanting a minimal execution environment  
- Researchers exploring instrumentation or decode strategies  
- Hobbyists who enjoy low-level systems  
- Anyone wanting an ARM VM they can grok in a weekend  

## 🚀 Future Directions

- More full ARMv7-A instruction coverage  
- Simple MMU  
- Better device models  
- Multi-CPU (SMP)  
- Improved debugging UI  
- Plugin/device API  

## 🐟 Ecosystem

- **libvm** — VM core  
- **hostd** — VM host daemon  
- **vim-cmd** — VM control CLI  
- **guppy** — disk/image builder  

## 📜 License

Insert your license here.

## ✏️ Final Thoughts

ARM-VM is **not** the biggest or fastest ARM emulator.  
It *is* clear, hackable, and fun to read.  
A VM you can actually understand.

Welcome aboard. 🐟💻⚙️
