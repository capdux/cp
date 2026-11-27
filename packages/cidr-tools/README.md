[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# cidr-tools

Zero-allocation IPv4 CIDR parser, formatter, overlap/containment checker, merge, and exclude engine (npm cidr-tools / ip-cidr parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cidr-tools` ([https://github.com/silverwind/cidr-tools](https://github.com/silverwind/cidr-tools))

| Metadata | Value |
| --- | --- |
| Package | `cidr-tools` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add cidr-tools
```

```cp
import {
    Cidr4,
    Range64,
    invalidCidr,
    pow2_64,
    prefixMask,
    parseIpv4,
    parseCidr4,
    appendU8Dec,
} from "cidr-tools";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Cidr4` | `class` | Fields: `network: u32, prefix: u8, valid: bool` |
| `Range64` | `class` | Fields: `lo: u64, hi: u64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `invalidCidr` | `(): Cidr4` | Invokes `invalidCidr` and returns `Cidr4`. |
| `pow2_64` | `(n: u8): u64` | Invokes `pow2_64` and returns `u64`. |
| `prefixMask` | `(prefix: u8): u32` | Invokes `prefixMask` and returns `u32`. |
| `parseIpv4` | `(s: &[]u8): i64` | Parses input via `parseIpv4` and returns `i64`. |
| `parseCidr4` | `(s: &[]u8): Cidr4` | Parses input via `parseCidr4` and returns `Cidr4`. |
| `appendU8Dec` | `(dst: []u8, pos: usize, v_in: u8): usize` | Invokes `appendU8Dec` and returns `usize`. |
| `formatCidr4` | `(c: Cidr4, dst: []u8): usize` | Serializes/formats data via `formatCidr4` returning `usize`. |
| `cidrHostCount` | `(c: Cidr4): u64` | Invokes `cidrHostCount` and returns `u64`. |
| `cidrStart` | `(c: Cidr4): u32` | Invokes `cidrStart` and returns `u32`. |
| `cidrEnd` | `(c: Cidr4): u32` | Invokes `cidrEnd` and returns `u32`. |
| `containsIp` | `(c: Cidr4, ip: u32): bool` | Invokes `containsIp` and returns `bool`. |
| `containsCidr` | `(outer: Cidr4, inner: Cidr4): bool` | Invokes `containsCidr` and returns `bool`. |
| `overlaps` | `(a: Cidr4, b: Cidr4): bool` | Invokes `overlaps` and returns `bool`. |
| `emitRangeCidrs` | `(lo_in: u64, hi: u64, out: []Cidr4, pos_in: usize): usize` | Invokes `emitRangeCidrs` and returns `usize`. |
| `mergeCidrs` | `(inputs: &[]Cidr4, out: []Cidr4): usize` | Invokes `mergeCidrs` and returns `usize`. |
| `excludeCidr` | `(base: Cidr4, remove: Cidr4, out: []Cidr4): usize` | Invokes `excludeCidr` and returns `usize`. |

## Minimal Example

```cp
import { parseIpv4 } from "cidr-tools";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = parseIpv4(out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
