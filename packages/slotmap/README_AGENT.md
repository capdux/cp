# slotmap@1.0.0 (Agent Quickref)

> Zero-allocation generational-index slot map (SlotKey, SlotMapI64, SecondaryMapI64) with O(1) free-list and strict ABA protection (Rust slotmap v1 parity) for CP | License: MIT
> Import: `import { ... } from "slotmap";` | Full types: `src/index.d.cp`

## Functions
- `export fn slmIsOccupiedVersion(version: u32): bool;`
- `export fn slmAllocVersion(cur_version: u32): u32;`
- `export fn slmFreeVersion(cur_version: u32): u32;`
