[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# shortid

URL-friendly non-sequential short unique ID generator and validator (clean-room CP implementation of npm shortid)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:shortid` ([https://github.com/dylang/shortid](https://github.com/dylang/shortid))

| Metadata | Value |
| --- | --- |
| Package | `shortid` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add shortid
```

```cp
import { ShortIdGenerator, sidU64, sidCharIndex, isValidShortId } from "shortid";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ShortIdGenerator` | `class` | Fields: `alphabet: [64]u8, worker_id: u8, fmt_version: u8, last_tick: u64, counter: u32, rng_state: u64` · Methods: `new, workerId, version, nextRand, encodeValue, generate, +3 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `sidU64` | `(hi: u32, lo: u32): u64` | Invokes `sidU64` and returns `u64`. |
| `sidCharIndex` | `(c: u8, alphabet: string): i32` | Invokes `sidCharIndex` and returns `i32`. |
| `isValidShortId` | `(id: string): bool` | Checks predicate `isValidShortId` and returns `bool`. |

## Minimal Example

```cp
import { isValidShortId } from "shortid";

pub fn main() {
    let result = isValidShortId("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
