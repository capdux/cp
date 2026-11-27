# rend — AI Agent Quick Ref

Zero-allocation endian-aware primitive types (U16/U32/I32/U64/I64 Be/Le) and bounds-checked slice readers for zero-copy binary protocols in CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    U16Be,
    U16Le,
    U32Be,
    U32Le,
    I32Be,
    I32Le,
    U64Be,
    U64Le,
} from "rend";
```

## Core Types & Constants

```cp
class U16Be { bytes: [2]u8 } // methods: fromNative(...) +3
class U16Le { bytes: [2]u8 } // methods: fromNative(...) +3
class U32Be { bytes: [4]u8 } // methods: fromNative(...) +3
class U32Le { bytes: [4]u8 } // methods: fromNative(...) +3
class I32Be { bytes: [4]u8 } // methods: fromNative(...) +3
class I32Le { bytes: [4]u8 } // methods: fromNative(...) +3
class U64Be { bytes: [8]u8 } // methods: fromNative(...) +3
class U64Le { bytes: [8]u8 } // methods: fromNative(...) +3
class I64Be { bytes: [8]u8 } // methods: fromNative(...) +3
class I64Le { bytes: [8]u8 } // methods: fromNative(...) +3
```

## API Signatures

```cp
fn rndPackU16Be(v: u16): [2]u8;
fn rndPackU16Le(v: u16): [2]u8;
fn rndUnpackU16Be(b: [2]u8): u16;
fn rndUnpackU16Le(b: [2]u8): u16;
fn rndPackU32Be(v: u32): [4]u8;
fn rndPackU32Le(v: u32): [4]u8;
fn rndUnpackU32Be(b: [4]u8): u32;
fn rndUnpackU32Le(b: [4]u8): u32;
fn rndPackU64Be(v: u64): [8]u8;
fn rndPackU64Le(v: u64): [8]u8;
fn rndUnpackU64Be(b: [8]u8): u64;
fn rndUnpackU64Le(b: [8]u8): u64;
fn readU16Be(buf: &[]u8, offset: usize): u16?;
fn readU16Le(buf: &[]u8, offset: usize): u16?;
fn readU32Be(buf: &[]u8, offset: usize): u32?;
fn readU32Le(buf: &[]u8, offset: usize): u32?;
fn readU64Be(buf: &[]u8, offset: usize): u64?;
fn readU64Le(buf: &[]u8, offset: usize): u64?;
```

## Minimal Usage

```cp
import { rndPackU16Be } from "rend";

pub fn main() {
    let result = rndPackU16Be(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"rend"`; install via `cap add rend`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
