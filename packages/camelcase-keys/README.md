[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# camelcase-keys

Zero-heap streaming JSON object key case converter (camelCase, snake_case, PascalCase) composing camelcase in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:camelcase-keys` ([https://github.com/sindresorhus/camelcase-keys](https://github.com/sindresorhus/camelcase-keys)) / `npm:decamelize-keys` ([https://github.com/sindresorhus/decamelize-keys](https://github.com/sindresorhus/decamelize-keys))

| Metadata | Value |
| --- | --- |
| Package | `camelcase-keys` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add camelcase-keys
```

```cp
import {
    ckIsSpace,
    ckWriteByte,
    ckWriteSlice,
    ckConvertJsonKeys,
    convertJsonKeysToCamel,
    convertJsonKeysToSnake,
    convertJsonKeysToPascal,
} from "camelcase-keys";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ckIsSpace` | `(ch: u8): bool` | Invokes `ckIsSpace` and returns `bool`. |
| `ckWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `ckWriteByte` and returns `usize`. |
| `ckWriteSlice` | `(out: []u8, pos: usize, src: &[]u8): usize` | Invokes `ckWriteSlice` and returns `usize`. |
| `ckConvertJsonKeys` | `(json: string, mode: u8, out: []u8): usize` | Invokes `ckConvertJsonKeys` and returns `usize`. |
| `convertJsonKeysToCamel` | `(json: string, out: []u8): usize` | Invokes `convertJsonKeysToCamel` and returns `usize`. |
| `convertJsonKeysToSnake` | `(json: string, out: []u8): usize` | Invokes `convertJsonKeysToSnake` and returns `usize`. |
| `convertJsonKeysToPascal` | `(json: string, out: []u8): usize` | Invokes `convertJsonKeysToPascal` and returns `usize`. |

## Minimal Example

```cp
import { ckConvertJsonKeys } from "camelcase-keys";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = ckConvertJsonKeys("example", 0, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
