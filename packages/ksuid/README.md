[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ksuid

Zero-heap Segment KSUID (20-byte K-Sortable Unique IDentifier) 27-character Base62 encoder, decoder, validator, and comparator.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ksuid` ([https://github.com/segmentio/ksuid](https://github.com/segmentio/ksuid)) / `crates.io:svix-ksuid` ([https://github.com/svix/rust-ksuid](https://github.com/svix/rust-ksuid))

| Metadata | Value |
| --- | --- |
| Package | `ksuid` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ksuid
```

```cp
import {
    Ksuid,
    b62Char,
    b62Val,
    fromParts,
    timestampSecs,
    encode,
    decode,
    isValidKsuid,
} from "ksuid";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Ksuid` | `class` | Fields: `bytes: [20]u8` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `b62Char` | `(v: u8): u8` | Invokes `b62Char` and returns `u8`. |
| `b62Val` | `(c: u8): i32` | Invokes `b62Val` and returns `i32`. |
| `fromParts` | `(unix_secs: u64, payload: [16]u8): Ksuid` | Invokes `fromParts` and returns `Ksuid`. |
| `timestampSecs` | `(k: Ksuid): u64` | Invokes `timestampSecs` and returns `u64`. |
| `encode` | `(k: Ksuid, out: []u8): usize` | Serializes/formats data via `encode` returning `usize`. |
| `decode` | `(s: string, out: &mut Ksuid): bool` | Parses input via `decode` and returns `bool`. |
| `isValidKsuid` | `(s: string): bool` | Checks predicate `isValidKsuid` and returns `bool`. |
| `compare` | `(a: Ksuid, b: Ksuid): i32` | Invokes `compare` and returns `i32`. |

## Minimal Example

```cp
import { decode } from "ksuid";

pub fn main() {
    let result = decode("example", /* out: &mut Ksuid */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
