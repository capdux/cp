# ecdsa — AI Agent Quick Ref

Pure CP zero-allocation ECDSA signature codec (RFC 3279 ASN.1 DER <-> fixed r||s for P-256/P-384, BIP-62 Low-S normalization, constant-time equality via der-asn1 and subtle-ct) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    EcdsaSigResult,
    EcdsaSignature,
    ecsigZero48,
    ecsigIsAllZero,
    ecsigP256Order,
    ecsigP256HalfOrder,
    ecsigCmpBe32,
    ecsigWriteBeInt,
} from "ecdsa";
```

## Core Types & Constants

```cp
class EcdsaSigResult { ok: bool, sig: EcdsaSignature, err: i64 }
class EcdsaSignature { r: [48]u8, s: [48]u8, coord_len: usize } // methods: empty(...) +7
```

## API Signatures

```cp
fn ecsigZero48(): [48]u8;
fn ecsigIsAllZero(data: &[]u8): bool;
fn ecsigP256Order(): [32]u8;
fn ecsigP256HalfOrder(): [32]u8;
fn ecsigCmpBe32(a: &[]u8, b: &[]u8): i32;
fn ecsigWriteBeInt(w: &mut DerWriter, val: &[]u8): bool;
fn ecsigReadBeInt(r: &mut DerReader, coord_len: usize, out: []u8): i64;
```

## Minimal Usage

```cp
import { ecsigZero48 } from "ecdsa";

pub fn main() {
    let result = ecsigZero48();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ecdsa"`; install via `cap add ecdsa`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
