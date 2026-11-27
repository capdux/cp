[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# leb128

Zero-heap ULEB128 (unsigned) and SLEB128 (signed) variable-length integer encoder/decoder for WebAssembly, DWARF & binary formats (compatible with Rust leb128)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:leb128` ([https://github.com/gimli-rs/leb128](https://github.com/gimli-rs/leb128)) / `npm:leb` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

| Metadata | Value |
| --- | --- |
| Package | `leb128` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add leb128
```

```cp
import {
    UlebResult,
    SlebResult,
    encodeUnsigned,
    decodeUnsigned,
    encodeSigned,
    decodeSigned,
    unsignedEncodedLen,
} from "leb128";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `UlebResult` | `class` | Fields: `value: u64, consumed: usize, valid: bool` |
| `SlebResult` | `class` | Fields: `value: i64, consumed: usize, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `encodeUnsigned` | `(val: u64, out: []u8): usize` | Serializes/formats data via `encodeUnsigned` returning `usize`. |
| `decodeUnsigned` | `(data: &[]u8, start: usize): UlebResult` | Parses input via `decodeUnsigned` and returns `UlebResult`. |
| `encodeSigned` | `(val: i64, out: []u8): usize` | Serializes/formats data via `encodeSigned` returning `usize`. |
| `decodeSigned` | `(data: &[]u8, start: usize): SlebResult` | Parses input via `decodeSigned` and returns `SlebResult`. |
| `unsignedEncodedLen` | `(val: u64): usize` | Invokes `unsignedEncodedLen` and returns `usize`. |

## Minimal Example

```cp
import { encodeUnsigned } from "leb128";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeUnsigned(0, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
