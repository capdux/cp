# percent-encoding — AI Agent Quick Ref

Pure CP zero-allocation WHATWG URL Standard percent-encoding and decoding (Rust percent-encoding parity) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    AsciiSet,
    peBitMask,
    peHexUpper,
    peHexVal,
    controlsSet,
    fragmentSet,
    querySet,
    pathSet,
} from "percent-encoding";
```

## Core Types & Constants

```cp
class AsciiSet { bits: [4]u32 } // methods: empty(...) +4
```

## API Signatures

```cp
fn peBitMask(byte: u8): u32;
fn peHexUpper(nib: u8): u8;
fn peHexVal(ch: u8): u8;
fn controlsSet(): AsciiSet;
fn fragmentSet(): AsciiSet;
fn querySet(): AsciiSet;
fn pathSet(): AsciiSet;
fn userinfoSet(): AsciiSet;
fn componentSet(): AsciiSet;
fn nonAlphanumericSet(): AsciiSet;
fn percentEncodeLen(input: &[]u8, set: AsciiSet): usize;
fn percentEncode(input: &[]u8, set: AsciiSet, out: []u8): usize;
fn percentDecode(input: string, out: []u8): usize;
fn percentDecodeStrict(input: string, out: []u8): i64;
fn percentDecodePlus(input: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { percentDecode } from "percent-encoding";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = percentDecode("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"percent-encoding"`; install via `cap add percent-encoding`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
