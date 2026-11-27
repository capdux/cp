[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# sieve-cache

Zero-allocation NSDI'24 SIEVE cache eviction engine with static doubly-linked pool and open-addressing index for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:sieve-cache` ([https://github.com/jedisct1/rust-sieve-cache](https://github.com/jedisct1/rust-sieve-cache))

| Metadata | Value |
| --- | --- |
| Package | `sieve-cache` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add sieve-cache
```

```cp
import {
    SieveNode,
    SieveCache,
    emptyNode,
    mulWrap64,
    hashSlot,
} from "sieve-cache";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SieveNode` | `class` | Fields: `key: u64, value: i64, visited: bool, in_use: bool, prev: u16, next: u16` |
| `SieveCache` | `class` | Fields: `nodes: [128]SieveNode, buckets: [256]u16, free_stack: [128]u16, free_top: usize, head: u16, tail: u16, +7 more` · Methods: `init, len, capacity, isEmpty, hits, misses, +14 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `emptyNode` | `(): SieveNode` | Invokes `emptyNode` and returns `SieveNode`. |
| `mulWrap64` | `(a: u64, b: u64): u64` | Invokes `mulWrap64` and returns `u64`. |
| `hashSlot` | `(key: u64): usize` | Checks predicate `hashSlot` and returns `bool`. |

## Minimal Example

```cp
import { hashSlot } from "sieve-cache";

pub fn main() {
    let result = hashSlot(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
