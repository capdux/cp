[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# cssesc

Zero-allocation CSSOM-compliant CSS identifier and string escaper in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cssesc` ([https://github.com/mathiasbynens/cssesc](https://github.com/mathiasbynens/cssesc))

| Metadata | Value |
| --- | --- |
| Package | `cssesc` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add cssesc
```

```cp
import {
    CssQuoteStyle,
    CssEscOptions,
    CescWriter,
    CescUtf8Res,
    defaultCssEscOptions,
    cescDecodeUtf8,
    cescIsAsciiAlnumOrDash,
    cssescBuf,
} from "cssesc";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `CssQuoteStyle` | `Single, Double` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CssEscOptions` | `class` | Fields: `is_identifier: bool, quotes: CssQuoteStyle, wrap: bool, escape_everything: bool` |
| `CescWriter` | `class` | Fields: `buf: []u8, pos: usize, overflow: bool` · Methods: `writeByte, writeHexEscape` |
| `CescUtf8Res` | `class` | Fields: `cp: u32, next: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `defaultCssEscOptions` | `(): CssEscOptions` | Invokes `defaultCssEscOptions` and returns `CssEscOptions`. |
| `cescDecodeUtf8` | `(s: string, start: usize): CescUtf8Res` | Invokes `cescDecodeUtf8` and returns `CescUtf8Res`. |
| `cescIsAsciiAlnumOrDash` | `(cp: u32): bool` | Invokes `cescIsAsciiAlnumOrDash` and returns `bool`. |
| `cssescBuf` | `(input: string, opts: CssEscOptions, out: []u8): usize` | Invokes `cssescBuf` and returns `usize`. |
| `cssesc` | `(input: string, opts: CssEscOptions): string` | Invokes `cssesc` and returns `string`. |

## Minimal Example

```cp
import { cescDecodeUtf8 } from "cssesc";

pub fn main() {
    let result = cescDecodeUtf8("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
