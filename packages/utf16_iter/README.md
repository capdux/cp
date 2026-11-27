[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# utf16_iter

Zero-allocation bidirectional UTF-16 code point iterator, Latin-1 to UTF-16 expander, and UTF-16 to UTF-8 transcoder in CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:utf16_iter` ([https://github.com/hsivonen/utf16_iter](https://github.com/hsivonen/utf16_iter), `Apache-2.0 OR MIT`)

| Metadata | Value |
| --- | --- |
| Package | `utf16_iter` |
| Version | `1.0.0` |
| License | `Apache-2.0 OR MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add utf16_iter
```

```cp
import {
    Utf16Chars,
    Utf16Latin1,
    u16iIsHighSurrogate,
    u16iIsLowSurrogate,
    u16iCombineSurrogates,
    u16iEncodeUtf8,
    countUtf16Chars,
    utf16ToUtf8,
} from "utf16_iter";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Utf16Chars` | `class` | Fields: `slice: &[]u16, pos: usize, end: usize` · Methods: `new, isEmpty, asSlice, next, nextBack` |
| `Utf16Latin1` | `class` | Fields: `bytes: &[]u8, pos: usize, end: usize` · Methods: `new, isEmpty, asSlice, next, nextBack, writeTo, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `u16iIsHighSurrogate` | `(unit: u16): bool` | Invokes `u16iIsHighSurrogate` and returns `bool`. |
| `u16iIsLowSurrogate` | `(unit: u16): bool` | Invokes `u16iIsLowSurrogate` and returns `bool`. |
| `u16iCombineSurrogates` | `(high: u16, low: u16): u32` | Invokes `u16iCombineSurrogates` and returns `u32`. |
| `u16iEncodeUtf8` | `(cp: u32, out: []u8, offset: usize): usize` | Invokes `u16iEncodeUtf8` and returns `usize`. |
| `countUtf16Chars` | `(slice: &[]u16): usize` | Invokes `countUtf16Chars` and returns `usize`. |
| `utf16ToUtf8` | `(slice: &[]u16, out: []u8): usize` | Invokes `utf16ToUtf8` and returns `usize`. |

## Minimal Example

```cp
import { u16iIsHighSurrogate } from "utf16_iter";

pub fn main() {
    let result = u16iIsHighSurrogate(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
