# iconv-lite — AI Agent Quick Ref

Zero-allocation character encoding converter supporting UTF-8, ASCII, Latin1, Windows-1252, UTF-16LE/BE with BOM and surrogate pairs for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Charset,
    IcvUtf8Step,
    icvStrEq,
    parseCharset,
    encodingExists,
    icvWin1252ToUnicode,
    icvUnicodeToWin1252,
    icvWriteUtf8,
} from "iconv-lite";
```

## Core Types & Constants

```cp
enum Charset { Utf8, Ascii, Latin1, Win1252, Utf16Le, Utf16Be, ... }
class IcvUtf8Step { cp: u32, len: usize }
```

## API Signatures

```cp
fn icvStrEq(a: string, b: string): bool;
fn parseCharset(name: string): Charset;
fn encodingExists(name: string): bool;
fn icvWin1252ToUnicode(b: u8): u32;
fn icvUnicodeToWin1252(cp: u32): u8?;
fn icvWriteUtf8(out: []u8, pos: usize, cp: u32): usize;
fn icvReadUtf8(s: string, pos: usize): IcvUtf8Step;
fn icvReadU16(data: &[]u8, pos: usize, is_le: bool): u16;
fn icvWriteU16(out: []u8, pos: usize, val: u16, is_le: bool): usize;
fn decodeToUtf8(data: &[]u8, charset: Charset, strip_bom: bool, out: []u8): usize;
fn encodeFromUtf8(utf8: string, charset: Charset, add_bom: bool, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseCharset } from "iconv-lite";

pub fn main() {
    let result = parseCharset("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"iconv-lite"`; install via `cap add iconv-lite`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
