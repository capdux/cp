# rfc6979 — AI Agent Quick Ref

Pure CP zero-allocation RFC 6979 deterministic ECDSA nonce generator (HMAC-SHA256 DRBG, RustCrypto rfc6979 parity) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    HmacDrbg256,
    rfcCopy32,
    rfcCmpBe32,
    rfcIsNonZero32,
    rfcOrderValid32,
    rfcSubBe32,
    generateK,
} from "rfc6979";
```

## Core Types & Constants

```cp
class HmacDrbg256 { k: [32]u8, v: [32]u8 } // methods: init(...) +1
```

## API Signatures

```cp
fn rfcCopy32(dst: []u8, src: []u8);
fn rfcCmpBe32(a: []u8, b: []u8): i32;
fn rfcIsNonZero32(a: []u8): bool;
fn rfcOrderValid32(q: []u8): bool;
fn rfcSubBe32(a: []u8, b: []u8, out: []u8);
fn generateK(priv_key_32: []u8, msg_hash_32: []u8, order_32: []u8, out_k32: []u8): bool;
```

## Minimal Usage

```cp
import { rfcCmpBe32 } from "rfc6979";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = rfcCmpBe32(out_buf[0..256], buf_1[0..256]);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"rfc6979"`; install via `cap add rfc6979`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
