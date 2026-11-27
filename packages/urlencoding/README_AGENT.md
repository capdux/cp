# urlencoding — AI Agent Quick Ref

Zero-heap RFC 3986 percent-encoding and decoding in pure CP (modeled after Rust urlencoding v2) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    urlencHexUpper,
    urlencHexVal,
    isUnreserved,
    encodedLen,
    encodeBinary,
    encode,
    decode,
    decodeLossy,
} from "urlencoding";
```

## API Signatures

```cp
fn urlencHexUpper(nibble: u8): u8;
fn urlencHexVal(ch: u8): i16;
fn isUnreserved(b: u8): bool;
fn encodedLen(input: string): usize;
fn encodeBinary(data: &[]u8, out: []u8): usize;
fn encode(input: string, out: []u8): usize;
fn decode(input: string, out: []u8): i64;
fn decodeLossy(input: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { encodedLen } from "urlencoding";

pub fn main() {
    let result = encodedLen("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"urlencoding"`; install via `cap add urlencoding`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
