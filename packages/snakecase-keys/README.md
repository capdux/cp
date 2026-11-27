[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# snakecase-keys

Zero-heap object key converter to snake_case with exclude list and fixed key buffer pool, composing map-obj and heck-case (clean-room port of npm snakecase-keys)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:snakecase-keys` ([https://github.com/bendrucker/snakecase-keys](https://github.com/bendrucker/snakecase-keys))

| Metadata | Value |
| --- | --- |
| Package | `snakecase-keys` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add snakecase-keys
```

```cp
import {
    SnakecaseKeysResult,
    sckStrEq,
    sckIsExcluded,
    snakecaseKeyInto,
    snakecaseKeys,
} from "snakecase-keys";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SnakecaseKeysResult` | `class` | Fields: `key_buf: [1024]u8, key_buf_len: usize, key_offs: [32]usize, key_lens: [32]usize, values: [32]string, active: [32]bool, +1 more` · Methods: `empty, len, get, pushRaw, pushSnake` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `sckStrEq` | `(a: string, b: string): bool` | Invokes `sckStrEq` and returns `bool`. |
| `sckIsExcluded` | `(k: string, exclude: []string): bool` | Invokes `sckIsExcluded` and returns `bool`. |
| `snakecaseKeyInto` | `(key: string, out: []u8): usize` | Invokes `snakecaseKeyInto` and returns `usize`. |
| `snakecaseKeys` | `(src: MapObjResult, exclude: []string): SnakecaseKeysResult` | Invokes `snakecaseKeys` and returns `SnakecaseKeysResult`. |

## Minimal Example

```cp
import { sckStrEq } from "snakecase-keys";

pub fn main() {
    let result = sckStrEq("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
