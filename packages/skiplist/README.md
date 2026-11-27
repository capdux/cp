[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# skiplist

Zero-allocation static-pool deterministic skip list (O(log N) ordered map, lowerBound, rangeCount, freelist reuse) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:skiplist` ([https://github.com/JP-Ellis/rust-skiplist](https://github.com/JP-Ellis/rust-skiplist))

| Metadata | Value |
| --- | --- |
| Package | `skiplist` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add skiplist
```

```cp
import { SkipEntry, SkipNode, SkipList, emptyNode } from "skiplist";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SkipEntry` | `class` | Fields: `key: i64, value: i64, found: bool` · Methods: `none` |
| `SkipNode` | `class` | Fields: `key: i64, value: i64, level: u8, next: [8]u16` |
| `SkipList` | `class` | Fields: `nodes: [129]SkipNode, free_stack: [128]u16, free_top: usize, cur_level: u8, count: usize, rng_state: u64` · Methods: `init, len, isEmpty, clear, randomLevel, insert, +8 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `emptyNode` | `(): SkipNode` | Invokes `emptyNode` and returns `SkipNode`. |

## Minimal Example

```cp
import { emptyNode } from "skiplist";

pub fn main() {
    let result = emptyNode();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
