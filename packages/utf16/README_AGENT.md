# utf16 — AI Agent Quick Ref

Zero-heap UTF-8 <-> UTF-16 / UTF-16LE codec with full surrogate pair support, lone-surrogate rejection, BOM detection, and UTF-16 unit counting. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    detectBom,
    decodeUtf8Next,
    encodeUtf8Scalar,
    countUtf16Units,
    utf8ToUtf16,
    utf16ToUtf8,
    utf8ToUtf16LeBytes,
    utf16LeBytesToUtf8,
} from "utf16";
```

## API Signatures

```cp
fn detectBom(data: &[]u8): u8;
fn decodeUtf8Next(s: string, pos: usize, out_cp: &mut u32): i64;
fn encodeUtf8Scalar(cp: u32, out: []u8, pos: usize): i64;
fn countUtf16Units(utf8: string): i64;
fn utf8ToUtf16(utf8: string, out: []u16): i64;
fn utf16ToUtf8(utf16: &[]u16, out: []u8): i64;
fn utf8ToUtf16LeBytes(utf8: string, out: []u8): i64;
fn utf16LeBytesToUtf8(bytes: &[]u8, out: []u8): i64;
```

## Minimal Usage

```cp
import { decodeUtf8Next } from "utf16";

pub fn main() {
    let result = decodeUtf8Next("example", 0, /* out_cp: &mut u32 */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"utf16"`; install via `cap add utf16`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
