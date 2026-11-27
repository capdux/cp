[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# chardet

Zero-allocation character encoding detector (BOM, UTF-8, UTF-16/32, ASCII, Shift-JIS, GBK, Windows-1252, ISO-8859-1)

| Metadata | Value |
| --- | --- |
| Package | `chardet` |
| Version | `2.1.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add chardet
```

```cp
import {
    DetectedCharset,
    CharsetMatch,
    charsetName,
    chdScoreShiftJis,
    chdScoreGbk,
    detect,
    detectName,
} from "chardet";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `DetectedCharset` | `Utf8, Utf8Bom, Utf16Le, Utf16Be, Utf32Le, Utf32Be, Ascii, Windows1252 ...` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CharsetMatch` | `class` | Fields: `charset: DetectedCharset, name: string, confidence: u8` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `charsetName` | `(cs: DetectedCharset): string` | Invokes `charsetName` and returns `string`. |
| `chdScoreShiftJis` | `(data: &[]u8): u8` | Invokes `chdScoreShiftJis` and returns `u8`. |
| `chdScoreGbk` | `(data: &[]u8): u8` | Invokes `chdScoreGbk` and returns `u8`. |
| `detect` | `(data: &[]u8): CharsetMatch` | Invokes `detect` and returns `CharsetMatch`. |
| `detectName` | `(data: &[]u8): string` | Invokes `detectName` and returns `string`. |

## Minimal Example

```cp
import { charsetName } from "chardet";

pub fn main() {
    let result = charsetName(/* cs: DetectedCharset */);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
