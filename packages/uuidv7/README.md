[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# uuidv7

Zero-allocation RFC 9562 UUIDv7 constructor, 36-byte formatter, parser, timestamp extractor, and comparator in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:uuidv7` ([https://github.com/LiosK/uuidv7](https://github.com/LiosK/uuidv7)) / `crates.io:uuid7` ([https://github.com/LiosK/uuid7-rs](https://github.com/LiosK/uuid7-rs)) / `crates.io:uuid` ([https://github.com/uuid-rs/uuid](https://github.com/uuid-rs/uuid))（原库名 `uuidv7` → CP 包名 `uuidv7`）

| Metadata | Value |
| --- | --- |
| Package | `uuidv7` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add uuidv7
```

```cp
import {
    UuidV7,
    fromParts,
    timestampMs,
    version,
    isRfcVariant,
    formatHyphenated,
    parse,
    isValidUuidV7,
} from "uuidv7";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `UuidV7` | `class` | Fields: `bytes: [16]u8` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `fromParts` | `(timestamp_ms: u64, rand_bytes: [10]u8): UuidV7` | Invokes `fromParts` and returns `UuidV7`. |
| `timestampMs` | `(u: UuidV7): u64` | Invokes `timestampMs` and returns `u64`. |
| `version` | `(u: UuidV7): u8` | Invokes `version` and returns `u8`. |
| `isRfcVariant` | `(u: UuidV7): bool` | Checks predicate `isRfcVariant` and returns `bool`. |
| `formatHyphenated` | `(u: UuidV7, out: []u8): usize` | Serializes/formats data via `formatHyphenated` returning `usize`. |
| `parse` | `(s: string, out: &mut UuidV7): bool` | Parses input via `parse` and returns `bool`. |
| `isValidUuidV7` | `(s: string): bool` | Checks predicate `isValidUuidV7` and returns `bool`. |
| `compare` | `(a: UuidV7, b: UuidV7): i32` | Invokes `compare` and returns `i32`. |

## Minimal Example

```cp
import { parse } from "uuidv7";

pub fn main() {
    let result = parse("example", /* out: &mut UuidV7 */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
