# weezl — AI Agent Quick Ref

Zero-allocation GIF/TIFF/PDF LZW dictionary lossless compressor & decompressor with 9..12-bit variable code widths, Clear/EOI control codes, and KwKwK handling (Rust weezl parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import { hashPair, encode, decode } from "weezl";
```

## API Signatures

```cp
fn hashPair(key: u32): usize;
fn encode(src: &[]u8, dst: []u8): i64;
fn decode(src: &[]u8, dst: []u8): i64;
```

## Minimal Usage

```cp
import { hashPair } from "weezl";

pub fn main() {
    let result = hashPair(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"weezl"`; install via `cap add weezl`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add weezl
cpc check src/main.cp
```
