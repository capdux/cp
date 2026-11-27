# unicode-xid — AI Agent Quick Ref

Zero-allocation UAX #31 Unicode Identifier (XID_Start / XID_Continue) classification and identifier scanner for CP (clean-room MIT). (`v1.0.0`, `MIT`)

## Import

```cp
import {
    UxidRune,
    uxidDecodeUtf8Strict,
    uxidIsExtraStart,
    uxidBsearchStart,
    uxidBsearchCont,
    uxidIsAsciiAlpha,
    uxidIsAsciiDigit,
    isXidStart,
} from "unicode-xid";
```

## Core Types & Constants

```cp
class UxidRune { cp: u32, len: usize, valid: bool }
```

## API Signatures

```cp
fn uxidDecodeUtf8Strict(s: string, offset: usize): UxidRune;
fn uxidIsExtraStart(cp: u32): bool;
fn uxidBsearchStart(cp: u32): bool;
fn uxidBsearchCont(cp: u32): bool;
fn uxidIsAsciiAlpha(b: u8): bool;
fn uxidIsAsciiDigit(b: u8): bool;
fn isXidStart(cp: u32): bool;
fn isXidContinue(cp: u32): bool;
fn isIdentStart(cp: u32): bool;
fn isAsciiIdent(s: string): bool;
fn isValidIdent(s: string): bool;
fn scanIdentLen(s: string, start: usize): usize;
```

## Minimal Usage

```cp
import { isAsciiIdent } from "unicode-xid";

pub fn main() {
    let result = isAsciiIdent("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"unicode-xid"`; install via `cap add unicode-xid`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
