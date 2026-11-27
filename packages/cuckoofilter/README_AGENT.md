# cuckoofilter — AI Agent Quick Ref

Zero-allocation CoNEXT'14 Cuckoo Filter (256x4 16-bit fingerprint slots) with O(1) insert, contains, and safe delete for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    CuckooFilter,
    mulWrap64,
    avalanche64,
    hashBytes,
    fingerprintFromHash,
    altBucket,
} from "cuckoofilter";
```

## Core Types & Constants

```cp
class CuckooFilter { buckets: [1024]u16, item_count: usize, rng_state: u64 } // methods: init(...) +12
```

## API Signatures

```cp
fn mulWrap64(a: u64, b: u64): u64;
fn avalanche64(x: u64): u64;
fn hashBytes(data: &[]u8): u64;
fn fingerprintFromHash(h: u64): u16;
fn altBucket(bucket: usize, fp: u16): usize;
```

## Minimal Usage

```cp
import { hashBytes } from "cuckoofilter";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = hashBytes(out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"cuckoofilter"`; install via `cap add cuckoofilter`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
