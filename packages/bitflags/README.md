[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# bitflags

Zero-heap 64-bit bitflags set algebra (contains, intersects, insert, remove, toggle, difference, popcount, trailing/leading zeros) for CP (compatible with Rust bitflags & npm bitfield)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:bitfield` ([https://github.com/fb55/bitfield](https://github.com/fb55/bitfield))（原库名 `bitfield` → CP 包名 `bitflags`）

| Metadata | Value |
| --- | --- |
| Package | `bitflags` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add bitflags
```

```cp
import {
    Flags64,
    empty,
    fromBits,
    fromBitIndex,
    isEmpty,
    contains,
    intersects,
    unionFlags,
} from "bitflags";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Flags64` | `class` | Fields: `bits: u64` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `empty` | `(): Flags64` | Invokes `empty` and returns `Flags64`. |
| `fromBits` | `(raw: u64): Flags64` | Invokes `fromBits` and returns `Flags64`. |
| `fromBitIndex` | `(idx: u8): Flags64` | Invokes `fromBitIndex` and returns `Flags64`. |
| `isEmpty` | `(f: Flags64): bool` | Checks predicate `isEmpty` and returns `bool`. |
| `contains` | `(f: Flags64, other: Flags64): bool` | Invokes `contains` and returns `bool`. |
| `intersects` | `(f: Flags64, other: Flags64): bool` | Invokes `intersects` and returns `bool`. |
| `unionFlags` | `(a: Flags64, b: Flags64): Flags64` | Invokes `unionFlags` and returns `Flags64`. |
| `intersection` | `(a: Flags64, b: Flags64): Flags64` | Invokes `intersection` and returns `Flags64`. |
| `difference` | `(a: Flags64, b: Flags64): Flags64` | Invokes `difference` and returns `Flags64`. |
| `symmetricDifference` | `(a: Flags64, b: Flags64): Flags64` | Invokes `symmetricDifference` and returns `Flags64`. |
| `insert` | `(f: &mut Flags64, other: Flags64)` | Executes `insert` in-place. |
| `remove` | `(f: &mut Flags64, other: Flags64)` | Executes `remove` in-place. |
| `toggle` | `(f: &mut Flags64, other: Flags64)` | Executes `toggle` in-place. |
| `setCond` | `(f: &mut Flags64, other: Flags64, enabled: bool)` | Mutates or appends state via `setCond`. |
| `truncate` | `(raw: u64, valid_mask: u64): Flags64` | Invokes `truncate` and returns `Flags64`. |
| `countBits` | `(f: Flags64): u32` | Invokes `countBits` and returns `u32`. |
| `lowestBitIndex` | `(f: Flags64): i32` | Invokes `lowestBitIndex` and returns `i32`. |
| `formatHex` | `(f: Flags64, out: []u8): usize` | Serializes/formats data via `formatHex` returning `usize`. |

## Minimal Example

```cp
import { isEmpty } from "bitflags";

pub fn main() {
    let result = isEmpty(/* f: Flags64 */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
