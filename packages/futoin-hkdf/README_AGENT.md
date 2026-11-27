# futoin-hkdf — AI Agent Quick Ref

Pure CP zero-allocation RFC 5869 HKDF-SHA256 key derivation function (RustCrypto hkdf parity) (`v1.0.0`, `MIT`)

## Import

```cp
import { hkdfStreamingHmac, hkdfExtract, hkdfExpand, hkdfDerive } from "futoin-hkdf";
```

## API Signatures

```cp
fn hkdfStreamingHmac(prk32: []u8, prev_t: []u8, info: []u8, counter: u8, out32: []u8);
fn hkdfExtract(salt: []u8, ikm: []u8, out_prk32: []u8): bool;
fn hkdfExpand(prk32: []u8, info: []u8, out_okm: []u8): bool;
fn hkdfDerive(salt: []u8, ikm: []u8, info: []u8, out_okm: []u8): bool;
```

## Minimal Usage

```cp
import { hkdfExtract } from "futoin-hkdf";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let mut buf_2: [256]u8 = undefined;
    let result = hkdfExtract(out_buf[0..256], buf_1[0..256], buf_2[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"futoin-hkdf"`; install via `cap add futoin-hkdf`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add futoin-hkdf
cpc check src/main.cp
```
