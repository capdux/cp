[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# huffman-compress

Zero-allocation RFC 1951 Canonical Huffman prefix encoder & decoder with 256-symbol alphabet, 16-bit length limiting, and MSB-first bitstream framing (Rust huffman-compress parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:huffman-compress` ([https://github.com/sozu-proxy/huffman-compress](https://github.com/sozu-proxy/huffman-compress))（原库名 `huffman-compress` → CP 包名 `huffman-compress`）

| Metadata | Value |
| --- | --- |
| Package | `huffman-compress` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add huffman-compress
```

```cp
import { HuffmanCodec } from "huffman-compress";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `HuffmanCodec` | `class` | Fields: `code_len: [MAX_SYMBOLS]u8, codes: [MAX_SYMBOLS]u16, symbol_count: usize` · Methods: `empty, fromCodeLengths, fromFrequencies, codeLength, codeBits, encode, +1 more` |

## Minimal Example

```cp
import { HuffmanCodec } from "huffman-compress";

pub fn main() {
    // Refer to src/index.d.cp for HuffmanCodec fields and methods
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
