[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# lexical-write-float

Zero-allocation pure CP integer and floating-point formatter (rust-lexical lexical-write-float / lexical-write-integer parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:lexical-write-float` ([https://github.com/Alexhuszagh/rust-lexical](https://github.com/Alexhuszagh/rust-lexical))

| Metadata | Value |
| --- | --- |
| Package | `lexical-write-float` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add lexical-write-float
```

```cp
import {
    lexwCopyStr,
    lexwWritePaddedU64,
    lexwWriteSpecialFloat,
    writeUintRadix,
    writeIntRadix,
    writeFloatFixed,
    writeFloatSci,
} from "lexical-write-float";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `lexwCopyStr` | `(out: []u8, pos: usize, src: string): usize` | Invokes `lexwCopyStr` and returns `usize`. |
| `lexwWritePaddedU64` | `(out: []u8, pos: usize, val: u64, width: usize): usize` | Invokes `lexwWritePaddedU64` and returns `usize`. |
| `lexwWriteSpecialFloat` | `(val: f64, out: []u8): usize` | Invokes `lexwWriteSpecialFloat` and returns `usize`. |
| `writeUintRadix` | `(val: u64, radix: u32, uppercase: bool, out: []u8): usize` | Invokes `writeUintRadix` and returns `usize`. |
| `writeIntRadix` | `(val: i64, radix: u32, uppercase: bool, out: []u8): usize` | Invokes `writeIntRadix` and returns `usize`. |
| `writeFloatFixed` | `(val: f64, decimals: u8, out: []u8): usize` | Invokes `writeFloatFixed` and returns `usize`. |
| `writeFloatSci` | `(val: f64, precision: u8, out: []u8): usize` | Invokes `writeFloatSci` and returns `usize`. |

## Minimal Example

```cp
import { lexwCopyStr } from "lexical-write-float";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = lexwCopyStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
