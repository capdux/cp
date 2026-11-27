# rustls-pki-types@1.0.0 (Agent Quickref)

> Zero-allocation TLS/PKI core types (DnsName, ServerName, UnixTime, PEM object detection & DER decoding via @cap/pem-rfc7468) in pure CP (rustls-pki-types v1 parity). | License: MIT
> Import: `import { ... } from "rustls-pki-types";` | Full types: `src/index.d.cp`
> Deps: `pem-rfc7468`

## Functions
- `export fn pkiToLowerAscii(ch: u8): u8;`
- `export fn pkiIsAsciiDigit(ch: u8): bool;`
- `export fn pkiIsAsciiAlpha(ch: u8): bool;`
- `export fn pkiIsDnsChar(ch: u8): bool;`
- `export fn pkiHexVal(ch: u8): i32;`
- `export fn pkiBytesEqStr(buf: &[]u8, expected: string): bool;`
- `export fn pkiClassifyPemLabel(label: &[]u8): u8;`
- `export fn pkiParseIpv4Slice(s: string, start: usize, end: usize, out: &mut [4]u8): bool;`
- `export fn pkiParseIpv6Slice(input: string, out: &mut [16]u8): bool;`
- `export fn isValidDnsName(input: string): bool;`
- `export fn parseDnsName(input: string): DnsName;`
- `export fn parseServerName(input: string): ServerName;`
- `export fn detectPemKind(pem: string): u8;`
- `export fn decodePemDer(pem: string, der_out: []u8): PkiPemResult;`
- `export fn pemKindLabel(kind: u8): string;`
- `export fn isPrivateKeyKind(kind: u8): bool;`
