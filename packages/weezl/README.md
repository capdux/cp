[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# weezl

Zero-allocation GIF/TIFF/PDF LZW dictionary lossless compressor & decompressor with 9..12-bit variable code widths, Clear/EOI control codes, and KwKwK handling (Rust weezl parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:weezl` ([https://github.com/image-rs/lzw](https://github.com/image-rs/lzw))（原库名 `weezl` → CP 包名 `weezl`）

| Metadata | Value |
| --- | --- |
| Package | `weezl` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add weezl
```

```cp
import { hashPair, encode, decode } from "weezl";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `hashPair` | `(key: u32): usize` | Checks predicate `hashPair` and returns `bool`. |
| `encode` | `(src: &[]u8, dst: []u8): i64` | Serializes/formats data via `encode` returning `i64`. |
| `decode` | `(src: &[]u8, dst: []u8): i64` | Parses input via `decode` and returns `i64`. |

## Minimal Example

```cp
import { hashPair } from "weezl";

pub fn main() {
    let result = hashPair(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
