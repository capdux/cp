[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# webpki-roots

Zero-allocation Mozilla NSS / WebPKI root CA trust anchor catalog and dynamic trust store reusing x509-cert and spki (webpki-roots parity) in pure CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:webpki-roots` ([https://github.com/rustls/webpki-roots](https://github.com/rustls/webpki-roots))

| Metadata | Value |
| --- | --- |
| Package | `webpki-roots` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add webpki-roots
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `TrustAnchorInfo` | `class` | Fields: `name: string, org: string, country: string, key_alg: u8, spki_fingerprint: u64, not_after_year: u16` · Methods: `empty, init, isEmpty, isValidAtYear, matchesIssuer` |
| `TrustAnchorResult` | `class` | Fields: `found: bool, anchor: TrustAnchorInfo` |
| `TrustAnchorStore` | `class` | Fields: `anchors: [32]TrustAnchorInfo, len: usize` · Methods: `empty, withMozillaDefaults, count, capacity, at, addAnchor, +5 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `wrStrEq` | `(a: string, b: string): bool` | Invokes `wrStrEq` and returns `bool`. |
| `wrBytesEqStr` | `(b: &[]u8, s: string): bool` | Invokes `wrBytesEqStr` and returns `bool`. |
| `wrIsDigit` | `(ch: u8): bool` | Invokes `wrIsDigit` and returns `bool`. |
| `wrParseCertNotAfterYear` | `(cert: X509Certificate): u16` | Invokes `wrParseCertNotAfterYear` and returns `u16`. |
| `wrMakeRoot` | `(name: string, org: string, country: string, key_alg: u8, not_after_year: u16): TrustAnchorInfo` | Invokes `wrMakeRoot` and returns `TrustAnchorInfo`. |
| `mozillaRootAt` | `(idx: usize): TrustAnchorInfo` | Invokes `mozillaRootAt` and returns `TrustAnchorInfo`. |
| `lookupMozillaRootByCn` | `(cn: string): TrustAnchorResult` | Queries/retrieves data via `lookupMozillaRootByCn` returning `TrustAnchorResult`. |
| `certSpkiFingerprint` | `(cert: X509Certificate): u64` | Invokes `certSpkiFingerprint` and returns `u64`. |
| `wrEmptyAnchors` | `(): [32]TrustAnchorInfo` | Invokes `wrEmptyAnchors` and returns `[32]TrustAnchorInfo`. |

## Minimal Example

```cp
import { wrStrEq } from "webpki-roots";

pub fn main() {
    let result = wrStrEq("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
