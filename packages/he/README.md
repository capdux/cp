[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# he

Zero-allocation HTML5 named entity & numeric character reference UTF-8 encoder/decoder and XSS escaper (he / escape-html / html-entities / Rust html-escape parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:he` ([https://github.com/mathiasbynens/he](https://github.com/mathiasbynens/he))（原库名 `he` → CP 包名 `he`）

| Metadata | Value |
| --- | --- |
| Package | `he` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add he
```

```cp
import {
    pushByte,
    writeUtf8,
    pushHexEntity,
    needsHtmlEscape,
    escapeHtml,
    encodeNonAscii,
    lookupNamedEntity,
    decodeHtmlEntities,
} from "he";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `pushByte` | `(out: []u8, pos: usize, b: u8): usize` | Mutates or appends state via `pushByte`. |
| `writeUtf8` | `(out: []u8, pos: usize, raw_cp: u32): usize` | Invokes `writeUtf8` and returns `usize`. |
| `pushHexEntity` | `(out: []u8, pos: usize, cp: u32): usize` | Mutates or appends state via `pushHexEntity`. |
| `needsHtmlEscape` | `(s: string): bool` | Invokes `needsHtmlEscape` and returns `bool`. |
| `escapeHtml` | `(input: string, out: []u8): string` | Invokes `escapeHtml` and returns `string`. |
| `encodeNonAscii` | `(input: string, out: []u8): string` | Serializes/formats data via `encodeNonAscii` returning `string`. |
| `lookupNamedEntity` | `(name: string): u32` | Queries/retrieves data via `lookupNamedEntity` returning `u32`. |
| `decodeHtmlEntities` | `(input: string, out: []u8): string` | Parses input via `decodeHtmlEntities` and returns `string`. |

## Minimal Example

```cp
import { encodeNonAscii } from "he";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeNonAscii("example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
