[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# h2

Zero-allocation RFC 7541 HTTP/2 HPACK static table (61 entries), N-bit prefixed integer codec, and indexed header encoder for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:h2` ([https://github.com/hyperium/h2](https://github.com/hyperium/h2))（原库名 `h2` → CP 包名 `h2`）

| Metadata | Value |
| --- | --- |
| Package | `h2` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add h2
```

```cp
import {
    StaticEntry,
    IntDecodeResult,
    hpackEqBytes,
    hpackEntry,
    staticTableGet,
    staticTableFind,
    staticTableFindName,
    encodePrefixedInt,
} from "h2";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `StaticEntry` | `class` | Fields: `name: string, value: string, valid: bool` |
| `IntDecodeResult` | `class` | Fields: `value: u64, consumed: usize, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `hpackEqBytes` | `(a: &[]u8, b: &[]u8): bool` | Invokes `hpackEqBytes` and returns `bool`. |
| `hpackEntry` | `(n: string, v: string): StaticEntry` | Invokes `hpackEntry` and returns `StaticEntry`. |
| `staticTableGet` | `(index: usize): StaticEntry` | Invokes `staticTableGet` and returns `StaticEntry`. |
| `staticTableFind` | `(name: string, value: string): usize` | Invokes `staticTableFind` and returns `usize`. |
| `staticTableFindName` | `(name: string): usize` | Invokes `staticTableFindName` and returns `usize`. |
| `encodePrefixedInt` | `(val: u64, prefix_bits: u8, first_byte_mask: u8, out: []u8): usize` | Serializes/formats data via `encodePrefixedInt` returning `usize`. |
| `decodePrefixedInt` | `(data: &[]u8, prefix_bits: u8): IntDecodeResult` | Parses input via `decodePrefixedInt` and returns `IntDecodeResult`. |
| `encodeIndexedHeader` | `(index: usize, out: []u8): usize` | Serializes/formats data via `encodeIndexedHeader` returning `usize`. |

## Minimal Example

```cp
import { hpackEntry } from "h2";

pub fn main() {
    let result = hpackEntry("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
