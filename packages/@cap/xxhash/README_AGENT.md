# @cap/xxhash — AI Agent Quick Ref

xxHash 0.8.3 extremely fast non-cryptographic hash algorithm (XXH32/XXH64/XXH3) for CP (`v0.8.3`, `BSD-2-Clause`)

## Import

```cp
import {
    XXH_versionNumber,
    XXH32,
    XXH64,
    XXH3_64bits,
    versionNumber,
    xxh32,
    xxh64,
    xxh3_64bits,
} from "@cap/xxhash";
```

## API Signatures

```cp
fn XXH_versionNumber(): u32;
fn XXH32(input: [*]&u8!, length: usize, seed: u32): u32;
fn XXH64(input: [*]&u8!, length: usize, seed: u64): u64;
fn XXH3_64bits(input: [*]&u8!, length: usize): u64;
fn versionNumber(): u32;
fn xxh32(data: &[]u8, seed: u32): u32;
fn xxh64(data: &[]u8, seed: u64): u64;
fn xxh3_64bits(data: &[]u8): u64;
```

## Minimal Usage

```cp
import { versionNumber } from "@cap/xxhash";

pub fn main() {
    let result = versionNumber();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/xxhash"`; install via `cap add @cap/xxhash`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
