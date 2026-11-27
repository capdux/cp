# arrayvec — AI Agent Quick Ref

Zero-allocation stack-backed fixed-capacity containers: ArrayString64, ArrayString256, ArrayVecI64, ArrayVecU64 with O(1) swapRemove and truncating push (Rust arrayvec parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import { ArrayString64, ArrayString256, ArrayVecI64, ArrayVecU64 } from "arrayvec";
```

## Core Types & Constants

```cp
class ArrayString64 { buf: [STR64_CAP]u8, count: usize } // methods: init(...) +13
class ArrayString256 { buf: [STR256_CAP]u8, count: usize } // methods: init(...) +13
class ArrayVecI64 { items: [VEC64_CAP]i64, count: usize } // methods: init(...) +12
class ArrayVecU64 { items: [VEC64_CAP]u64, count: usize } // methods: init(...) +12
```

## Minimal Usage

```cp
import { ArrayString64 } from "arrayvec";

pub fn main() {
    // Refer to src/index.d.cp for ArrayString64 fields and methods
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"arrayvec"`; install via `cap add arrayvec`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add arrayvec
cpc check src/main.cp
```
