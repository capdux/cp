# filenamify — AI Agent Quick Ref

Zero-allocation cross-platform safe filename sanitizer (npm filenamify v6 parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    fnmIsInvalidChar,
    fnmHasInvalidChar,
    fnmToLowerAscii,
    fnmEq3IgnoreCase,
    fnmMatchAt,
    fnmEndsWith,
    fnmAppendStr,
    fnmUtf8SafeLen,
} from "filenamify";
```

## API Signatures

```cp
fn fnmIsInvalidChar(ch: u8): bool;
fn fnmHasInvalidChar(s: string): bool;
fn fnmToLowerAscii(ch: u8): u8;
fn fnmEq3IgnoreCase(s: string, a: u8, b: u8, c: u8): bool;
fn fnmMatchAt(buf: string, pos: usize, pat: string): bool;
fn fnmEndsWith(buf: string, len: usize, pat: string): bool;
fn fnmAppendStr(dst: []u8, pos: usize, src: string): usize;
fn fnmUtf8SafeLen(s: string, max_len: usize): usize;
fn isReservedFilename(name: string): bool;
fn filenamifyWithOptions(input: string, replacement: string, max_len: usize, out: []u8): usize;
fn filenamify(input: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { isReservedFilename } from "filenamify";

pub fn main() {
    let result = isReservedFilename("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"filenamify"`; install via `cap add filenamify`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
