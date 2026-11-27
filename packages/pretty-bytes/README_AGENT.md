# pretty-bytes — AI Agent Quick Ref

Pure CP zero-allocation human-readable byte and bit size formatter supporting SI/IEC, bits, signed, and compact modes (npm pretty-bytes parity) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    PrettyBytesOpts,
    pbUnitStr,
    pbAppendStr,
    pbAppendByte,
    pbAppendU64,
    prettyBytesWith,
    prettyBytes,
} from "pretty-bytes";
```

## Core Types & Constants

```cp
class PrettyBytesOpts { binary: bool, bits: bool, signed_val: bool, space: bool, /* +1 fields */ } // methods: defaults(...)
```

## API Signatures

```cp
fn pbUnitStr(idx: usize, binary: bool, bits: bool): string;
fn pbAppendStr(out: []u8, pos: &mut usize, s: string): bool;
fn pbAppendByte(out: []u8, pos: &mut usize, ch: u8): bool;
fn pbAppendU64(out: []u8, pos: &mut usize, v: u64): bool;
fn prettyBytesWith(num: f64, opts: PrettyBytesOpts, out: []u8): usize;
fn prettyBytes(num: f64, out: []u8): usize;
```

## Minimal Usage

```cp
import { pbAppendStr } from "pretty-bytes";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = pbAppendStr(out_buf[0..256], /* pos: &mut usize */, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"pretty-bytes"`; install via `cap add pretty-bytes`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
