[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @jridgewell/sourcemap-codec

Zero-heap Source Map v3 Base64 VLQ (Variable-Length Quantity) signed integer sequence encoder and decoder.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:@jridgewell/sourcemap-codec` ([https://github.com/jridgewell/sourcemap-codec](https://github.com/jridgewell/sourcemap-codec)) / `crates.io:vlq` ([https://github.com/fitzgen/vlq](https://github.com/fitzgen/vlq)) / `npm:vlq` ([https://github.com/Rich-Harris/vlq](https://github.com/Rich-Harris/vlq))（原库名 `@jridgewell/sourcemap-codec` → CP 包名 `@jridgewell/sourcemap-codec`）

| Metadata | Value |
| --- | --- |
| Package | `@jridgewell/sourcemap-codec` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @jridgewell/sourcemap-codec
```

```cp
import {
    DecodeResult,
    b64Char,
    b64Val,
    encodeOne,
    decodeOne,
    encodeSlice,
    decodeSlice,
} from "@jridgewell/sourcemap-codec";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `DecodeResult` | `class` | Fields: `value: i64, consumed: usize, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `b64Char` | `(v: u8): u8` | Invokes `b64Char` and returns `u8`. |
| `b64Val` | `(c: u8): i32` | Invokes `b64Val` and returns `i32`. |
| `encodeOne` | `(val: i64, out: []u8): usize` | Serializes/formats data via `encodeOne` returning `usize`. |
| `decodeOne` | `(input: string, start: usize): DecodeResult` | Parses input via `decodeOne` and returns `DecodeResult`. |
| `encodeSlice` | `(values: &[]i64, out: []u8): usize` | Serializes/formats data via `encodeSlice` returning `usize`. |
| `decodeSlice` | `(input: string, out: []i64): i64` | Parses input via `decodeSlice` and returns `i64`. |

## Minimal Example

```cp
import { decodeOne } from "@jridgewell/sourcemap-codec";

pub fn main() {
    let result = decodeOne("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
