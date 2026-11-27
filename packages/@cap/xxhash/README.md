[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/xxhash

xxHash 0.8.3 extremely fast non-cryptographic hash algorithm (XXH32/XXH64/XXH3) for CP

| Metadata | Value |
| --- | --- |
| Package | `@cap/xxhash` |
| Version | `0.8.3` |
| License | `BSD-2-Clause` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/xxhash
```

```cp
import {
    XXH_versionNumber,
    XXH32,
    XXH64,
    XXH3_64bits,
    versionNumber,
    xxh32,
    xxh64,
    xxh3_64bits,
} from "@cap/xxhash";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `XXH_versionNumber` | `(): u32` | Invokes `XXH_versionNumber` and returns `u32`. |
| `XXH32` | `(input: [*]&u8!, length: usize, seed: u32): u32` | Invokes `XXH32` and returns `u32`. |
| `XXH64` | `(input: [*]&u8!, length: usize, seed: u64): u64` | Invokes `XXH64` and returns `u64`. |
| `XXH3_64bits` | `(input: [*]&u8!, length: usize): u64` | Invokes `XXH3_64bits` and returns `u64`. |
| `versionNumber` | `(): u32` | Invokes `versionNumber` and returns `u32`. |
| `xxh32` | `(data: &[]u8, seed: u32): u32` | Invokes `xxh32` and returns `u32`. |
| `xxh64` | `(data: &[]u8, seed: u64): u64` | Invokes `xxh64` and returns `u64`. |
| `xxh3_64bits` | `(data: &[]u8): u64` | Invokes `xxh3_64bits` and returns `u64`. |

## Minimal Example

```cp
import { versionNumber } from "@cap/xxhash";

pub fn main() {
    let result = versionNumber();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
