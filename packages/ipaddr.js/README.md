[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# `ipaddr.js`

Zero-allocation IPv4 & IPv6 parser, RFC 5952 canonical formatter, CIDR matcher, IPv4-mapped IPv6 converter, and IANA address range classifier for CP — a clean-room MIT counterpart to npm [`ipaddr.js`](https://github.com/whitequark/ipaddr.js) (v2 parity).

## Installation & Import

```bash
cap add ipaddr.js
```

```cp
import {
    Ipv4Addr,
    Ipv6Addr,
    CidrV4,
    CidrV6,
    parseIpv4,
    parseIpv6,
    parseCidrV4,
    parseCidrV6,
    isValidIp,
    isValidIpv4,
    isValidIpv6,
    ipKind,
} from "ipaddr.js";
```

## Core API

### Free Functions

| Function | Signature | Description |
| :--- | :--- | :--- |
| `parseIpv4` | `(s: string): Ipv4Addr` | Parse dotted-decimal IPv4; check `.ok` before use |
| `parseIpv6` | `(s: string): Ipv6Addr` | Parse compressed, full, or IPv4-embedded IPv6; check `.ok` |
| `parseCidrV4` | `(s: string): CidrV4` | Parse IPv4 CIDR (e.g., `"10.0.0.0/8"`); check `.ok` |
| `parseCidrV6` | `(s: string): CidrV6` | Parse IPv6 CIDR (e.g., `"2001:db8::/32"`); check `.ok` |
| `isValidIp` / `isValidIpv4` / `isValidIpv6` | `(s: string): bool` | Fast validity predicates |
| `ipKind` | `(s: string): string` | Returns `"ipv4"`, `"ipv6"`, or `"invalid"` |

### `Ipv4Addr` (`octets: [4]u8`, `ok: bool`)

| Method | Signature | Description |
| :--- | :--- | :--- |
| `Ipv4Addr.fromU32` | `(val: u32): Ipv4Addr` | Construct from host-order 32-bit integer |
| `Ipv4Addr.subnetMaskFromPrefixLength` | `(prefix: u8): Ipv4Addr` | Build subnet mask from prefix `0..=32` |
| `kind` / `range` | `(): string` | Returns `"ipv4"` / range label (`"private"`, `"loopback"`, `"unicast"`, etc.) |
| `toU32` / `toByteArray` | `(): u32` / `(): [4]u8` | Convert to big-endian `u32` or 4-byte array |
| `equals` | `(other: Ipv4Addr): bool` | Exact value equality |
| `toStr` | `(out: []u8): usize` | Format dotted-decimal into `out`; returns bytes written |
| `matchCidr` | `(net: Ipv4Addr, prefix: u8): bool` | Test if address falls within `net/prefix` |
| `prefixLengthFromSubnetMask` | `(): i8` | Return CIDR prefix length (`0..=32`) or `-1` if non-contiguous |
| `toIpv4MappedIpv6` | `(): Ipv6Addr` | Convert to `::ffff:a.b.c.d` IPv6 address |

### `Ipv6Addr` (`parts: [8]u16`, `ok: bool`) & CIDR Types

| Method | Signature | Description |
| :--- | :--- | :--- |
| `Ipv6Addr.kind` / `range` | `(): string` | Returns `"ipv6"` / range label (`"loopback"`, `"uniqueLocal"`, `"linkLocal"`, etc.) |
| `Ipv6Addr.toByteArray` / `equals` | `(): [16]u8` / `(other: Ipv6Addr): bool` | Convert to 16-byte network-order array / compare equality |
| `Ipv6Addr.toStr` / `toNormalizedStr` | `(out: []u8): usize` | Format RFC 5952 shortest form (`::`) or full expanded 8-group hex |
| `Ipv6Addr.matchCidr` | `(net: Ipv6Addr, prefix: u8): bool` | Test if address falls within `net/prefix` |
| `Ipv6Addr.isIpv4Mapped` / `toIpv4` | `(): bool` / `(): Ipv4Addr` | Detect `::ffff:0:0/96` and extract embedded IPv4 |
| `CidrV4.matches` / `CidrV6.matches` | `(ip: Ipv4Addr / Ipv6Addr): bool` | Test membership against parsed CIDR block |

## Examples

### 1. Parsing, Range Classification & CIDR Matching

```cp
import { print } from "cap:io";
import { parseIpv4, parseCidrV4 } from "ipaddr.js";

fn main() {
    const ip = parseIpv4("192.168.1.42");
    const cidr = parseCidrV4("192.168.0.0/16");
    if (ip.ok && cidr.ok) {
        print("range:", ip.range());          // "private"
        print("matches:", cidr.matches(ip));  // true
        print("u32:", ip.toU32());            // 3232235818
    }
}
```

### 2. RFC 5952 Formatting & IPv4-Mapped IPv6 Conversion

```cp
import { print } from "cap:io";
import { parseIpv6, parseIpv4 } from "ipaddr.js";

fn main() {
    const v6 = parseIpv6("2001:0db8:0000:0000:0000:0000:0000:0001");
    if (v6.ok) {
        let short_buf: [64]u8 = undefined;
        const n = v6.toStr(short_buf[0..64]);
        print("RFC 5952:", short_buf[0..n]); // "2001:db8::1"
    }

    const mapped = parseIpv4("127.0.0.1").toIpv4MappedIpv6();
    if (mapped.isIpv4Mapped()) {
        const back = mapped.toIpv4();
        print("loopback:", back.range()); // "loopback"
    }
}
```

## Differences from Upstream & Performance Highlights

- **Zero Heap Allocation**: `Ipv4Addr` (5 bytes) and `Ipv6Addr` (17 bytes) are pure stack value types; string formatting writes directly into caller-provided `[]u8` buffers.
- **No Exceptions**: Parsing never throws — invalid inputs return a value with `.ok == false` instead of throwing JS `Error` objects.
- **RFC 5952 Compliant**: `Ipv6Addr.toStr` compresses the longest run of two or more zero hextets to `::` in lowercase, while `toNormalizedStr` emits all 8 groups explicitly.
