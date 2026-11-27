# potential_utf — AI Agent Quick Ref

Zero-allocation potentially-ill-formed UTF-8, UTF-16, and Unicode scalar value wrappers (ICU4X potential_utf parity) for CP (`v1.0.0`, `Unicode-3.0`)

## Import

```cp
import {
    PotentialUtf8,
    PotentialUtf16,
    PotentialCodePoint,
    putfIsCont,
    putfValidSeqLen,
    putfMaximalSubpart,
    putfEncodeScalar,
    putfWriteReplacement,
} from "potential_utf";
```

## Core Types & Constants

```cp
class PotentialUtf8 { bytes: &[]u8 } // methods: fromBytes(...) +7
class PotentialUtf16 { units: &[]u16 } // methods: fromSlice(...) +5
class PotentialCodePoint { raw: u32 } // methods: fromU32(...) +4
```

## API Signatures

```cp
fn putfIsCont(b: u8): bool;
fn putfValidSeqLen(bytes: &[]u8, pos: usize): usize;
fn putfMaximalSubpart(bytes: &[]u8, pos: usize): usize;
fn putfEncodeScalar(scalar: u32, out: []u8, w: usize): usize;
fn putfWriteReplacement(out: []u8, w: usize): usize;
```

## Minimal Usage

```cp
import { putfIsCont } from "potential_utf";

pub fn main() {
    let result = putfIsCont(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"potential_utf"`; install via `cap add potential_utf`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
