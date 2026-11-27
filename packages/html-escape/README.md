[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# html-escape

Zero-heap HTML entity escaper and unescaper (named entities + decimal/hex numeric character references) for CP (compatible with npm escape-html, he & Rust html-escape)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:html-escape` ([https://github.com/magiclen/html-escape](https://github.com/magiclen/html-escape)) / `npm:html-entities` ([https://github.com/mdevils/html-entities](https://github.com/mdevils/html-entities))

| Metadata | Value |
| --- | --- |
| Package | `html-escape` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add html-escape
```

```cp
import {
    appendStr,
    needsEscape,
    escapeHtml,
    encodeUtf8,
    decodeNamedEntity,
    decodeNumericEntity,
    unescapeHtml,
} from "html-escape";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `appendStr` | `(dst: []u8, pos: usize, src: string): usize` | Invokes `appendStr` and returns `usize`. |
| `needsEscape` | `(input: string): bool` | Invokes `needsEscape` and returns `bool`. |
| `escapeHtml` | `(input: string, out: []u8): usize` | Invokes `escapeHtml` and returns `usize`. |
| `encodeUtf8` | `(cp: u32, out: []u8, pos: usize): usize` | Serializes/formats data via `encodeUtf8` returning `usize`. |
| `decodeNamedEntity` | `(name: string): u32` | Parses input via `decodeNamedEntity` and returns `u32`. |
| `decodeNumericEntity` | `(body: string): u32` | Parses input via `decodeNumericEntity` and returns `u32`. |
| `unescapeHtml` | `(input: string, out: []u8): usize` | Invokes `unescapeHtml` and returns `usize`. |

## Minimal Example

```cp
import { decodeNamedEntity } from "html-escape";

pub fn main() {
    let result = decodeNamedEntity("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
