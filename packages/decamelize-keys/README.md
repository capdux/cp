[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# decamelize-keys

Zero-heap object key decamelizer with custom separator, exclude list, and fixed key buffer pool, composing map-obj and decamelize (clean-room port of npm decamelize-keys)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:decamelize-keys` ([https://github.com/sindresorhus/decamelize-keys](https://github.com/sindresorhus/decamelize-keys))

| Metadata | Value |
| --- | --- |
| Package | `decamelize-keys` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add decamelize-keys
```

```cp
import { DecamelizeKeysResult, dckStrEq, dckIsExcluded, decamelizeKeys } from "decamelize-keys";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `DecamelizeKeysResult` | `class` | Fields: `key_buf: [1024]u8, key_buf_len: usize, key_offs: [32]usize, key_lens: [32]usize, values: [32]string, active: [32]bool, +1 more` · Methods: `empty, len, get, pushRaw, pushDecamelized` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `dckStrEq` | `(a: string, b: string): bool` | Invokes `dckStrEq` and returns `bool`. |
| `dckIsExcluded` | `(k: string, exclude: []string): bool` | Invokes `dckIsExcluded` and returns `bool`. |
| `decamelizeKeys` | `(src: MapObjResult, sep: string, exclude: []string): DecamelizeKeysResult` | Invokes `decamelizeKeys` and returns `DecamelizeKeysResult`. |

## Minimal Example

```cp
import { dckStrEq } from "decamelize-keys";

pub fn main() {
    let result = dckStrEq("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
