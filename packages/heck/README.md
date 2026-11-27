[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# heck

Zero-allocation Rust heck-compatible case conversion engine (PascalCase, camelCase, snake_case, SHOUTY_SNAKE_CASE, kebab-case, SHOUTY-KEBAB-CASE, Title Case, Train-Case) for CP (clean-room MIT).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:heck` ([https://github.com/withoutboats/heck](https://github.com/withoutboats/heck))（原库名 `heck` → CP 包名 `heck`）

| Metadata | Value |
| --- | --- |
| Package | `heck` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add heck
```

```cp
import {
    HckRune,
    hckDecodeUtf8,
    hckClassify,
    hckToLowerCp,
    hckToUpperCp,
    hckEncodeUtf8,
    hckEmitWord,
    hckConvert,
} from "heck";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `HckRune` | `class` | Fields: `cp: u32, len: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `hckDecodeUtf8` | `(src: string, offset: usize): HckRune` | Invokes `hckDecodeUtf8` and returns `HckRune`. |
| `hckClassify` | `(cp: u32): u8` | Invokes `hckClassify` and returns `u8`. |
| `hckToLowerCp` | `(cp: u32): u32` | Invokes `hckToLowerCp` and returns `u32`. |
| `hckToUpperCp` | `(cp: u32): u32` | Invokes `hckToUpperCp` and returns `u32`. |
| `hckEncodeUtf8` | `(cp: u32, out: []u8, pos: usize): usize` | Invokes `hckEncodeUtf8` and returns `usize`. |
| `hckEmitWord` | `(src: string, start: usize, end: usize, out: []u8, wpos: usize, word_idx: usize, sep: u8, style: u8): usize` | Invokes `hckEmitWord` and returns `usize`. |
| `hckConvert` | `(src: string, out: []u8, sep: u8, style: u8): usize` | Invokes `hckConvert` and returns `usize`. |
| `toUpperCamelCase` | `(src: string, out: []u8): usize` | Invokes `toUpperCamelCase` and returns `usize`. |
| `toLowerCamelCase` | `(src: string, out: []u8): usize` | Invokes `toLowerCamelCase` and returns `usize`. |
| `toSnakeCase` | `(src: string, out: []u8): usize` | Invokes `toSnakeCase` and returns `usize`. |
| `toShoutySnakeCase` | `(src: string, out: []u8): usize` | Invokes `toShoutySnakeCase` and returns `usize`. |
| `toKebabCase` | `(src: string, out: []u8): usize` | Invokes `toKebabCase` and returns `usize`. |
| `toShoutyKebabCase` | `(src: string, out: []u8): usize` | Invokes `toShoutyKebabCase` and returns `usize`. |
| `toTitleCase` | `(src: string, out: []u8): usize` | Invokes `toTitleCase` and returns `usize`. |
| `toTrainCase` | `(src: string, out: []u8): usize` | Invokes `toTrainCase` and returns `usize`. |

## Minimal Example

```cp
import { hckDecodeUtf8 } from "heck";

pub fn main() {
    let result = hckDecodeUtf8("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
