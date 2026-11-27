# url-template — AI Agent Quick Ref

Zero-allocation RFC 6570 URI Template (Levels 1-4) expander and validator for CP. (`v0.1.0`, `MIT`)

## Import

```cp
import {
    TemplateVar,
    utIsAlpha,
    utIsDigit,
    utIsHex,
    utHexUpper,
    utIsUnreserved,
    utIsReserved,
    utIsOperator,
} from "url-template";
```

## Core Types & Constants

```cp
class TemplateVar { name: string, value: string }
```

## API Signatures

```cp
fn utIsAlpha(c: u8): bool;
fn utIsDigit(c: u8): bool;
fn utIsHex(c: u8): bool;
fn utHexUpper(n: u8): u8;
fn utIsUnreserved(c: u8): bool;
fn utIsReserved(c: u8): bool;
fn utIsOperator(c: u8): bool;
fn utIsReservedOp(c: u8): bool;
fn utEqBytes(a: &[]u8, b: &[]u8): bool;
fn utIsValidVarSpec(spec: &[]u8): bool;
fn isValid(tmpl: string): bool;
fn countVars(tmpl: string): usize;
fn extractVarName(tmpl: string, index: usize, out: []u8): usize;
fn pctEncode(input: string, allow_reserved: bool, out: []u8): usize;
fn utTruncateUtf8(val: &[]u8, maxChars: usize): &[]u8;
fn utWriteSlice(out: []u8, pos: &mut usize, src: &[]u8);
fn utWriteByte(out: []u8, pos: &mut usize, b: u8);
fn expand(tmpl: string, vars: &[]TemplateVar, out: []u8): usize;
```

## Minimal Usage

```cp
import { isValid } from "url-template";

pub fn main() {
    let result = isValid("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"url-template"`; install via `cap add url-template`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
