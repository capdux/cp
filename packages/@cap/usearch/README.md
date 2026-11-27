[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/usearch

USearch (v2.26.6) high-performance HNSW vector search engine for CP AI Agents

| Metadata | Value |
| --- | --- |
| Package | `@cap/usearch` |
| Version | `2.26.6` |
| License | `Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/usearch
```

```cp
import {
    UsearchError,
    UsearchInitOptions,
    VectorIndex,
    usearch_version,
    usearch_init,
    usearch_free,
    usearch_reserve,
    usearch_add,
} from "@cap/usearch";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `UsearchError` | `InvalidDimensions, InitFailed, ReserveFailed, AddFailed, SearchFailed, RemoveFailed, IndexClosed` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `UsearchInitOptions` | `class` | Fields: `metric_kind: u32, metric: *auto!?, quantization: u32, dimensions: usize, connectivity: usize, expansion_add: usize, +2 more` |
| `VectorIndex` | `class` | Fields: `handle: *auto!?, dims: usize` · Methods: `initWithMetric, initCosine, initL2, reserve, addF32, searchF32, +5 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `usearch_version` | `(): [*:0]&u8!` | Invokes `usearch_version` and returns `[*:0]&u8!`. |
| `usearch_init` | `(options: *UsearchInitOptions!, err: *[*:0]&u8!?!): *auto!?` | Invokes `usearch_init` and returns `*auto!?`. |
| `usearch_free` | `(index: *auto!?, err: *[*:0]&u8!?!)` | Executes `usearch_free` in-place. |
| `usearch_reserve` | `(index: *auto!?, capacity: usize, err: *[*:0]&u8!?!)` | Executes `usearch_reserve` in-place. |
| `usearch_add` | `(index: *auto!?, key: u64, vector: *&anyopaque!, vector_kind: u32, err: *[*:0]&u8!?!)` | Executes `usearch_add` in-place. |
| `usearch_search` | `(index: *auto!?, query_vector: *&anyopaque!, query_kind: u32, count: usize, keys: [*]u64!, distances: [*]f32!, err: *[*:0]&u8!?!): usize` | Invokes `usearch_search` and returns `usize`. |
| `usearch_size` | `(index: *auto!?, err: *[*:0]&u8!?!): usize` | Invokes `usearch_size` and returns `usize`. |
| `usearch_dimensions` | `(index: *auto!?, err: *[*:0]&u8!?!): usize` | Invokes `usearch_dimensions` and returns `usize`. |
| `usearch_contains` | `(index: *auto!?, key: u64, err: *[*:0]&u8!?!): bool` | Invokes `usearch_contains` and returns `bool`. |
| `usearch_remove` | `(index: *auto!?, key: u64, err: *[*:0]&u8!?!): usize` | Invokes `usearch_remove` and returns `usize`. |
| `versionString` | `(): string` | Invokes `versionString` and returns `string`. |

## Minimal Example

```cp
import { versionString } from "@cap/usearch";

pub fn main() {
    let result = versionString();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Deterministic resource cleanup**: Native handles provide RAII `__cp_deinit` or explicit `defer` release functions to prevent leaks.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
