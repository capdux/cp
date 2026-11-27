# sha2 — AI Agent Quick Ref

Zero-heap FIPS 180-4 SHA-256 & SHA-224 streaming hashers with hex output (crates.io sha2 equivalent, reuses cookie-signature) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Sha256,
    Sha224,
    sh2HexDigit,
    sh2EncodeHex,
    sha256Hex,
    sha224Hex,
} from "sha2";
```

## Core Types & Constants

```cp
class Sha256 { st: Sha256State } // methods: new(...) +5
class Sha224 { st: Sha256State } // methods: new(...) +4
```

## API Signatures

```cp
fn sh2HexDigit(n: u8): u8;
fn sh2EncodeHex(data: []u8, out: []u8): usize;
fn sha256Hex(s: string, out64: []u8): usize;
fn sha224Hex(s: string, out56: []u8): usize;
```

## Minimal Usage

```cp
import { sha256Hex } from "sha2";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = sha256Hex("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"sha2"`; install via `cap add sha2`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
