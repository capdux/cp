# `ipaddr.js` — AI Agent Quick Ref

Zero-allocation IPv4/IPv6 parser, RFC 5952 formatter, CIDR matcher & range classifier (npm `ipaddr.js` v2 parity).

## Import

```cp
import {
    Ipv4Addr, Ipv6Addr, CidrV4, CidrV6,
    parseIpv4, parseIpv6, parseCidrV4, parseCidrV6,
    isValidIp, isValidIpv4, isValidIpv6, ipKind,
} from "ipaddr.js";
```

## Core API Signatures

```cp
// Free functions
fn parseIpv4(s: string): Ipv4Addr;
fn parseIpv6(s: string): Ipv6Addr;
fn parseCidrV4(s: string): CidrV4;
fn parseCidrV6(s: string): CidrV6;
fn isValidIp(s: string): bool;
fn isValidIpv4(s: string): bool;
fn isValidIpv6(s: string): bool;
fn ipKind(s: string): string;                  // "ipv4" | "ipv6" | "invalid"

// Ipv4Addr { octets: [4]u8, ok: bool }
Ipv4Addr.fromU32(val: u32): Ipv4Addr;
Ipv4Addr.subnetMaskFromPrefixLength(prefix: u8): Ipv4Addr;
v4.kind(): string;  v4.range(): string;  v4.toU32(): u32;  v4.toByteArray(): [4]u8;
v4.equals(other: Ipv4Addr): bool;
v4.toStr(out: []u8): usize;                    // min 15 bytes
v4.matchCidr(net: Ipv4Addr, prefix: u8): bool;
v4.prefixLengthFromSubnetMask(): i8;           // -1 if invalid mask
v4.toIpv4MappedIpv6(): Ipv6Addr;

// Ipv6Addr { parts: [8]u16, ok: bool }
v6.kind(): string;  v6.range(): string;  v6.toByteArray(): [16]u8;
v6.equals(other: Ipv6Addr): bool;
v6.toStr(out: []u8): usize;                    // RFC 5952 shortest; min 45 bytes
v6.toNormalizedStr(out: []u8): usize;          // full 8-group hex
v6.matchCidr(net: Ipv6Addr, prefix: u8): bool;
v6.isIpv4Mapped(): bool;  v6.toIpv4(): Ipv4Addr;

// CidrV4 / CidrV6 { addr, prefix: u8, ok: bool }
cidr4.matches(ip: Ipv4Addr): bool;
cidr6.matches(ip: Ipv6Addr): bool;
```

## Minimal Example

```cp
import { parseIpv4, parseCidrV4 } from "ipaddr.js";

const ip = parseIpv4("10.1.2.3");
const net = parseCidrV4("10.0.0.0/8");
if (ip.ok && net.ok && net.matches(ip)) {
    let buf: [32]u8 = undefined;
    const n = ip.toStr(buf[0..32]); // buf[0..n] == "10.1.2.3"
}
```

## Gotchas & Borrow Rules

- All methods on `Ipv4Addr`/`Ipv6Addr`/`CidrV4`/`CidrV6` are read-only; values can be `const`.
- Always check `.ok` on parsed results (`parseIpv4`, `parseIpv6`, `parseCidrV4`, `parseCidrV6`, `toIpv4`).
- `toStr` / `toNormalizedStr` write into caller-supplied mutable `[]u8` and return bytes written (`[64]u8` is safe for both IPv4 and IPv6).
- `prefixLengthFromSubnetMask()` returns `-1` for non-contiguous masks.
