# litrs — AI Agent Quick Ref

Zero-allocation Rust literal lexer, validator, and string unescaper (Rust litrs parity) for CP (`v1.0.0`, `MIT OR Apache-2.0`)

## Import

```cp
import {
    LiteralKind,
    ParsedLiteral,
    ltrInvalid,
    ltrStrEq,
    ltrIsIdentStart,
    ltrIsIdentCont,
    ltrIsValidSuffix,
    ltrHexVal,
} from "litrs";
```

## Core Types & Constants

```cp
enum LiteralKind { Invalid, Bool, Integer, Float, Char, Byte, ... }
class ParsedLiteral { kind: LiteralKind, raw: string, suffix: string, int_val: u64, /* +3 fields */ }
```

## API Signatures

```cp
fn ltrInvalid(src: string): ParsedLiteral;
fn ltrStrEq(a: string, b: string): bool;
fn ltrIsIdentStart(c: u8): bool;
fn ltrIsIdentCont(c: u8): bool;
fn ltrIsValidSuffix(s: string): bool;
fn ltrHexVal(c: u8): i32;
fn ltrDigitVal(c: u8, radix: u64): i32;
fn ltrParseRawString(src: string, start: usize, kind: LiteralKind): ParsedLiteral;
fn ltrParseQuotedString(src: string, quote_pos: usize, kind: LiteralKind): ParsedLiteral;
fn ltrParseCharOrByte(src: string, quote_pos: usize, is_byte: bool): ParsedLiteral;
fn ltrParseNumber(src: string): ParsedLiteral;
fn parseLiteral(src: string): ParsedLiteral;
fn ltrEncodeUtf8(cp: u32, out: []u8, pos: usize): usize?;
fn unescapeString(lit_raw: string, out: []u8): i64;
```

## Minimal Usage

```cp
import { parseLiteral } from "litrs";

pub fn main() {
    let result = parseLiteral("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"litrs"`; install via `cap add litrs`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
