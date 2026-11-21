# `cap:coff` — Microsoft COFF / PE-COFF `.obj` Zero-Allocation Parser

Pure CP, zero-allocation parser for Microsoft COFF / PE-COFF `.obj` object files: 20-byte `IMAGE_FILE_HEADER`, 40-byte `IMAGE_SECTION_HEADER`, 18-byte `IMAGE_SYMBOL`, 10-byte `IMAGE_RELOCATION`, and COFF string table (supporting both `/<decimal>` long section names and `zeroes == 0` long symbol names).

## Protocol Constants

- Structure Sizes:
  - `COFF_HEADER_SIZE = 20`, `COFF_SECTION_SIZE = 40`, `COFF_SYMBOL_SIZE = 18`, `COFF_RELOC_SIZE = 10`
- Machine Types:
  - `MACHINE_UNKNOWN (0x0000)`, `MACHINE_I386 (0x014C)`, `MACHINE_ARM (0x01C0)`, `MACHINE_AMD64 (0x8664)`, `MACHINE_ARM64 (0xAA64)`, `MACHINE_RISCV64 (0x5064)`
- Section Characteristics:
  - `SCN_CNT_CODE (0x20)`, `SCN_CNT_INITIALIZED_DATA (0x40)`, `SCN_CNT_UNINITIALIZED_DATA (0x80)`
  - `SCN_MEM_EXECUTE (0x20000000)`, `SCN_MEM_READ (0x40000000)`, `SCN_MEM_WRITE (0x80000000)`
- Symbol Storage Classes & Special Section Numbers:
  - `SYM_CLASS_EXTERNAL (2)`, `SYM_CLASS_STATIC (3)`, `SYM_CLASS_FILE (103)`, `SYM_CLASS_SECTION (104)`
  - `SYM_UNDEFINED (0)`, `SYM_ABSOLUTE (-1)`, `SYM_DEBUG (-2)`

## Core Types & Functions

| Symbol | Description |
| :--- | :--- |
| `isKnownCoffMachine(machine: u16): bool` | Checks if a COFF machine ID is recognized. |
| `parseCoff(data: []const u8): CoffObject?` | Validates header, machine ID, section table, symbol table, and 4B string table bounds. |
| `CoffSection` | 40B section header (`isCode()`, `isReadable()`, `isWritable()`, `isExecutable()`, `shortName()`). |
| `CoffSymbol` | 18B symbol entry (`isExternal()`, `isStatic()`, `isUndefined()`, `isFunction()`). |
| `CoffReloc` | 10B relocation entry (`virtual_addr`, `symbol_idx`, `reloc_type`). |
| `CoffObject` methods | `sectionAt(idx)`, `sectionData(sec)`, `sectionName(sec)`, `symbolAt(idx)`, `symbolName(sym)`, `stringAt(off)`, `relocAt(sec, idx)`. |

## Example

```cp
import { parseCoff } from "cap:coff";

fn inspectObj(bytes: []const u8) {
    if (obj := parseCoff(bytes)) {
        if (sec := obj.sectionAt(0)) {
            * = obj.sectionName(sec);
            * = obj.sectionData(sec);
            * = obj.relocAt(sec, 0);
        }
        if (sym := obj.symbolAt(0)) {
            * = obj.symbolName(sym);
        }
    }
}
```
