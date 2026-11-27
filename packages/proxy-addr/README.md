[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# proxy-addr

Pure CP zero-allocation reverse-proxy client IP resolver and trusted proxy chain validator (npm proxy-addr parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:proxy-addr` ([https://github.com/jshttp/proxy-addr](https://github.com/jshttp/proxy-addr))

| Metadata | Value |
| --- | --- |
| Package | `proxy-addr` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add proxy-addr
```

```cp
import {
    IpSpan,
    PaIpv4,
    PaIpv6Info,
    paTrimBounds,
    paStripV4Mapped,
    paParseIpv4,
    paParseIpv6Info,
    isLoopback,
} from "proxy-addr";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `IpSpan` | `class` | Fields: `start: usize, len: usize` |
| `PaIpv4` | `class` | Fields: `ok: bool, o0: u8, o1: u8, o2: u8, o3: u8` |
| `PaIpv6Info` | `class` | Fields: `ok: bool, is_loopback: bool, first_word: u16` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `paTrimBounds` | `(s: string): IpSpan` | Invokes `paTrimBounds` and returns `IpSpan`. |
| `paStripV4Mapped` | `(s: string): string` | Invokes `paStripV4Mapped` and returns `string`. |
| `paParseIpv4` | `(s: string): PaIpv4` | Invokes `paParseIpv4` and returns `PaIpv4`. |
| `paParseIpv6Info` | `(s: string): PaIpv6Info` | Invokes `paParseIpv6Info` and returns `PaIpv6Info`. |
| `isLoopback` | `(ip: string): bool` | Checks predicate `isLoopback` and returns `bool`. |
| `isLinkLocal` | `(ip: string): bool` | Checks predicate `isLinkLocal` and returns `bool`. |
| `isUniqueLocal` | `(ip: string): bool` | Checks predicate `isUniqueLocal` and returns `bool`. |
| `isTrustedBuiltin` | `(ip: string, preset: u8): bool` | Checks predicate `isTrustedBuiltin` and returns `bool`. |
| `parseXffAddresses` | `(remote_addr: string, xff_header: string, out: []IpSpan): usize` | Parses input via `parseXffAddresses` and returns `usize`. |
| `paCopyToOut` | `(src: string, out: []u8): usize` | Invokes `paCopyToOut` and returns `usize`. |
| `resolveClientIp` | `(remote_addr: string, xff_header: string, trust_preset: u8, max_hops: usize, out: []u8): usize` | Invokes `resolveClientIp` and returns `usize`. |

## Minimal Example

```cp
import { isLoopback } from "proxy-addr";

pub fn main() {
    let result = isLoopback("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
