[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# plist

Zero-allocation Apple XML Property List (.plist) dictionary parser and serializer in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:plist` ([https://github.com/ebarnard/rust-plist](https://github.com/ebarnard/rust-plist))

| Metadata | Value |
| --- | --- |
| Package | `plist` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add plist
```

```cp
import {
    PlistValKind,
    PlistValue,
    plstInvalid,
    plstEq,
    plstStartsAt,
    plstFindSub,
    plstSkipWs,
    plstParseI64,
} from "plist";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `PlistValKind` | `String, Integer, Real, Bool, Data, Date, Invalid` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `PlistValue` | `class` | Fields: `kind: PlistValKind, str_val: string, int_val: i64, bool_val: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `plstInvalid` | `(): PlistValue` | Invokes `plstInvalid` and returns `PlistValue`. |
| `plstEq` | `(a: string, b: string): bool` | Invokes `plstEq` and returns `bool`. |
| `plstStartsAt` | `(hay: string, pos: usize, needle: string): bool` | Invokes `plstStartsAt` and returns `bool`. |
| `plstFindSub` | `(hay: string, start: usize, needle: string): usize?` | Invokes `plstFindSub` and returns `usize?`. |
| `plstSkipWs` | `(s: string, start: usize): usize` | Invokes `plstSkipWs` and returns `usize`. |
| `plstParseI64` | `(s: string): i64` | Invokes `plstParseI64` and returns `i64`. |
| `plstSkipBalanced` | `(xml: string, pos: usize, open_tag: string, close_tag: string): usize` | Invokes `plstSkipBalanced` and returns `usize`. |
| `plstSkipElement` | `(xml: string, pos: usize): usize` | Invokes `plstSkipElement` and returns `usize`. |
| `plstParseValueAt` | `(xml: string, pos: usize): PlistValue` | Invokes `plstParseValueAt` and returns `PlistValue`. |
| `plistGetDictValue` | `(xml: string, key_name: string): PlistValue` | Invokes `plistGetDictValue` and returns `PlistValue`. |
| `plstWriteBytes` | `(dst: []u8, pos: usize, src: string): usize` | Invokes `plstWriteBytes` and returns `usize`. |
| `plstFormatI64` | `(val: i64, buf: []u8): usize` | Invokes `plstFormatI64` and returns `usize`. |
| `plistWriteDictEntry` | `(key: string, val: PlistValue, out: []u8): usize` | Invokes `plistWriteDictEntry` and returns `usize`. |

## Minimal Example

```cp
import { plstEq } from "plist";

pub fn main() {
    let result = plstEq("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
