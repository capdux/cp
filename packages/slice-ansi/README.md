[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# slice-ansi

Zero-allocation ANSI-preserving terminal column slicer and truncator with CJK double-width support in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:slice-ansi` ([https://github.com/chalk/slice-ansi](https://github.com/chalk/slice-ansi))

| Metadata | Value |
| --- | --- |
| Package | `slice-ansi` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add slice-ansi
```

```cp
import {
    SaUtf8Char,
    SaStyleState,
    saDecodeUtf8,
    saWriteByte,
    saWriteBytes,
    saWriteU16,
    saWriteSgrParam,
    saEmitActiveStyle,
} from "slice-ansi";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SaUtf8Char` | `class` | Fields: `cp: u32, byte_len: usize` |
| `SaStyleState` | `class` | Fields: `bold: bool, dim: bool, italic: bool, underline: bool, blink: bool, inverse: bool, +12 more` · Methods: `empty, reset, hasAttr, hasAny, applySgr` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `saDecodeUtf8` | `(s: string, pos: usize): SaUtf8Char` | Invokes `saDecodeUtf8` and returns `SaUtf8Char`. |
| `saWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `saWriteByte` and returns `usize`. |
| `saWriteBytes` | `(out: []u8, pos: usize, src: string): usize` | Invokes `saWriteBytes` and returns `usize`. |
| `saWriteU16` | `(out: []u8, pos: usize, val: u16): usize` | Invokes `saWriteU16` and returns `usize`. |
| `saWriteSgrParam` | `(out: []u8, pos: usize, first: &mut bool, code: u16): usize` | Invokes `saWriteSgrParam` and returns `usize`. |
| `saEmitActiveStyle` | `(out: []u8, pos: usize, st: SaStyleState): usize` | Invokes `saEmitActiveStyle` and returns `usize`. |
| `saEmitCloseStyle` | `(out: []u8, pos: usize, st: SaStyleState): usize` | Invokes `saEmitCloseStyle` and returns `usize`. |
| `ansiVisibleWidth` | `(input: string): usize` | Invokes `ansiVisibleWidth` and returns `usize`. |
| `sliceAnsi` | `(input: string, begin: usize, end: usize, out: []u8): usize` | Invokes `sliceAnsi` and returns `usize`. |
| `sliceAnsiFrom` | `(input: string, begin: usize, out: []u8): usize` | Invokes `sliceAnsiFrom` and returns `usize`. |
| `truncateAnsi` | `(input: string, max_cols: usize, ellipsis: string, out: []u8): usize` | Invokes `truncateAnsi` and returns `usize`. |

## Minimal Example

```cp
import { saDecodeUtf8 } from "slice-ansi";

pub fn main() {
    let result = saDecodeUtf8("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
