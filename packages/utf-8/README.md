[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# utf-8

Zero-copy incremental streaming UTF-8 decoder and RFC 3629 validator with Unicode Maximal Subpart lossy recovery in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:utf-8` ([https://github.com/SimonSapin/rust-utf8](https://github.com/SimonSapin/rust-utf8)) — `MIT OR Apache-2.0`

| Metadata | Value |
| --- | --- |
| Package | `utf-8` |
| Version | `1.0.0` |
| License | `MIT OR Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add utf-8
```

```cp
import {
    Utf8DecodeStatus,
    Utf8ChunkResult,
    LossyUtf8Decoder,
    u8dExpectedLen,
    u8dIsValidSecond,
    u8dIsValidCont,
    u8dWriteSlice,
    u8dWriteReplacement,
} from "utf-8";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `Utf8DecodeStatus` | `Ok, InvalidSequence, Incomplete` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Utf8ChunkResult` | `class` | Fields: `status: Utf8DecodeStatus, valid_prefix: string, consumed: usize, error_len: usize` |
| `LossyUtf8Decoder` | `class` | Fields: `pending: [4]u8, pending_len: usize` · Methods: `new, hasPending, pendingLen, reset, push, finish` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `u8dExpectedLen` | `(b0: u8): usize` | Invokes `u8dExpectedLen` and returns `usize`. |
| `u8dIsValidSecond` | `(b0: u8, b1: u8): bool` | Invokes `u8dIsValidSecond` and returns `bool`. |
| `u8dIsValidCont` | `(b0: u8, idx: usize, b: u8): bool` | Invokes `u8dIsValidCont` and returns `bool`. |
| `u8dWriteSlice` | `(out: []u8, pos: usize, src: &[]u8): usize` | Invokes `u8dWriteSlice` and returns `usize`. |
| `u8dWriteReplacement` | `(out: []u8, pos: usize): usize` | Invokes `u8dWriteReplacement` and returns `usize`. |
| `decodeUtf8Chunk` | `(input: &[]u8): Utf8ChunkResult` | Parses input via `decodeUtf8Chunk` and returns `Utf8ChunkResult`. |
| `isValidUtf8` | `(input: &[]u8): bool` | Checks predicate `isValidUtf8` and returns `bool`. |

## Minimal Example

```cp
import { decodeUtf8Chunk } from "utf-8";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = decodeUtf8Chunk(out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
