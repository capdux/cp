# webpki-roots — AI Agent Quick Ref

Zero-allocation Mozilla NSS / WebPKI root CA trust anchor catalog and dynamic trust store reusing x509-cert and spki (webpki-roots parity) in pure CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    TrustAnchorInfo,
    TrustAnchorResult,
    TrustAnchorStore,
    wrStrEq,
    wrBytesEqStr,
    wrIsDigit,
    wrParseCertNotAfterYear,
    wrMakeRoot,
} from "webpki-roots";
```

## Core Types & Constants

```cp
class TrustAnchorInfo { name: string, org: string, country: string, key_alg: u8, /* +2 fields */ } // methods: empty(...) +4
class TrustAnchorResult { found: bool, anchor: TrustAnchorInfo }
class TrustAnchorStore { anchors: [32]TrustAnchorInfo, len: usize } // methods: empty(...) +10
```

## API Signatures

```cp
fn wrStrEq(a: string, b: string): bool;
fn wrBytesEqStr(b: &[]u8, s: string): bool;
fn wrIsDigit(ch: u8): bool;
fn wrParseCertNotAfterYear(cert: X509Certificate): u16;
fn wrMakeRoot(name: string, org: string, country: string, key_alg: u8, not_after_year: u16): TrustAnchorInfo;
fn mozillaRootAt(idx: usize): TrustAnchorInfo;
fn lookupMozillaRootByCn(cn: string): TrustAnchorResult;
fn certSpkiFingerprint(cert: X509Certificate): u64;
fn wrEmptyAnchors(): [32]TrustAnchorInfo;
```

## Minimal Usage

```cp
import { wrStrEq } from "webpki-roots";

pub fn main() {
    let result = wrStrEq("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"webpki-roots"`; install via `cap add webpki-roots`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
