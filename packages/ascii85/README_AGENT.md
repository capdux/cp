# ascii85 — AI Agent Quick Ref

Zero-allocation Adobe Ascii85 (PostScript / PDF base-85 with 'z' zero compression and <~...~> delimiters) encoder and decoder for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    buildA85DecLut,
    a85IsSpace,
    maxEncodedLen,
    encode,
    decode,
} from "ascii85";
```

## API Signatures

```cp
fn buildA85DecLut(): [256]u8;
fn a85IsSpace(c: u8): bool;
fn maxEncodedLen(byte_len: usize, with_delimiters: bool): usize;
fn encode(data: &[]u8, with_delimiters: bool, out: []u8): usize;
fn decode(input: string, out: []u8): i64;
```

## Minimal Usage

```cp
import { decode } from "ascii85";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = decode("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ascii85"`; install via `cap add ascii85`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add ascii85
cpc check src/main.cp
```
