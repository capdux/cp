[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ipnet

Zero-heap pure-CP IPv4/CIDR subnet calculation engine (parse, format, masks, containment, supernet, host count — crates.io ipnet equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ipnet` ([https://github.com/krisprice/ipnet](https://github.com/krisprice/ipnet))

| Metadata | Value |
| --- | --- |
| Package | `ipnet` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ipnet
```

```cp
import {
    Ipv4Net,
    ipnParseIpv4Range,
    ipnParsePrefixRange,
    ipnWriteU8Dec,
    ipnNetmask,
    parseIpv4,
    formatIpv4,
} from "ipnet";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Ipv4Net` | `class` | Fields: `addr: u32, prefix_len: u8` · Methods: `new, parse, netmask, hostmask, network, broadcast, +6 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ipnParseIpv4Range` | `(s: string, start: usize, end: usize): u32?` | Invokes `ipnParseIpv4Range` and returns `u32?`. |
| `ipnParsePrefixRange` | `(s: string, start: usize, end: usize): u8?` | Invokes `ipnParsePrefixRange` and returns `u8?`. |
| `ipnWriteU8Dec` | `(v: u8, out: []u8, pos: usize): usize` | Invokes `ipnWriteU8Dec` and returns `usize`. |
| `ipnNetmask` | `(prefix_len: u8): u32` | Invokes `ipnNetmask` and returns `u32`. |
| `parseIpv4` | `(s: string): u32?` | Parses input via `parseIpv4` and returns `u32?`. |
| `formatIpv4` | `(ip: u32, out: []u8): usize` | Serializes/formats data via `formatIpv4` returning `usize`. |

## Minimal Example

```cp
import { parseIpv4 } from "ipnet";

pub fn main() {
    let result = parseIpv4("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
