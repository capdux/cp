[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# convert_case

Zero-allocation 14-style string case converter (crates.io convert_case v0.6 parity) composing heck-case for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:convert_case` ([https://github.com/rutrum/convert-case](https://github.com/rutrum/convert-case))（原库名 `convert_case` → CP 包名 `convert_case`）

| Metadata | Value |
| --- | --- |
| Package | `convert_case` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add convert_case
```

```cp
import {
    ccvReplaceUnderscoreWithSpace,
    ccvStripUnderscores,
    toFlatCase,
    toUpperFlatCase,
    toLowerWords,
    toUpperWords,
    toAlternatingCase,
    convertCase,
} from "convert_case";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ccvReplaceUnderscoreWithSpace` | `(out: []u8, len: usize)` | Executes `ccvReplaceUnderscoreWithSpace` in-place. |
| `ccvStripUnderscores` | `(out: []u8, len: usize): usize` | Invokes `ccvStripUnderscores` and returns `usize`. |
| `toFlatCase` | `(src: string, out: []u8): usize` | Invokes `toFlatCase` and returns `usize`. |
| `toUpperFlatCase` | `(src: string, out: []u8): usize` | Invokes `toUpperFlatCase` and returns `usize`. |
| `toLowerWords` | `(src: string, out: []u8): usize` | Invokes `toLowerWords` and returns `usize`. |
| `toUpperWords` | `(src: string, out: []u8): usize` | Invokes `toUpperWords` and returns `usize`. |
| `toAlternatingCase` | `(src: string, out: []u8): usize` | Invokes `toAlternatingCase` and returns `usize`. |
| `convertCase` | `(src: string, style: u8, out: []u8): usize` | Invokes `convertCase` and returns `usize`. |

## Minimal Example

```cp
import { toFlatCase } from "convert_case";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = toFlatCase("example", out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
