[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# boyer-moore-magiclen

Zero-heap Boyer-Moore-Horspool sublinear substring search with reusable 256-byte bad-character table, reverse search, counting, and ASCII case-insensitive search.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:boyer-moore-magiclen` ([https://github.com/magiclen/boyer-moore-magiclen](https://github.com/magiclen/boyer-moore-magiclen))（原库名 `boyer-moore-magiclen` → CP 包名 `boyer-moore-magiclen`）

| Metadata | Value |
| --- | --- |
| Package | `boyer-moore-magiclen` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add boyer-moore-magiclen
```

```cp
import {
    BmhPattern,
    compilePattern,
    findWithCompiled,
    findFirst,
    findLast,
    countNonOverlapping,
    findIgnoreCaseAscii,
} from "boyer-moore-magiclen";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `BmhPattern` | `class` | Fields: `shift: [256]usize, pat_len: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `compilePattern` | `(needle: &[]u8): BmhPattern` | Invokes `compilePattern` and returns `BmhPattern`. |
| `findWithCompiled` | `(haystack: &[]u8, needle: &[]u8, compiled: BmhPattern): i64` | Queries/retrieves data via `findWithCompiled` returning `i64`. |
| `findFirst` | `(haystack: &[]u8, needle: &[]u8): i64` | Queries/retrieves data via `findFirst` returning `i64`. |
| `findLast` | `(haystack: &[]u8, needle: &[]u8): i64` | Queries/retrieves data via `findLast` returning `i64`. |
| `countNonOverlapping` | `(haystack: &[]u8, needle: &[]u8): usize` | Invokes `countNonOverlapping` and returns `usize`. |
| `findIgnoreCaseAscii` | `(haystack: string, needle: string): i64` | Queries/retrieves data via `findIgnoreCaseAscii` returning `i64`. |

## Minimal Example

```cp
import { findIgnoreCaseAscii } from "boyer-moore-magiclen";

pub fn main() {
    let result = findIgnoreCaseAscii("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
