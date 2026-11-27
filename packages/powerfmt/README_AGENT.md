# powerfmt — AI Agent Quick Ref

Zero-heap high-performance integer formatting (alignment, padding, radix 2/8/10/16, digit grouping) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    IntFmtSpec,
    digitByte,
    writeByte,
    writeRepeat,
    formatMagnitude,
    formatU64,
    formatI64,
    formatDecPadded,
} from "powerfmt";
```

## Core Types & Constants

```cp
class IntFmtSpec { min_width: usize, pad_byte: u8, alignment: u8, radix: u8, /* +4 fields */ } // methods: defaultSpec(...)
```

## API Signatures

```cp
fn digitByte(d: u64, uppercase: bool): u8;
fn writeByte(out: []u8, pos: usize, b: u8): usize;
fn writeRepeat(out: []u8, pos: usize, b: u8, count: usize): usize;
fn formatMagnitude(mag: u64, sign_byte: u8, spec: IntFmtSpec, out: []u8): usize;
fn formatU64(val: u64, spec: IntFmtSpec, out: []u8): usize;
fn formatI64(val: i64, spec: IntFmtSpec, out: []u8): usize;
fn formatDecPadded(val: i64, width: usize, out: []u8): usize;
fn formatHex(val: u64, uppercase: bool, out: []u8): usize;
fn formatWithCommas(val: i64, out: []u8): usize;
```

## Minimal Usage

```cp
import { formatU64 } from "powerfmt";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatU64(0, /* spec: IntFmtSpec */, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"powerfmt"`; install via `cap add powerfmt`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
