[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `rustls-pki-types`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rustls-pki-types` ([https://github.com/rustls/pki-types](https://github.com/rustls/pki-types))

Zero-allocation TLS/PKI core types in pure CP (`rustls-pki-types` v1 parity). Reuses `@cap/pem-rfc7468` for RFC 7468 PEM label detection and DER decoding.

## API

### `DnsName` (RFC 1035 / RFC 5890)
- `DnsName.parse(input: string): DnsName` / `parseDnsName(input: string): DnsName` — Validates total length `1..253`, label length `1..63`, no leading/trailing `-`, rejects all-numeric TLDs and illegal characters, normalizes a single trailing `.`
- `isValidDnsName(input: string): bool`
- `dns.isValid(): bool`
- `dns.eqIgnoreCase(other: DnsName): bool` / `dns.eqIgnoreCaseStr(other: string): bool`
- `dns.labelCount(): usize`
- `dns.isSubdomainOf(parent: DnsName): bool`
- `dns.toLowercase(out: []u8): usize`

### `ServerName`
- Constants: `SERVER_NAME_INVALID` (`0`), `SERVER_NAME_DNS` (`1`), `SERVER_NAME_IPV4` (`2`), `SERVER_NAME_IPV6` (`3`)
- `ServerName.parse(input: string): ServerName` / `parseServerName(input: string): ServerName` — Automatically distinguishes IPv4 (`"127.0.0.1"`), IPv6 (`"::1"`, `"[::1]"`, `"::ffff:192.0.2.128"`), and DNS names (`"example.com"`)
- `ServerName.fromDns(dns: DnsName)` / `ServerName.fromIpv4(octets: [4]u8)` / `ServerName.fromIpv6(octets: [16]u8)`
- Predicates: `isValid()`, `isDns()`, `isIp()`, `isIpv4()`, `isIpv6()`

### `UnixTime`
- `UnixTime.epoch()` / `UnixTime.fromSeconds(secs: u64)`
- `asSeconds(): u64`, `addSeconds(delta: u64): UnixTime` (saturating), `subSeconds(delta: u64): UnixTime` (saturating)
- `isBefore(other: UnixTime): bool`, `isAfter(other: UnixTime): bool`, `equals(other: UnixTime): bool`, `isWithin(not_before: UnixTime, not_after: UnixTime): bool`

### PEM Object Detection & DER Decoding (via `@cap/pem-rfc7468`)
- Constants: `PEM_UNKNOWN` (`0`), `PEM_CERTIFICATE` (`1`), `PEM_PKCS8_KEY` (`2`), `PEM_PKCS1_KEY` (`3`), `PEM_SEC1_KEY` (`4`), `PEM_PUBLIC_KEY` (`5`), `PEM_CRL` (`6`), `PEM_CSR` (`7`)
- `detectPemKind(pem: string): u8` — Calls `@cap/pem-rfc7468::decodeLabel`
- `decodePemDer(pem: string, der_out: []u8): PkiPemResult` — Calls `@cap/pem-rfc7468::decodePem`
- `pemKindLabel(kind: u8): string`, `isPrivateKeyKind(kind: u8): bool`
