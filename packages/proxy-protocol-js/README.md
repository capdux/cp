[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# proxy-protocol-js

Zero-heap HAProxy PROXY Protocol v1 (ASCII) and v2 (binary) parser and encoder for L4 client IP preservation.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:proxy-protocol-js` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:ppp` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `proxy-protocol-js` → CP 包名 `proxy-protocol-js`）

| Metadata | Value |
| --- | --- |
| Package | `proxy-protocol-js` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add proxy-protocol-js
```

```cp
import {
    ProxyV1Header,
    ProxyV2Header,
    isV1Prefix,
    isV2Signature,
    parsePortStrict,
    parseV1,
    appendSlice,
    appendU16,
} from "proxy-protocol-js";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ProxyV1Header` | `class` | Fields: `proto: u8, src_ip: string, dst_ip: string, src_port: u16, dst_port: u16, consumed: usize` |
| `ProxyV2Header` | `class` | Fields: `is_proxy: bool, family: u8, transport: u8, payload_len: u16, src_ipv4: [4]u8, dst_ipv4: [4]u8, +3 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isV1Prefix` | `(data: &[]u8): bool` | Checks predicate `isV1Prefix` and returns `bool`. |
| `isV2Signature` | `(data: &[]u8): bool` | Checks predicate `isV2Signature` and returns `bool`. |
| `parsePortStrict` | `(s: string): i32` | Parses input via `parsePortStrict` and returns `i32`. |
| `parseV1` | `(data: string, out: &mut ProxyV1Header): i32` | Parses input via `parseV1` and returns `i32`. |
| `appendSlice` | `(out: []u8, pos: usize, s: string): i64` | Invokes `appendSlice` and returns `i64`. |
| `appendU16` | `(out: []u8, pos: usize, v: u16): i64` | Invokes `appendU16` and returns `i64`. |
| `encodeV1` | `(proto: u8, src_ip: string, dst_ip: string, src_port: u16, dst_port: u16, out: []u8): usize` | Serializes/formats data via `encodeV1` returning `usize`. |
| `parseV2` | `(data: &[]u8, out: &mut ProxyV2Header): i32` | Parses input via `parseV2` and returns `i32`. |
| `encodeV2Ipv4` | `(src_ip: [4]u8, dst_ip: [4]u8, src_port: u16, dst_port: u16, out: []u8): usize` | Serializes/formats data via `encodeV2Ipv4` returning `usize`. |

## Minimal Example

```cp
import { parsePortStrict } from "proxy-protocol-js";

pub fn main() {
    let result = parsePortStrict("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
