[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# potential_utf

Zero-allocation potentially-ill-formed UTF-8, UTF-16, and Unicode scalar value wrappers (ICU4X potential_utf parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:potential_utf` ([https://github.com/unicode-org/icu4x](https://github.com/unicode-org/icu4x))

| Metadata | Value |
| --- | --- |
| Package | `potential_utf` |
| Version | `1.0.0` |
| License | `Unicode-3.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add potential_utf
```

```cp
import {
    PotentialUtf8,
    PotentialUtf16,
    PotentialCodePoint,
    putfIsCont,
    putfValidSeqLen,
    putfMaximalSubpart,
    putfEncodeScalar,
    putfWriteReplacement,
} from "potential_utf";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PotentialUtf8` | `class` | Fields: `bytes: &[]u8` · Methods: `fromBytes, fromStr, asBytes, len, isEmpty, isValidUtf8, +2 more` |
| `PotentialUtf16` | `class` | Fields: `units: &[]u16` · Methods: `fromSlice, asSlice, len, isEmpty, isValidUtf16, toUtf8Lossy` |
| `PotentialCodePoint` | `class` | Fields: `raw: u32` · Methods: `fromU32, toU32, isValidScalar, toCharOrReplacement, encodeUtf8Lossy` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `putfIsCont` | `(b: u8): bool` | Mutates or appends state via `putfIsCont`. |
| `putfValidSeqLen` | `(bytes: &[]u8, pos: usize): usize` | Mutates or appends state via `putfValidSeqLen`. |
| `putfMaximalSubpart` | `(bytes: &[]u8, pos: usize): usize` | Mutates or appends state via `putfMaximalSubpart`. |
| `putfEncodeScalar` | `(scalar: u32, out: []u8, w: usize): usize` | Mutates or appends state via `putfEncodeScalar`. |
| `putfWriteReplacement` | `(out: []u8, w: usize): usize` | Mutates or appends state via `putfWriteReplacement`. |

## Minimal Example

```cp
import { putfIsCont } from "potential_utf";

pub fn main() {
    let result = putfIsCont(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
