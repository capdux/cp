[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# character-entities

Zero-allocation HTML5 named and numeric character reference decoder and encoder (covers decode-named-character-reference, parse-entities, stringify-entities)

| Metadata | Value |
| --- | --- |
| Package | `character-entities` |
| Version | `2.0.2` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add character-entities
```

```cp
import {
    mapInvalidCharRef,
    decodeNamedEntity,
    decodeNumericEntity,
    decodeEntities,
    encodeEntities,
} from "character-entities";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `mapInvalidCharRef` | `(cp: u32): u32` | Invokes `mapInvalidCharRef` and returns `u32`. |
| `decodeNamedEntity` | `(name: string): u32?` | Parses input via `decodeNamedEntity` and returns `u32?`. |
| `decodeNumericEntity` | `(body: string): u32?` | Parses input via `decodeNumericEntity` and returns `u32?`. |
| `decodeEntities` | `(input: string, out: []u8): usize` | Parses input via `decodeEntities` and returns `usize`. |
| `encodeEntities` | `(input: string, out: []u8): usize` | Serializes/formats data via `encodeEntities` returning `usize`. |

## Minimal Example

```cp
import { decodeNamedEntity } from "character-entities";

pub fn main() {
    let result = decodeNamedEntity("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
