[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# string-width

Zero-allocation visual terminal column width calculator (npm string-width v7 parity) composing ansi-strip and unicode-width for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:string-width` ([https://github.com/sindresorhus/string-width](https://github.com/sindresorhus/string-width))

| Metadata | Value |
| --- | --- |
| Package | `string-width` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add string-width
```

```cp
import {
    StringWidthOptions,
    SwDecoded,
    swDecodeUtf8,
    swIsRegionalIndicator,
    swIsSkinToneModifier,
    swIsTagChar,
    swBaseCharWidth,
    swMeasurePlain,
} from "string-width";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `StringWidthOptions` | `class` | Fields: `ambiguous_is_narrow: bool, count_ansi_escape_codes: bool` · Methods: `defaults` |
| `SwDecoded` | `class` | Fields: `cp: u32, next_pos: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `swDecodeUtf8` | `(s: string, pos: usize): SwDecoded` | Invokes `swDecodeUtf8` and returns `SwDecoded`. |
| `swIsRegionalIndicator` | `(cp: u32): bool` | Invokes `swIsRegionalIndicator` and returns `bool`. |
| `swIsSkinToneModifier` | `(cp: u32): bool` | Invokes `swIsSkinToneModifier` and returns `bool`. |
| `swIsTagChar` | `(cp: u32): bool` | Invokes `swIsTagChar` and returns `bool`. |
| `swBaseCharWidth` | `(cp: u32, ambiguous_is_narrow: bool): usize` | Invokes `swBaseCharWidth` and returns `usize`. |
| `swMeasurePlain` | `(s: string, ambiguous_is_narrow: bool): usize` | Invokes `swMeasurePlain` and returns `usize`. |
| `swMeasureStrippedChunk` | `(chunk: string, ambiguous_is_narrow: bool): usize` | Invokes `swMeasureStrippedChunk` and returns `usize`. |
| `stringWidthWithOptions` | `(input: string, opts: StringWidthOptions): usize` | Invokes `stringWidthWithOptions` and returns `usize`. |
| `stringWidth` | `(input: string): usize` | Invokes `stringWidth` and returns `usize`. |
| `maxLineWidthWithOptions` | `(multiline: string, opts: StringWidthOptions): usize` | Invokes `maxLineWidthWithOptions` and returns `usize`. |
| `maxLineWidth` | `(multiline: string): usize` | Invokes `maxLineWidth` and returns `usize`. |
| `swCopyBytes` | `(dst: []u8, pos: usize, src: string): usize` | Invokes `swCopyBytes` and returns `usize`. |
| `swFillBytes` | `(dst: []u8, pos: usize, ch: u8, count: usize): usize` | Invokes `swFillBytes` and returns `usize`. |
| `padEndVisible` | `(input: string, target_cols: usize, fill_byte: u8, out: []u8): usize` | Invokes `padEndVisible` and returns `usize`. |
| `padStartVisible` | `(input: string, target_cols: usize, fill_byte: u8, out: []u8): usize` | Invokes `padStartVisible` and returns `usize`. |

## Minimal Example

```cp
import { swDecodeUtf8 } from "string-width";

pub fn main() {
    let result = swDecodeUtf8("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
