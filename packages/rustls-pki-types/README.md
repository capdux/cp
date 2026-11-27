[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# rustls-pki-types

> **Source Attribution**: Clean-room MIT CP port of **crates.io** [`rustls-pki-types`](https://github.com/rustls/pki-types) (`v1` / `MIT`), CP package `rustls-pki-types@1.0.0`.

Zero-allocation TLS/PKI core types in pure CP: RFC 1035/1123 `DnsName`, IPv4/IPv6/DNS `ServerName`, `UnixTime`, and PEM object classification & DER decoding powered by `pem-rfc7468`.

## Why CP `rustls-pki-types`?

- **Zero Heap Allocation**: `DnsName` stores up to 253 bytes inline, `ServerName` stores DNS or IPv4/IPv6 octets inline, and `decodePemDer` writes directly to caller buffers.
- **Zero-Overhead Stack Initialization**: Fixed stack buffers (`[253]u8`, `[4]u8`, `[16]u8`, IPv6 parsing arrays) use `undefined` initialization to eliminate redundant stack zeroing.
- **RFC-Compliant Validation**: Enforces RFC 1035/1123 label rules (max 63 chars/label, max 253 total, no leading/trailing `-`, rejects bare numeric TLDs) and full IPv4/IPv6 (including `::` compression and IPv4-mapped IPv6).
- **Unified PEM/DER Bridge**: Reuses `pem-rfc7468` for `detectPemKind` and `decodePemDer`.

## Zero-Cost Migration (Rust → CP)

Keep the exact same package name (`rustls-pki-types`) — no renaming required.

**Rust (`Cargo.toml` / `main.rs`):**
```rust
use rustls_pki_types::{DnsName, ServerName, UnixTime, CertificateDer};
```

**CP (`cap.json` / `main.cp`):**
```typescript
import {
    DnsName, ServerName, UnixTime, PkiPemResult,
    parseDnsName, isValidDnsName, parseServerName,
    detectPemKind, decodePemDer, pemKindLabel, isPrivateKeyKind
} from "rustls-pki-types";
```

## Quick Start

### Validate TLS SNI (`ServerName` & `DnsName`)

```typescript
import { parseServerName, parseDnsName, UnixTime } from "rustls-pki-types";

export fn checkTlsTarget(host: string, now_secs: u64): bool {
    let sn = parseServerName(host);
    if (!sn.isValid()) {
        return false;
    }
    if (sn.isDns()) {
        let parent = parseDnsName("example.com");
        if (!sn.dns.isSubdomainOf(parent) && !sn.dns.eqIgnoreCase(parent)) {
            return false;
        }
    }
    let now = UnixTime.fromSeconds(now_secs);
    let not_before = UnixTime.fromSeconds(1700000000);
    let not_after = UnixTime.fromSeconds(1900000000);
    return now.isWithin(not_before, not_after);
}
```

### Detect & Decode PEM Objects to DER

```typescript
import { decodePemDer, pemKindLabel } from "rustls-pki-types";

export fn decodeCertOrKey(pem: string): usize {
    let mut der_buf: [4096]u8 = undefined;
    let res = decodePemDer(pem, der_buf[0..4096]);
    if (!res.isOk()) {
        return 0;
    }
    if (res.isCertificate() || res.isPrivateKey()) {
        let label = pemKindLabel(res.kind);
        _ = label;
        return res.der_len;
    }
    return 0;
}
```

## API Reference

- **`DnsName`**: `invalid()`, `parse(input: string)`, `isValid()`, `eqIgnoreCase(other: DnsName)`, `eqIgnoreCaseStr(other: string)`, `labelCount()`, `isSubdomainOf(parent: DnsName)`, `toLowercase(out: []u8): usize`. Helpers: `isValidDnsName(input)`, `parseDnsName(input)`.
- **`ServerName`**: `invalid()`, `fromDns(dns)`, `fromIpv4(octets: [4]u8)`, `fromIpv6(octets: [16]u8)`, `parse(input: string)`, `isValid()`, `isDns()`, `isIpv4()`, `isIpv6()`, `isIp()`. Helper: `parseServerName(input)`.
- **`UnixTime`**: `epoch()`, `fromSeconds(secs: u64)`, `asSeconds()`, `addSeconds(delta)`, `subSeconds(delta)`, `isBefore(other)`, `isAfter(other)`, `equals(other)`, `isWithin(not_before, not_after)`.
- **PEM Helpers**: `detectPemKind(pem: string): u8`, `decodePemDer(pem: string, der_out: []u8): PkiPemResult`, `pemKindLabel(kind: u8): string`, `isPrivateKeyKind(kind: u8): bool`.
- **Constants**:
  - `SERVER_NAME_INVALID (0)`, `SERVER_NAME_DNS (1)`, `SERVER_NAME_IPV4 (2)`, `SERVER_NAME_IPV6 (3)`
  - `PEM_UNKNOWN (0)`, `PEM_CERTIFICATE (1)`, `PEM_PKCS8_KEY (2)`, `PEM_PKCS1_KEY (3)`, `PEM_SEC1_KEY (4)`, `PEM_PUBLIC_KEY (5)`, `PEM_CRL (6)`, `PEM_CSR (7)`
  - `PKI_OK (0)`, `PKI_ERR_INVALID_DNS_NAME (-101)`, `PKI_ERR_INVALID_IP_ADDR (-102)`, `PKI_ERR_INVALID_SERVER_NAME (-103)`, `PKI_ERR_UNKNOWN_PEM_KIND (-104)`

## License

MIT
