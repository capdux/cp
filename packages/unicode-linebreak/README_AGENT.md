# unicode-linebreak — AI Agent Quick Ref

Zero-allocation pure CP UAX #14 Unicode line break opportunity detector (unicode-linebreak parity) (`v1.0.0`, `Apache-2.0`)

## Import

```cp
import {
    BreakKind,
    LineBreak,
    UlbrDecoded,
    ulbrDecodeUtf8,
    ulbrClassify,
    ulbrEvalBreak,
    findLineBreaks,
    countLineBreaks,
} from "unicode-linebreak";
```

## Core Types & Constants

```cp
enum BreakKind { Mandatory, Allowed }
class LineBreak { offset: usize, kind: BreakKind }
class UlbrDecoded { cp: u32, len: usize }
```

## API Signatures

```cp
fn ulbrDecodeUtf8(s: string, pos: usize): UlbrDecoded;
fn ulbrClassify(cp: u32): u8;
fn ulbrEvalBreak(prev: u8, prevNonSp: u8, curr: u8): u8;
fn findLineBreaks(utf8: string, out: []LineBreak): usize;
fn countLineBreaks(utf8: string): usize;
```

## Minimal Usage

```cp
import { ulbrDecodeUtf8 } from "unicode-linebreak";

pub fn main() {
    let result = ulbrDecodeUtf8("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"unicode-linebreak"`; install via `cap add unicode-linebreak`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
