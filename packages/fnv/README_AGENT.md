# fnv — AI Agent Quick Ref

Zero-allocation FNV-1a 32/64-bit and Rust rustc-hash FxHash 64-bit streaming non-cryptographic hashers for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Fnv32Hasher,
    Fnv64Hasher,
    FxHasher,
    mulWrap32,
    mulWrap64,
    rotl64By5,
    fnv1a32,
    fnv1a64,
} from "fnv";
```

## Core Types & Constants

```cp
class Fnv32Hasher { state: u32 } // methods: init(...) +3
class Fnv64Hasher { state: u64 } // methods: init(...) +3
class FxHasher { state: u64 } // methods: init(...) +4
```

## API Signatures

```cp
fn mulWrap32(a: u32, b: u32): u32;
fn mulWrap64(a: u64, b: u64): u64;
fn rotl64By5(x: u64): u64;
fn fnv1a32(data: string): u32;
fn fnv1a64(data: string): u64;
fn fxHash64(data: string): u64;
```

## Minimal Usage

```cp
import { fnv1a32 } from "fnv";

pub fn main() {
    let result = fnv1a32("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"fnv"`; install via `cap add fnv`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
