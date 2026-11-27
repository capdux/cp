[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# is-plain-obj

Zero-heap plain object metadata checker, JSON object literal validator, and prototype-pollution key guard (clean-room port of npm is-plain-obj)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:is-plain-obj` ([https://github.com/sindresorhus/is-plain-obj](https://github.com/sindresorhus/is-plain-obj))

| Metadata | Value |
| --- | --- |
| Package | `is-plain-obj` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add is-plain-obj
```

```cp
import {
    ValueMeta,
    ipoIsSpace,
    ipoSliceEq,
    ipoIsDangerousSlice,
    isPlainObjectMeta,
    isPlainJsonObject,
    isPlainObjectKey,
    hasDangerousProtoKey,
} from "is-plain-obj";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ValueMeta` | `class` | Fields: `proto_kind: u8, has_to_string_tag: bool, is_iterator: bool, is_null: bool` · Methods: `plain, nullProto` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ipoIsSpace` | `(ch: u8): bool` | Invokes `ipoIsSpace` and returns `bool`. |
| `ipoSliceEq` | `(s: string, start: usize, end: usize, target: string): bool` | Invokes `ipoSliceEq` and returns `bool`. |
| `ipoIsDangerousSlice` | `(s: string, start: usize, end: usize): bool` | Invokes `ipoIsDangerousSlice` and returns `bool`. |
| `isPlainObjectMeta` | `(m: ValueMeta): bool` | Checks predicate `isPlainObjectMeta` and returns `bool`. |
| `isPlainJsonObject` | `(json_text: string): bool` | Checks predicate `isPlainJsonObject` and returns `bool`. |
| `isPlainObjectKey` | `(key: string): bool` | Checks predicate `isPlainObjectKey` and returns `bool`. |
| `hasDangerousProtoKey` | `(json_text: string): bool` | Checks predicate `hasDangerousProtoKey` and returns `bool`. |

## Minimal Example

```cp
import { isPlainJsonObject } from "is-plain-obj";

pub fn main() {
    let result = isPlainJsonObject("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
