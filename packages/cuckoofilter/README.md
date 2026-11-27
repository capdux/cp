[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# cuckoofilter

Zero-allocation CoNEXT'14 Cuckoo Filter (256x4 16-bit fingerprint slots) with O(1) insert, contains, and safe delete for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:cuckoofilter` ([https://github.com/seiflotfy/rust-cuckoofilter](https://github.com/seiflotfy/rust-cuckoofilter))（原库名 `cuckoofilter` → CP 包名 `cuckoofilter`）

| Metadata | Value |
| --- | --- |
| Package | `cuckoofilter` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add cuckoofilter
```

```cp
import {
    CuckooFilter,
    mulWrap64,
    avalanche64,
    hashBytes,
    fingerprintFromHash,
    altBucket,
} from "cuckoofilter";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CuckooFilter` | `class` | Fields: `buckets: [1024]u16, item_count: usize, rng_state: u64` · Methods: `init, len, capacity, isEmpty, loadFactor, clear, +7 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `mulWrap64` | `(a: u64, b: u64): u64` | Invokes `mulWrap64` and returns `u64`. |
| `avalanche64` | `(x: u64): u64` | Invokes `avalanche64` and returns `u64`. |
| `hashBytes` | `(data: &[]u8): u64` | Checks predicate `hashBytes` and returns `bool`. |
| `fingerprintFromHash` | `(h: u64): u16` | Invokes `fingerprintFromHash` and returns `u16`. |
| `altBucket` | `(bucket: usize, fp: u16): usize` | Invokes `altBucket` and returns `usize`. |

## Minimal Example

```cp
import { hashBytes } from "cuckoofilter";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = hashBytes(out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
