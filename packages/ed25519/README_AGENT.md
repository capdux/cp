# ed25519 — AI Agent Quick Ref

Zero-heap RFC 8032 Ed25519 signature & key containers, canonical scalar S < L validation, RFC 8410 SPKI/PKCS#8 DER and hex codecs in pure CP (`v1.0.0`, `Apache-2.0 OR MIT`)

## Import

```cp
import {
    Ed25519Signature,
    Ed25519PublicKey,
    Ed25519SecretKey,
    ed25Zero32,
    ed25Zero64,
    ed25GroupOrderL,
    ed25SpkiPrefix,
    ed25Pkcs8Prefix,
} from "ed25519";
```

## Core Types & Constants

```cp
class Ed25519Signature { bytes: [64]u8 } // methods: fromBytes(...) +9
class Ed25519PublicKey { bytes: [32]u8 } // methods: fromBytes(...) +7
class Ed25519SecretKey { bytes: [32]u8 } // methods: fromBytes(...) +7
```

## API Signatures

```cp
fn ed25Zero32(): [32]u8;
fn ed25Zero64(): [64]u8;
fn ed25GroupOrderL(): [32]u8;
fn ed25SpkiPrefix(): [12]u8;
fn ed25Pkcs8Prefix(): [16]u8;
fn ed25HexNibble(ch: u8): i32;
fn ed25HexDigit(n: u8): u8;
fn ed25EncodeHex(src: &[]u8, out: []u8): usize;
```

## Minimal Usage

```cp
import { ed25Zero32 } from "ed25519";

pub fn main() {
    let result = ed25Zero32();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ed25519"`; install via `cap add ed25519`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
