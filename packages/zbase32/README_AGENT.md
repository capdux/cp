# zbase32 — AI Agent Quick Ref

Zero-allocation Zooko human-oriented z-base-32 encoder, decoder, and validator in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    decodeChar,
    encodedLen,
    encode,
    decode,
    isValidZBase32,
} from "zbase32";
```

## API Signatures

```cp
fn decodeChar(b: u8): i32;
fn encodedLen(byte_len: usize): usize;
fn encode(data: &[]u8, out: []u8): usize;
fn decode(input: string, out: []u8): i64;
fn isValidZBase32(input: string): bool;
```

## Minimal Usage

```cp
import { decode } from "zbase32";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = decode("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"zbase32"`; install via `cap add zbase32`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add zbase32
cpc check src/main.cp
```
