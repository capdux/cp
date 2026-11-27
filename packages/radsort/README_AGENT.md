# radsort@1.0.0 (Agent Quickref)

> Zero-allocation O(N) 8-bit LSD radix sort for u32, i32, u64, i64, and IEEE-754 f64 with pass skipping for CP | License: MIT
> Import: `import { ... } from "radsort";` | Full types: `src/index.d.cp`

## Functions
- `export fn i32ToSortable(v: i32): u32;`
- `export fn i64ToSortable(v: i64): u64;`
- `export fn f64ToSortable(v: f64): u64;`
- `export fn sortU32(data: []u32, scratch: []u32): bool;`
- `export fn sortI32(data: []i32, scratch: []i32): bool;`
- `export fn sortU64(data: []u64, scratch: []u64): bool;`
- `export fn sortI64(data: []i64, scratch: []i64): bool;`
- `export fn sortF64(data: []f64, scratch: []f64): bool;`
- `export fn isSortedI64(data: &[]i64): bool;`
- `export fn isSortedU64(data: &[]u64): bool;`
