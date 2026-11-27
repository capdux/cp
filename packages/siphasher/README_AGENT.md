# siphasher — AI Agent Quick Ref

Zero-allocation SipHash-2-4 and SipHash-1-3 keyed streaming PRF/hashers (HashDoS-resistant) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    SipState,
    SipHasher24,
    SipHasher13,
    addWrap64,
    rotl64,
    sipRound,
    loadLe64,
    loadLe64Str,
} from "siphasher";
```

## Core Types & Constants

```cp
class SipState { v0: u64, v1: u64, v2: u64, v3: u64 }
class SipHasher24 { v0: u64, v1: u64, v2: u64, v3: u64, /* +3 fields */ } // methods: init(...) +3
class SipHasher13 { v0: u64, v1: u64, v2: u64, v3: u64, /* +3 fields */ } // methods: init(...) +3
```

## API Signatures

```cp
fn addWrap64(a: u64, b: u64): u64;
fn rotl64(x: u64, r: u64): u64;
fn sipRound(s: SipState): SipState;
fn loadLe64(buf: []u8, offset: usize): u64;
fn loadLe64Str(s: string, offset: usize): u64;
fn siphash24(k0: u64, k1: u64, data: string): u64;
fn siphash13(k0: u64, k1: u64, data: string): u64;
```

## Minimal Usage

```cp
import { loadLe64Str } from "siphasher";

pub fn main() {
    let result = loadLe64Str("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"siphasher"`; install via `cap add siphasher`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
