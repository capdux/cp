# hi-base32 — AI Agent Quick Ref

Zero-allocation RFC 4648 Section 6/7 standard Base32 (A-Z2-7) and Base32hex (0-9A-V) encoder and decoder in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    decodeStdChar,
    decodeHexChar,
    encodedLen,
    encodeWithAlphabet,
    decodeWithMode,
    encode,
    decode,
    encodeHex,
} from "hi-base32";
```

## API Signatures

```cp
fn decodeStdChar(b: u8): i32;
fn decodeHexChar(b: u8): i32;
fn encodedLen(byte_len: usize, pad: bool): usize;
fn encodeWithAlphabet(data: &[]u8, pad: bool, alphabet: string, out: []u8): usize;
fn decodeWithMode(input: string, is_hex: bool, out: []u8): i64;
fn encode(data: &[]u8, pad: bool, out: []u8): usize;
fn decode(input: string, out: []u8): i64;
fn encodeHex(data: &[]u8, pad: bool, out: []u8): usize;
fn decodeHex(input: string, out: []u8): i64;
```

## Minimal Usage

```cp
import { decodeWithMode } from "hi-base32";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = decodeWithMode("example", false, out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"hi-base32"`; install via `cap add hi-base32`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
