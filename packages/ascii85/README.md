[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ascii85

Zero-allocation Adobe Ascii85 (PostScript / PDF base-85 with 'z' zero compression and <~...~> delimiters) encoder and decoder for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ascii85` ([https://github.com/noseglid/node-ascii85](https://github.com/noseglid/node-ascii85)) / `crates.io:ascii85` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

| Metadata | Value |
| --- | --- |
| Package | `ascii85` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ascii85
```

```cp
import {
    buildA85DecLut,
    a85IsSpace,
    maxEncodedLen,
    encode,
    decode,
} from "ascii85";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `buildA85DecLut` | `(): [256]u8` | Invokes `buildA85DecLut` and returns `[256]u8`. |
| `a85IsSpace` | `(c: u8): bool` | Invokes `a85IsSpace` and returns `bool`. |
| `maxEncodedLen` | `(byte_len: usize, with_delimiters: bool): usize` | Invokes `maxEncodedLen` and returns `usize`. |
| `encode` | `(data: &[]u8, with_delimiters: bool, out: []u8): usize` | Serializes/formats data via `encode` returning `usize`. |
| `decode` | `(input: string, out: []u8): i64` | Parses input via `decode` and returns `i64`. |

## Minimal Example

```cp
import { decode } from "ascii85";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = decode("example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
