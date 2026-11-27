[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# strip-bom

Pure CP zero-allocation UTF-8/UTF-16/UTF-32 BOM detection, zero-copy stripping and idempotent prepending (npm strip-bom parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:strip-bom` ([https://github.com/sindresorhus/strip-bom](https://github.com/sindresorhus/strip-bom)) / `npm:strip-bom-buf` ([https://github.com/sindresorhus/strip-bom-buf](https://github.com/sindresorhus/strip-bom-buf))

| Metadata | Value |
| --- | --- |
| Package | `strip-bom` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add strip-bom
```

```cp
import {
    BomInfo,
    detectBom,
    hasUtf8Bom,
    stripUtf8Bom,
    stripAnyBomOffset,
    prependUtf8Bom,
} from "strip-bom";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `BomInfo` | `class` | Fields: `kind: u8, byte_len: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `detectBom` | `(data: []u8): BomInfo` | Invokes `detectBom` and returns `BomInfo`. |
| `hasUtf8Bom` | `(s: string): bool` | Checks predicate `hasUtf8Bom` and returns `bool`. |
| `stripUtf8Bom` | `(s: string): string` | Invokes `stripUtf8Bom` and returns `string`. |
| `stripAnyBomOffset` | `(data: []u8): usize` | Invokes `stripAnyBomOffset` and returns `usize`. |
| `prependUtf8Bom` | `(s: string, out: []u8): usize` | Invokes `prependUtf8Bom` and returns `usize`. |

## Minimal Example

```cp
import { hasUtf8Bom } from "strip-bom";

pub fn main() {
    let result = hasUtf8Bom("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
