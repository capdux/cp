[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# roaring

Zero-allocation Roaring Bitmap with 16-bit high-key containers, binary search, rank, and set algebra (Rust roaring parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`roaring`](https://github.com/RoaringBitmap/roaring-rs)（`MIT OR Apache-2.0`）（包名 `roaring`）。

| Metadata | Value |
| --- | --- |
| Package | `roaring` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add roaring
```

```cp
import {
    BinSearch,
    RoaringContainer,
    RoaringBitmap,
    highKey,
    lowVal,
    combineKeyVal,
    emptyContainer,
    searchU16,
} from "roaring";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `BinSearch` | `class` | Fields: `found: bool, pos: usize` |
| `RoaringContainer` | `class` | Fields: `key: u16, values: [MAX_CONTAINER_ITEMS]u16, count: usize` |
| `RoaringBitmap` | `class` | Fields: `containers: [MAX_CONTAINERS]RoaringContainer, container_count: usize, total_len: u64` · Methods: `init, insert, remove, contains, len, isEmpty, +7 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `highKey` | `(v: u32): u16` | Invokes `highKey` and returns `u16`. |
| `lowVal` | `(v: u32): u16` | Invokes `lowVal` and returns `u16`. |
| `combineKeyVal` | `(hi: u16, lo: u16): u32` | Invokes `combineKeyVal` and returns `u32`. |
| `emptyContainer` | `(): RoaringContainer` | Invokes `emptyContainer` and returns `RoaringContainer`. |
| `searchU16` | `(arr: [MAX_CONTAINER_ITEMS]u16, count: usize, target: u16): BinSearch` | Invokes `searchU16` and returns `BinSearch`. |
| `searchContainer` | `(containers: [MAX_CONTAINERS]RoaringContainer, count: usize, key: u16): BinSearch` | Invokes `searchContainer` and returns `BinSearch`. |
| `unionContainers` | `(a: RoaringContainer, b: RoaringContainer): RoaringContainer` | Invokes `unionContainers` and returns `RoaringContainer`. |
| `intersectContainers` | `(a: RoaringContainer, b: RoaringContainer): RoaringContainer` | Invokes `intersectContainers` and returns `RoaringContainer`. |
| `diffContainers` | `(a: RoaringContainer, b: RoaringContainer): RoaringContainer` | Invokes `diffContainers` and returns `RoaringContainer`. |

## Minimal Example

```cp
import { highKey } from "roaring";

pub fn main() {
    let result = highKey(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
