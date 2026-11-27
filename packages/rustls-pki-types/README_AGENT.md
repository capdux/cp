# rustls-pki-types (AI Agent Quick Ref)

- **Import**: `import { DnsName, ServerName, UnixTime, PkiPemResult, isValidDnsName, parseDnsName, parseServerName, detectPemKind, decodePemDer, pemKindLabel, isPrivateKeyKind, SERVER_NAME_DNS, SERVER_NAME_IPV4, SERVER_NAME_IPV6, PEM_CERTIFICATE, PEM_PKCS8_KEY, PEM_PKCS1_KEY, PEM_SEC1_KEY } from "rustls-pki-types";`
- **Source**: `crates.io:rustls-pki-types` v1 (MIT, zero-alloc, wraps `pem-rfc7468`, `undefined` stack buffers).

## Core Signatures

```cp
export class DnsName {
    buf: [253]u8, len: usize, valid: bool,
    fn invalid(): DnsName;
    fn parse(input: string): DnsName;
    fn isValid(): bool;
    fn eqIgnoreCase(other: DnsName): bool;
    fn eqIgnoreCaseStr(other: string): bool;
    fn labelCount(): usize;
    fn isSubdomainOf(parent: DnsName): bool;
    fn toLowercase(out: []u8): usize;
}
export fn isValidDnsName(input: string): bool;
export fn parseDnsName(input: string): DnsName;

export class ServerName {
    kind: u8, dns: DnsName, ipv4: [4]u8, ipv6: [16]u8,
    fn invalid(): ServerName;
    fn fromDns(dns: DnsName): ServerName;
    fn fromIpv4(octets: [4]u8): ServerName;
    fn fromIpv6(octets: [16]u8): ServerName;
    fn parse(input: string): ServerName;
    fn isValid(): bool;
    fn isDns(): bool;
    fn isIpv4(): bool;
    fn isIpv6(): bool;
    fn isIp(): bool;
}
export fn parseServerName(input: string): ServerName;

export class UnixTime {
    seconds: u64,
    fn epoch(): UnixTime;
    fn fromSeconds(secs: u64): UnixTime;
    fn asSeconds(): u64;
    fn addSeconds(delta: u64): UnixTime;
    fn subSeconds(delta: u64): UnixTime;
    fn isBefore(other: UnixTime): bool;
    fn isAfter(other: UnixTime): bool;
    fn equals(other: UnixTime): bool;
    fn isWithin(not_before: UnixTime, not_after: UnixTime): bool;
}

export class PkiPemResult {
    kind: u8, der_len: usize, err: i64,
    fn isOk(): bool;
    fn isCertificate(): bool;
    fn isPrivateKey(): bool;
}
export fn detectPemKind(pem: string): u8;
export fn decodePemDer(pem: string, der_out: []u8): PkiPemResult;
export fn pemKindLabel(kind: u8): string;
export fn isPrivateKeyKind(kind: u8): bool;
```

## Constants

- `SERVER_NAME_INVALID=0`, `SERVER_NAME_DNS=1`, `SERVER_NAME_IPV4=2`, `SERVER_NAME_IPV6=3`
- `PEM_UNKNOWN=0`, `PEM_CERTIFICATE=1`, `PEM_PKCS8_KEY=2`, `PEM_PKCS1_KEY=3`, `PEM_SEC1_KEY=4`, `PEM_PUBLIC_KEY=5`, `PEM_CRL=6`, `PEM_CSR=7`
- `PKI_OK=0`, `PKI_ERR_INVALID_DNS_NAME=-101`, `PKI_ERR_INVALID_IP_ADDR=-102`, `PKI_ERR_INVALID_SERVER_NAME=-103`, `PKI_ERR_UNKNOWN_PEM_KIND=-104`

## Pitfalls

1. **Always check `.isValid()`**: `parseDnsName` and `parseServerName` return structs with `valid: false` / `kind: SERVER_NAME_INVALID` instead of throwing.
2. **Bracketed IPv6**: `ServerName.parse` accepts raw IPv6 (`::1`, `2001:db8::1`); strip `[...]` URL brackets before calling if parsing from a URI.
3. **`decodePemDer` single-block**: Decodes only the first PEM block; for multi-cert bundles use `rustls-pemfile`.
4. **Inline `DnsName.buf` slice**: Read bytes via `dns.buf[0..dns.len]` (not the full 253-byte array, since unused tail bytes are `undefined`).
