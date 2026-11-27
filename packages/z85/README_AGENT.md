# z85 — AI Agent Quick Ref

Zero-allocation Base85 multi-standard encoder/decoder supporting ZeroMQ Z85 (RFC 32) and Adobe Ascii85 with 'z' zero-compression and <~...~> framing (Rust z85 / ascii85 parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import { encodeZ85, decodeZ85, encodeAscii85, decodeAscii85 } from "z85";
```

## API Signatures

```cp
fn encodeZ85(input: string, out: []u8): string?;
fn decodeZ85(encoded: string, out: []u8): string?;
fn encodeAscii85(input: string, out: []u8): string?;
fn decodeAscii85(encoded: string, out: []u8): string?;
```

## Minimal Usage

```cp
import { encodeZ85 } from "z85";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeZ85("example", out_buf[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"z85"`; install via `cap add z85`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add z85
cpc check src/main.cp
```
