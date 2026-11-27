[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# rustls-webpki

Pure CP zero-allocation WebPKI X.509 certificate verification engine (RFC 6125 DNS hostname matching, RFC 5280 validity & chain verification; reuses x509-cert)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rustls-webpki` ([https://github.com/rustls/webpki](https://github.com/rustls/webpki))

| Metadata | Value |
| --- | --- |
| Package | `rustls-webpki` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add rustls-webpki
```

```cp
import {
    X509TimeResult,
    wpkiAsciiLower,
    wpkiIsAsciiAlphaNum,
    wpkiIsValidDnsLabelBytes,
    wpkiValidateDnsNameBytes,
    wpkiCountDotsBytes,
    wpkiEqIgnoreCaseBytes,
    wpkiHasStarBytes,
} from "rustls-webpki";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `X509TimeResult` | `class` | Fields: `ok: bool, unix_secs: u64, year: u32, month: u8, day: u8, hour: u8, +2 more` · Methods: `fail` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `wpkiAsciiLower` | `(b: u8): u8` | Invokes `wpkiAsciiLower` and returns `u8`. |
| `wpkiIsAsciiAlphaNum` | `(b: u8): bool` | Invokes `wpkiIsAsciiAlphaNum` and returns `bool`. |
| `wpkiIsValidDnsLabelBytes` | `(label: &[]u8): bool` | Invokes `wpkiIsValidDnsLabelBytes` and returns `bool`. |
| `wpkiValidateDnsNameBytes` | `(name: &[]u8): bool` | Invokes `wpkiValidateDnsNameBytes` and returns `bool`. |
| `wpkiCountDotsBytes` | `(s: &[]u8): usize` | Invokes `wpkiCountDotsBytes` and returns `usize`. |
| `wpkiEqIgnoreCaseBytes` | `(a: &[]u8, b: &[]u8): bool` | Invokes `wpkiEqIgnoreCaseBytes` and returns `bool`. |
| `wpkiHasStarBytes` | `(s: &[]u8): bool` | Invokes `wpkiHasStarBytes` and returns `bool`. |
| `verifyDnsHostnameBytes` | `(pattern: &[]u8, hostname: &[]u8): bool` | Validates input with `verifyDnsHostnameBytes` returning `bool`. |
| `verifyDnsHostname` | `(pattern: string, hostname: string): bool` | Validates input with `verifyDnsHostname` returning `bool`. |
| `wpkiDigit` | `(b: u8): i32` | Invokes `wpkiDigit` and returns `i32`. |
| `wpkiParse2Digits` | `(buf: &[]u8, off: usize): i32` | Invokes `wpkiParse2Digits` and returns `i32`. |
| `wpkiParse4Digits` | `(buf: &[]u8, off: usize): i32` | Invokes `wpkiParse4Digits` and returns `i32`. |
| `wpkiIsLeapYear` | `(year: u32): bool` | Invokes `wpkiIsLeapYear` and returns `bool`. |
| `wpkiDaysInMonth` | `(year: u32, month: u8): u8` | Invokes `wpkiDaysInMonth` and returns `u8`. |
| `wpkiDaysFromCivil` | `(year: u32, month: u8, day: u8): u64` | Invokes `wpkiDaysFromCivil` and returns `u64`. |
| `parseX509TimeUtc` | `(time_bytes: &[]u8): X509TimeResult` | Parses input via `parseX509TimeUtc` and returns `X509TimeResult`. |
| `verifyValidityWindow` | `(validity: X509Validity, now_unix: u64): i32` | Validates input with `verifyValidityWindow` returning `i32`. |
| `verifyValidityAt` | `(cert: X509Certificate, now_unix: u64): i32` | Validates input with `verifyValidityAt` returning `i32`. |
| `verifyCertForDnsName` | `(cert: X509Certificate, hostname: string, now_unix: u64): i32` | Validates input with `verifyCertForDnsName` returning `i32`. |
| `verifyDerCertForDnsName` | `(der: &[]u8, hostname: string, now_unix: u64): i32` | Validates input with `verifyDerCertForDnsName` returning `i32`. |
| `verifyChainLink` | `(leaf: X509Certificate, issuer: X509Certificate): bool` | Validates input with `verifyChainLink` returning `bool`. |
| `verifyTrustAnchor` | `(anchor: X509Certificate, now_unix: u64): i32` | Validates input with `verifyTrustAnchor` returning `i32`. |
| `verifyEndEntityChain` | `(leaf: X509Certificate, issuer: X509Certificate, hostname: string, now_unix: u64): i32` | Validates input with `verifyEndEntityChain` returning `i32`. |

## Minimal Example

```cp
import { verifyDnsHostname } from "rustls-webpki";

pub fn main() {
    let result = verifyDnsHostname("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
