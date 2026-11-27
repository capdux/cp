[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# rle-decode-fast

Zero-allocation Apple/TIFF PackBits byte run-length compression and Prometheus/Gorilla-style Delta & Delta-of-Delta time-series integer encoding for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rle-decode-fast` ([https://github.com/okaneco/rle-decode-fast](https://github.com/okaneco/rle-decode-fast))（原库名 `rle-decode-fast` → CP 包名 `rle-decode-fast`）

| Metadata | Value |
| --- | --- |
| Package | `rle-decode-fast` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add rle-decode-fast
```

```cp
import {
    encodePackBits,
    decodePackBits,
    encodeDeltaI64,
    decodeDeltaI64,
    encodeDeltaOfDeltaI64,
    decodeDeltaOfDeltaI64,
} from "rle-decode-fast";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `encodePackBits` | `(input: string, out: []u8): string?` | Serializes/formats data via `encodePackBits` returning `string?`. |
| `decodePackBits` | `(encoded: string, out: []u8): string?` | Parses input via `decodePackBits` and returns `string?`. |
| `encodeDeltaI64` | `(values: &[]i64, out: []i64): usize` | Serializes/formats data via `encodeDeltaI64` returning `usize`. |
| `decodeDeltaI64` | `(deltas: &[]i64, out: []i64): usize` | Parses input via `decodeDeltaI64` and returns `usize`. |
| `encodeDeltaOfDeltaI64` | `(values: &[]i64, out: []i64): usize` | Serializes/formats data via `encodeDeltaOfDeltaI64` returning `usize`. |
| `decodeDeltaOfDeltaI64` | `(dod: &[]i64, out: []i64): usize` | Parses input via `decodeDeltaOfDeltaI64` and returns `usize`. |

## Minimal Example

```cp
import { encodePackBits } from "rle-decode-fast";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodePackBits("example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
