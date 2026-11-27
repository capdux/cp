# @cap/usearch — AI Agent Quick Ref

USearch (v2.26.6) high-performance HNSW vector search engine for CP AI Agents (`v2.26.6`, `Apache-2.0`)

## Import

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

## Core Types & Constants

```cp
error UsearchError { InvalidDimensions, InitFailed, ReserveFailed, AddFailed, SearchFailed, RemoveFailed }
class UsearchInitOptions { metric_kind: u32, metric: *auto!?, quantization: u32, dimensions: usize, /* +4 fields */ }
class VectorIndex { handle: *auto!?, dims: usize } // methods: initWithMetric(...) +10
```

## API Signatures

```cp
fn usearch_version(): [*:0]&u8!;
fn usearch_init(options: *UsearchInitOptions!, err: *[*:0]&u8!?!): *auto!?;
fn usearch_free(index: *auto!?, err: *[*:0]&u8!?!);
fn usearch_reserve(index: *auto!?, capacity: usize, err: *[*:0]&u8!?!);
fn usearch_add(index: *auto!?, key: u64, vector: *&anyopaque!, vector_kind: u32, err: *[*:0]&u8!?!);
fn usearch_search(index: *auto!?, query_vector: *&anyopaque!, query_kind: u32, count: usize, keys: [*]u64!, distances: [*]f32!, err: *[*:0]&u8!?!): usize;
fn usearch_size(index: *auto!?, err: *[*:0]&u8!?!): usize;
fn usearch_dimensions(index: *auto!?, err: *[*:0]&u8!?!): usize;
fn usearch_contains(index: *auto!?, key: u64, err: *[*:0]&u8!?!): bool;
fn usearch_remove(index: *auto!?, key: u64, err: *[*:0]&u8!?!): usize;
fn versionString(): string;
```

## Minimal Usage

```cp
import { versionString } from "@cap/usearch";

pub fn main() {
    let result = versionString();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/usearch"`; install via `cap add @cap/usearch`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.
