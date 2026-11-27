# unicode-ident — AI Agent Quick Ref

Zero-heap UAX #31 Unicode identifier classification (XID_Start / XID_Continue) and UTF-8 identifier scanner in pure CP (`v1.0.0`, `MIT OR Apache-2.0`)

## Import

```cp
import {
    Utf8Codepoint,
    uidInRange,
    decodeUtf8At,
    uidIsXidStartNonAscii,
    uidIsXidContinueExtra,
    isXidStart,
    isXidContinue,
    isIdentStart,
} from "unicode-ident";
```

## Core Types & Constants

```cp
class Utf8Codepoint { cp: u32, len: usize, ok: bool }
```

## API Signatures

```cp
fn uidInRange(cp: u32, lo: u32, hi: u32): bool;
fn decodeUtf8At(s: string, offset: usize): Utf8Codepoint;
fn uidIsXidStartNonAscii(cp: u32): bool;
fn uidIsXidContinueExtra(cp: u32): bool;
fn isXidStart(cp: u32): bool;
fn isXidContinue(cp: u32): bool;
fn isIdentStart(cp: u32): bool;
fn scanIdentLen(s: string): usize;
fn isValidIdent(s: string): bool;
```

## Minimal Usage

```cp
import { decodeUtf8At } from "unicode-ident";

pub fn main() {
    let result = decodeUtf8At("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"unicode-ident"`; install via `cap add unicode-ident`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
