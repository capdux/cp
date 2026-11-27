[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# toml_writer

Zero-heap TOML 1.0 serializer and formatter (bare/quoted keys, escaped strings, tables, array-of-tables, inline arrays) inspired by Rust toml_writer

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:toml_writer` ([https://github.com/toml-rs/toml](https://github.com/toml-rs/toml))（原库名 `toml_writer` → CP 包名 `toml_writer`）

| Metadata | Value |
| --- | --- |
| Package | `toml_writer` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add toml_writer
```

```cp
import { TomlWriter, isBareKey } from "toml_writer";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `TomlWriter` | `class` | Fields: `out: []u8, pos: usize, overflow: bool` · Methods: `init, asSlice, putByte, putRaw, writeQuotedString, writeKey, +9 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isBareKey` | `(k: string): bool` | Checks predicate `isBareKey` and returns `bool`. |

## Minimal Example

```cp
import { isBareKey } from "toml_writer";

pub fn main() {
    let result = isBareKey("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
