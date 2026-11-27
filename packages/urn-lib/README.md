[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# urn-lib

Zero-allocation RFC 8141 Uniform Resource Name (URN) parser, equivalence comparator, and canonical formatter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:urn-lib` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:urn` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `urn-lib` → CP 包名 `urn-lib`）

| Metadata | Value |
| --- | --- |
| Package | `urn-lib` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add urn-lib
```

```cp
import {
    Urn,
    invalidUrn,
    urnIsSpace,
    isValidNid,
    parseUrn,
    urnEquivalent,
    pushSlice,
    pushLowerSlice,
} from "urn-lib";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Urn` | `class` | Fields: `nid: string, nss: string, r_component: string, q_component: string, f_component: string, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `invalidUrn` | `(): Urn` | Invokes `invalidUrn` and returns `Urn`. |
| `urnIsSpace` | `(b: u8): bool` | Invokes `urnIsSpace` and returns `bool`. |
| `isValidNid` | `(nid: string): bool` | Checks predicate `isValidNid` and returns `bool`. |
| `parseUrn` | `(input: string): Urn` | Parses input via `parseUrn` and returns `Urn`. |
| `urnEquivalent` | `(a: Urn, b: Urn): bool` | Invokes `urnEquivalent` and returns `bool`. |
| `pushSlice` | `(dst: []u8, pos: usize, src: string): i64` | Mutates or appends state via `pushSlice`. |
| `pushLowerSlice` | `(dst: []u8, pos: usize, src: string): i64` | Mutates or appends state via `pushLowerSlice`. |
| `formatUrn` | `(u: Urn, out: []u8): usize` | Serializes/formats data via `formatUrn` returning `usize`. |

## Minimal Example

```cp
import { isValidNid } from "urn-lib";

pub fn main() {
    let result = isValidNid("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
