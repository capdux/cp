[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# querystringify

Zero-allocation WHATWG application/x-www-form-urlencoded parser, serializer, and percent-encoding codec for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:querystringify` ([https://github.com/unshiftio/querystringify](https://github.com/unshiftio/querystringify)) / `crates.io:urlencoding` ([https://github.com/kornelski/rust_urlencoding](https://github.com/kornelski/rust_urlencoding))（原库名 `querystringify` → CP 包名 `querystringify`）

| Metadata | Value |
| --- | --- |
| Package | `querystringify` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add querystringify
```

```cp
import {
    FormPair,
    FormSerializer,
    isFormSafe,
    encodedSize,
    percentEncode,
    decodedSize,
    percentDecode,
    parseFormPairs,
} from "querystringify";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `FormPair` | `class` | Fields: `key_start: usize, key_len: usize, val_start: usize, val_len: usize` |
| `FormSerializer` | `class` | Fields: `len: usize, count: usize` · Methods: `new, appendPair` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isFormSafe` | `(b: u8): bool` | Checks predicate `isFormSafe` and returns `bool`. |
| `encodedSize` | `(input: string, space_as_plus: bool): usize` | Serializes/formats data via `encodedSize` returning `usize`. |
| `percentEncode` | `(input: string, space_as_plus: bool, out: []u8): usize` | Invokes `percentEncode` and returns `usize`. |
| `decodedSize` | `(input: string): usize` | Parses input via `decodedSize` and returns `usize`. |
| `percentDecode` | `(input: string, plus_as_space: bool, out: []u8): usize` | Invokes `percentDecode` and returns `usize`. |
| `parseFormPairs` | `(query: string, out: []FormPair): usize` | Parses input via `parseFormPairs` and returns `usize`. |
| `decodedMatches` | `(raw: string, plus_as_space: bool, target: string): bool` | Parses input via `decodedMatches` and returns `bool`. |
| `getParamDecoded` | `(query: string, target_key: string, out: []u8): i64` | Queries/retrieves data via `getParamDecoded` returning `i64`. |

## Minimal Example

```cp
import { encodedSize } from "querystringify";

pub fn main() {
    let result = encodedSize("example", false);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
