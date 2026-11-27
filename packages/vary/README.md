[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# vary

Zero-heap RFC 9110 HTTP Vary header manipulation, deduplication, case-insensitive lookup, token validation, and wildcard collapsing.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:vary` ([https://github.com/jshttp/vary](https://github.com/jshttp/vary))

| Metadata | Value |
| --- | --- |
| Package | `vary` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add vary
```

```cp
import {
    isWs,
    isTchar,
    isValidFieldName,
    isWildcard,
    hasField,
    headerHasWildcard,
    validateFieldList,
    dstContainsToken,
} from "vary";
```

## Core API

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isWs` | `(c: u8): bool` | Checks predicate `isWs` and returns `bool`. |
| `isTchar` | `(c: u8): bool` | Checks predicate `isTchar` and returns `bool`. |
| `isValidFieldName` | `(field: &[]u8): bool` | Checks predicate `isValidFieldName` and returns `bool`. |
| `isWildcard` | `(tok: &[]u8): bool` | Checks predicate `isWildcard` and returns `bool`. |
| `hasField` | `(header: &[]u8, field: &[]u8): bool` | Checks predicate `hasField` and returns `bool`. |
| `headerHasWildcard` | `(src: &[]u8): bool` | Invokes `headerHasWildcard` and returns `bool`. |
| `validateFieldList` | `(field_list: &[]u8): bool` | Validates input with `validateFieldList` returning `bool`. |
| `dstContainsToken` | `(dst: &[]u8, written: usize, tok: &[]u8): bool` | Invokes `dstContainsToken` and returns `bool`. |
| `appendTokens` | `(src: &[]u8, dst: []u8, start_pos: usize): i64` | Invokes `appendTokens` and returns `i64`. |
| `append` | `(header: &[]u8, field: &[]u8, dst: []u8): i64` | Invokes `append` and returns `i64`. |
| `appendMany` | `(header: &[]u8, fields: &[]&[]u8, dst: []u8): i64` | Invokes `appendMany` and returns `i64`. |

## Minimal Example

```cp
import { isWs } from "vary";

pub fn main() {
    let result = isWs(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
