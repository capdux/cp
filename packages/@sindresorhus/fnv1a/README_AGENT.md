# @sindresorhus/fnv1a — AI Agent Quick Ref

Zero-heap FNV-1 and FNV-1a 32-bit and 64-bit streaming non-cryptographic hash functions for CP (compatible with Rust fnv & npm @sindresorhus/fnv1a) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    fnv64OffsetBasis,
    fnv64Prime,
    mulWrap32,
    mulFnv64Prime,
    fnv1a32Update,
    fnv1a32,
    fnv132,
    fnv1a64Update,
} from "@sindresorhus/fnv1a";
```

## API Signatures

```cp
fn fnv64OffsetBasis(): u64;
fn fnv64Prime(): u64;
fn mulWrap32(a: u32, b: u32): u32;
fn mulFnv64Prime(h: u64): u64;
fn fnv1a32Update(state: u32, data: &[]u8): u32;
fn fnv1a32(data: &[]u8): u32;
fn fnv132(data: &[]u8): u32;
fn fnv1a64Update(state: u64, data: &[]u8): u64;
fn fnv1a64(data: &[]u8): u64;
fn fnv164(data: &[]u8): u64;
fn formatHex64(h: u64, out: []u8): usize;
```

## Minimal Usage

```cp
import { formatHex64 } from "@sindresorhus/fnv1a";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatHex64(0, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@sindresorhus/fnv1a"`; install via `cap add @sindresorhus/fnv1a`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
