# untrusted — AI Agent Quick Ref

Zero-heap big-endian/little-endian binary codecs & bounds-checked untrusted ByteReader/ByteWriter (inspired by Rust byteorder + untrusted) (`v1.0.1`, `MIT`)

## Import

```cp
import {
    ByteReader,
    ByteWriter,
    readU16Be,
    readU16Le,
    readU32Be,
    readU32Le,
    readU64Be,
    readU64Le,
} from "untrusted";
```

## Core Types & Constants

```cp
class ByteReader { data: string, pos: usize } // methods: init(...) +13
class ByteWriter { out: []u8, pos: usize, overflow: bool } // methods: init(...) +9
```

## API Signatures

```cp
fn readU16Be(buf: string, off: usize): u16?;
fn readU16Le(buf: string, off: usize): u16?;
fn readU32Be(buf: string, off: usize): u32?;
fn readU32Le(buf: string, off: usize): u32?;
fn readU64Be(buf: string, off: usize): u64?;
fn readU64Le(buf: string, off: usize): u64?;
fn writeU16Be(out: []u8, off: usize, v: u16): bool;
fn writeU16Le(out: []u8, off: usize, v: u16): bool;
fn writeU32Be(out: []u8, off: usize, v: u32): bool;
fn writeU32Le(out: []u8, off: usize, v: u32): bool;
fn writeU64Be(out: []u8, off: usize, v: u64): bool;
fn writeU64Le(out: []u8, off: usize, v: u64): bool;
```

## Minimal Usage

```cp
import { readU16Be } from "untrusted";

pub fn main() {
    let result = readU16Be("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"untrusted"`; install via `cap add untrusted`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
