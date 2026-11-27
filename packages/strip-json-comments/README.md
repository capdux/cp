[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# strip-json-comments

Zero-allocation JSONC comment and trailing-comma stripper with line/column-preserving whitespace mode (npm strip-json-comments parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:strip-json-comments` ([https://github.com/sindresorhus/strip-json-comments](https://github.com/sindresorhus/strip-json-comments))

| Metadata | Value |
| --- | --- |
| Package | `strip-json-comments` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add strip-json-comments
```

```cp
import {
    StripOptions,
    isTrailingComma,
    stripComments,
    stripJsonc,
    stripCompact,
} from "strip-json-comments";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `StripOptions` | `class` | Fields: `preserve_whitespace: bool, strip_trailing_commas: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isTrailingComma` | `(src: &[]u8, commaIdx: usize): bool` | Checks predicate `isTrailingComma` and returns `bool`. |
| `stripComments` | `(src: &[]u8, dst: []u8, opts: StripOptions): i64` | Invokes `stripComments` and returns `i64`. |
| `stripJsonc` | `(src: &[]u8, dst: []u8): i64` | Invokes `stripJsonc` and returns `i64`. |
| `stripCompact` | `(src: &[]u8, dst: []u8): i64` | Invokes `stripCompact` and returns `i64`. |

## Minimal Example

```cp
import { isTrailingComma } from "strip-json-comments";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = isTrailingComma(out_buf[0..256], 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
