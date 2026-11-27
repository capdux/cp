[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# escalade

Zero-heap ancestor directory path walker (clean-room CP implementation of npm escalade)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:escalade` ([https://github.com/lukeed/escalade](https://github.com/lukeed/escalade))

| Metadata | Value |
| --- | --- |
| Package | `escalade` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add escalade
```

```cp
import { AncestorWalker, joinCandidate } from "escalade";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `AncestorWalker` | `class` | Fields: `buf: [512]u8, len: usize, done: bool` · Methods: `new, isDone, next, remainingDepth` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `joinCandidate` | `(dir: string, name: string, out: []u8): usize` | Invokes `joinCandidate` and returns `usize`. |

## Minimal Example

```cp
import { joinCandidate } from "escalade";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = joinCandidate("example", "example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
