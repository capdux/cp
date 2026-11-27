# smart-buffer — AI Agent Quick Ref

Zero-heap cursor-based binary reader/writer buffer with big/little-endian integer and string operations (`v4.2.0`, `MIT`)

## Import

```cp
import {
    SmartBuffer,
    sbufMin,
    sbufW16BE,
    sbufW16LE,
    sbufW32BE,
    sbufW32LE,
    sbufR16BE,
    sbufR16LE,
} from "smart-buffer";
```

## Core Types & Constants

```cp
class SmartBuffer { buf: [1024]u8, read_offset: usize, write_offset: usize } // methods: new(...) +17
```

## API Signatures

```cp
fn sbufMin(a: usize, b: usize): usize;
fn sbufW16BE(dst: []u8, off: usize, v: u16): bool;
fn sbufW16LE(dst: []u8, off: usize, v: u16): bool;
fn sbufW32BE(dst: []u8, off: usize, v: u32): bool;
fn sbufW32LE(dst: []u8, off: usize, v: u32): bool;
fn sbufR16BE(src: &[]u8, off: usize): u16?;
fn sbufR16LE(src: &[]u8, off: usize): u16?;
fn sbufR32BE(src: &[]u8, off: usize): u32?;
fn sbufR32LE(src: &[]u8, off: usize): u32?;
```

## Minimal Usage

```cp
import { sbufMin } from "smart-buffer";

pub fn main() {
    let result = sbufMin(0, 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"smart-buffer"`; install via `cap add smart-buffer`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
