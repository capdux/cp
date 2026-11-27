# slotmap — AI Agent Quick Ref

Zero-allocation generational-index slot map (SlotKey, SlotMapI64, SecondaryMapI64) with O(1) free-list and strict ABA protection (Rust slotmap v1 parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    SlotKey,
    SlotMapI64,
    SecondaryMapI64,
    slmIsOccupiedVersion,
    slmAllocVersion,
    slmFreeVersion,
} from "slotmap";
```

## Core Types & Constants

```cp
class SlotKey { idx: u32, version: u32 } // methods: nullKey(...) +4
class SlotMapI64 { values: [128]i64, versions: [128]u32, next_free: [128]u32, free_head: u32, /* +1 fields */ } // methods: new(...) +9
class SecondaryMapI64 { values: [128]i64, versions: [128]u32, count: usize } // methods: new(...) +6
```

## API Signatures

```cp
fn slmIsOccupiedVersion(version: u32): bool;
fn slmAllocVersion(cur_version: u32): u32;
fn slmFreeVersion(cur_version: u32): u32;
```

## Minimal Usage

```cp
import { slmIsOccupiedVersion } from "slotmap";

pub fn main() {
    let result = slmIsOccupiedVersion(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"slotmap"`; install via `cap add slotmap`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
