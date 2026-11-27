# rustls-webpki — AI Agent Quick Ref

Pure CP zero-allocation WebPKI X.509 certificate verification engine (RFC 6125 DNS hostname matching, RFC 5280 validity & chain verification; reuses x509-cert) (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class X509TimeResult { ok: bool, unix_secs: u64, year: u32, month: u8, /* +4 fields */ } // methods: fail(...)
```

## API Signatures

```cp
fn wpkiAsciiLower(b: u8): u8;
fn wpkiIsAsciiAlphaNum(b: u8): bool;
fn wpkiIsValidDnsLabelBytes(label: &[]u8): bool;
fn wpkiValidateDnsNameBytes(name: &[]u8): bool;
fn wpkiCountDotsBytes(s: &[]u8): usize;
fn wpkiEqIgnoreCaseBytes(a: &[]u8, b: &[]u8): bool;
fn wpkiHasStarBytes(s: &[]u8): bool;
fn verifyDnsHostnameBytes(pattern: &[]u8, hostname: &[]u8): bool;
fn verifyDnsHostname(pattern: string, hostname: string): bool;
fn wpkiDigit(b: u8): i32;
fn wpkiParse2Digits(buf: &[]u8, off: usize): i32;
fn wpkiParse4Digits(buf: &[]u8, off: usize): i32;
fn wpkiIsLeapYear(year: u32): bool;
fn wpkiDaysInMonth(year: u32, month: u8): u8;
fn wpkiDaysFromCivil(year: u32, month: u8, day: u8): u64;
fn parseX509TimeUtc(time_bytes: &[]u8): X509TimeResult;
fn verifyValidityWindow(validity: X509Validity, now_unix: u64): i32;
fn verifyValidityAt(cert: X509Certificate, now_unix: u64): i32;
fn verifyCertForDnsName(cert: X509Certificate, hostname: string, now_unix: u64): i32;
fn verifyDerCertForDnsName(der: &[]u8, hostname: string, now_unix: u64): i32;
fn verifyChainLink(leaf: X509Certificate, issuer: X509Certificate): bool;
fn verifyTrustAnchor(anchor: X509Certificate, now_unix: u64): i32;
fn verifyEndEntityChain(leaf: X509Certificate, issuer: X509Certificate, hostname: string, now_unix: u64): i32;
```

## Minimal Usage

```cp
import { verifyDnsHostname } from "rustls-webpki";

pub fn main() {
    let result = verifyDnsHostname("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"rustls-webpki"`; install via `cap add rustls-webpki`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
