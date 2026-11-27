[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# bstr

Zero-allocation byte string utilities (Rust bstr v1 parity) powered by memchr SWAR search, line splitting, lossy UTF-8 decoding, and ASCII transforms

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bstr` ([https://github.com/BurntSushi/bstr](https://github.com/BurntSushi/bstr))

| Metadata | Value |
| --- | --- |
| Package | `bstr` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add bstr
```

```cp
import {
    SliceRange,
    bsIsContByte,
    bsUtf8ValidLen,
    bsUtf8InvalidSkip,
    bstrFind,
    bstrRfind,
    bstrFindByte,
    bstrRfindByte,
} from "bstr";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SliceRange` | `class` | Fields: `start: usize, end: usize` · Methods: `len, isEmpty` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `bsIsContByte` | `(b: u8): bool` | Invokes `bsIsContByte` and returns `bool`. |
| `bsUtf8ValidLen` | `(src: &[]u8, i: usize): usize` | Invokes `bsUtf8ValidLen` and returns `usize`. |
| `bsUtf8InvalidSkip` | `(src: &[]u8, i: usize): usize` | Invokes `bsUtf8InvalidSkip` and returns `usize`. |
| `bstrFind` | `(haystack: &[]u8, needle: &[]u8): i64` | Invokes `bstrFind` and returns `i64`. |
| `bstrRfind` | `(haystack: &[]u8, needle: &[]u8): i64` | Invokes `bstrRfind` and returns `i64`. |
| `bstrFindByte` | `(haystack: &[]u8, b: u8): i64` | Invokes `bstrFindByte` and returns `i64`. |
| `bstrRfindByte` | `(haystack: &[]u8, b: u8): i64` | Invokes `bstrRfindByte` and returns `i64`. |
| `bstrContains` | `(haystack: &[]u8, needle: &[]u8): bool` | Invokes `bstrContains` and returns `bool`. |
| `bstrStartsWith` | `(s: &[]u8, prefix: &[]u8): bool` | Invokes `bstrStartsWith` and returns `bool`. |
| `bstrEndsWith` | `(s: &[]u8, suffix: &[]u8): bool` | Invokes `bstrEndsWith` and returns `bool`. |
| `bstrTrimStart` | `(s: &[]u8): SliceRange` | Invokes `bstrTrimStart` and returns `SliceRange`. |
| `bstrTrimEnd` | `(s: &[]u8): SliceRange` | Invokes `bstrTrimEnd` and returns `SliceRange`. |
| `bstrTrim` | `(s: &[]u8): SliceRange` | Invokes `bstrTrim` and returns `SliceRange`. |
| `bstrLineCount` | `(s: &[]u8): usize` | Invokes `bstrLineCount` and returns `usize`. |
| `bstrLineAt` | `(s: &[]u8, idx: usize): SliceRange` | Invokes `bstrLineAt` and returns `SliceRange`. |
| `bstrReplaceAll` | `(src: &[]u8, pattern: &[]u8, replacement: &[]u8, out: []u8): usize` | Invokes `bstrReplaceAll` and returns `usize`. |
| `bstrIsUtf8` | `(src: &[]u8): bool` | Invokes `bstrIsUtf8` and returns `bool`. |
| `bstrToUtf8Lossy` | `(src: &[]u8, out: []u8): usize` | Invokes `bstrToUtf8Lossy` and returns `usize`. |
| `bstrIsAscii` | `(s: &[]u8): bool` | Invokes `bstrIsAscii` and returns `bool`. |
| `bstrToLowerAscii` | `(src: &[]u8, out: []u8): usize` | Invokes `bstrToLowerAscii` and returns `usize`. |
| `bstrToUpperAscii` | `(src: &[]u8, out: []u8): usize` | Invokes `bstrToUpperAscii` and returns `usize`. |
| `bstrEqIgnoreAsciiCase` | `(a: &[]u8, b: &[]u8): bool` | Invokes `bstrEqIgnoreAsciiCase` and returns `bool`. |

## Minimal Example

```cp
import { bsIsContByte } from "bstr";

pub fn main() {
    let result = bsIsContByte(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
