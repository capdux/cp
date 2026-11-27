[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# smol_str

Zero-heap pure-CP O(1)-clone small string optimization (24-byte inline SSO + read-only slice fallback, crates.io smol_str equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:smol_str` ([https://github.com/rust-analyzer/smol_str](https://github.com/rust-analyzer/smol_str))（原库名 `smol_str` → CP 包名 `smol_str`）

| Metadata | Value |
| --- | --- |
| Package | `smol_str` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add smol_str
```

```cp
import {
    SmolStr,
    smolGetByte,
    smolMinUsize,
    smolStartsWith,
    smolEndsWith,
} from "smol_str";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SmolStr` | `class` | Fields: `inline_buf: [24]u8, inline_len: u8, is_inline: bool, ext: string` · Methods: `empty, new, fromInt, len, isEmpty, isInline, +7 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `smolGetByte` | `(s: SmolStr, idx: usize): u8` | Invokes `smolGetByte` and returns `u8`. |
| `smolMinUsize` | `(a: usize, b: usize): usize` | Invokes `smolMinUsize` and returns `usize`. |
| `smolStartsWith` | `(s: SmolStr, prefix: string): bool` | Invokes `smolStartsWith` and returns `bool`. |
| `smolEndsWith` | `(s: SmolStr, suffix: string): bool` | Invokes `smolEndsWith` and returns `bool`. |

## Minimal Example

```cp
import { smolStartsWith } from "smol_str";

pub fn main() {
    let result = smolStartsWith(/* s: SmolStr */, "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
