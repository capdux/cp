[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# hashring

Zero-allocation consistent hash ring with virtual nodes, O(log V) binary search routing, and deduplicated replica selection for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:hashring` ([https://github.com/jeromefroe/hashring-rs](https://github.com/jeromefroe/hashring-rs))（原库名 `hashring` → CP 包名 `hashring`）

| Metadata | Value |
| --- | --- |
| Package | `hashring` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add hashring
```

```cp
import {
    RingPoint,
    ReplicaList,
    HashRing,
    mulWrap64,
    avalanche64,
    fnv1a64Feed,
    hashKey,
    hashVnode,
} from "hashring";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RingPoint` | `class` | Fields: `hash: u64, node_idx: u16` |
| `ReplicaList` | `class` | Fields: `nodes: [8]string, count: usize` · Methods: `empty, nodeAt` |
| `HashRing` | `class` | Fields: `nodes: [32]string, node_active: [32]bool, active_nodes: usize, vnodes_per_node: u16, ring: [256]RingPoint, ring_len: usize` · Methods: `init, nodeCount, vnodeCount, addNode, removeNode, lowerBound, +2 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `mulWrap64` | `(a: u64, b: u64): u64` | Invokes `mulWrap64` and returns `u64`. |
| `avalanche64` | `(x: u64): u64` | Invokes `avalanche64` and returns `u64`. |
| `fnv1a64Feed` | `(state: u64, b: u8): u64` | Invokes `fnv1a64Feed` and returns `u64`. |
| `hashKey` | `(key: string): u64` | Checks predicate `hashKey` and returns `bool`. |
| `hashVnode` | `(name: string, v_idx: u16): u64` | Checks predicate `hashVnode` and returns `bool`. |
| `eqStr` | `(a: string, b: string): bool` | Invokes `eqStr` and returns `bool`. |

## Minimal Example

```cp
import { hashKey } from "hashring";

pub fn main() {
    let result = hashKey("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
