[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# clf-parser

Zero-allocation Apache/Nginx Common Log Format (CLF) and Combined Log Format parser and formatter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:clf-parser` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `clf-parser` → CP 包名 `clf-parser`）

| Metadata | Value |
| --- | --- |
| Package | `clf-parser` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add clf-parser
```

```cp
import {
    ClfRecord,
    invalidClf,
    isClfSpace,
    isLineEnd,
    parseClf,
    pushSlice,
    pushU64,
    formatClf,
} from "clf-parser";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ClfRecord` | `class` | Fields: `host_start: usize, host_len: usize, user_start: usize, user_len: usize, time_start: usize, time_len: usize, +14 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `invalidClf` | `(): ClfRecord` | Invokes `invalidClf` and returns `ClfRecord`. |
| `isClfSpace` | `(b: u8): bool` | Checks predicate `isClfSpace` and returns `bool`. |
| `isLineEnd` | `(b: u8): bool` | Checks predicate `isLineEnd` and returns `bool`. |
| `parseClf` | `(line: &[]u8): ClfRecord` | Parses input via `parseClf` and returns `ClfRecord`. |
| `pushSlice` | `(dst: []u8, pos: usize, src: &[]u8): i64` | Mutates or appends state via `pushSlice`. |
| `pushU64` | `(dst: []u8, pos: usize, val: u64): i64` | Mutates or appends state via `pushU64`. |
| `formatClf` | `(host: &[]u8, user: &[]u8, time: &[]u8, method: &[]u8, path: &[]u8, proto: &[]u8, status: u16, bytes: i64, dst: []u8): i64` | Serializes/formats data via `formatClf` returning `i64`. |

## Minimal Example

```cp
import { isClfSpace } from "clf-parser";

pub fn main() {
    let result = isClfSpace(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
