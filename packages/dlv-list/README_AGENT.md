# dlv-list — AI Agent Quick Ref

Zero-heap Vec-backed doubly linked list with stable u32 handles and O(1) slot reuse (`v0.5.2`, `MIT`)

## Import

```cp
import {
    DlvListI64,
    dlvlZeroI64,
    dlvlNoneU32,
    dlvlFalseBool,
    dlvlAllocSlot,
} from "dlv-list";
```

## Core Types & Constants

```cp
class DlvListI64 { values: [DLV_CAP]i64, prev: [DLV_CAP]u32, next: [DLV_CAP]u32, occupied: [DLV_CAP]bool, /* +5 fields */ } // methods: new(...) +10
```

## API Signatures

```cp
fn dlvlZeroI64(): [DLV_CAP]i64;
fn dlvlNoneU32(): [DLV_CAP]u32;
fn dlvlFalseBool(): [DLV_CAP]bool;
fn dlvlAllocSlot(list: &mut DlvListI64): u32;
```

## Minimal Usage

```cp
import { dlvlZeroI64 } from "dlv-list";

pub fn main() {
    let result = dlvlZeroI64();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"dlv-list"`; install via `cap add dlv-list`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
