# radsort — AI Agent Quick Ref

Zero-allocation O(N) 8-bit LSD radix sort for u32, i32, u64, i64, and IEEE-754 f64 with pass skipping for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    i32ToSortable,
    i64ToSortable,
    f64ToSortable,
    sortU32,
    sortI32,
    sortU64,
    sortI64,
    sortF64,
} from "radsort";
```

## API Signatures

```cp
fn i32ToSortable(v: i32): u32;
fn i64ToSortable(v: i64): u64;
fn f64ToSortable(v: f64): u64;
fn sortU32(data: []u32, scratch: []u32): bool;
fn sortI32(data: []i32, scratch: []i32): bool;
fn sortU64(data: []u64, scratch: []u64): bool;
fn sortI64(data: []i64, scratch: []i64): bool;
fn sortF64(data: []f64, scratch: []f64): bool;
fn isSortedI64(data: &[]i64): bool;
fn isSortedU64(data: &[]u64): bool;
```

## Minimal Usage

```cp
import { isSortedI64 } from "radsort";

pub fn main() {
    let result = isSortedI64(/* data: &[]i64 */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"radsort"`; install via `cap add radsort`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
