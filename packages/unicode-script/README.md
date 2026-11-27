[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# unicode-script

Zero-heap UAX #24 Unicode Script property classifier and dominant script detector in pure CP (modeled after Rust unicode-script)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unicode-script` ([https://github.com/unicode-rs/unicode-script](https://github.com/unicode-rs/unicode-script))（原库名 `unicode-script` → CP 包名 `unicode-script`）

| Metadata | Value |
| --- | --- |
| Package | `unicode-script` |
| Version | `1.0.0` |
| License | `MIT OR Apache-2.0` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add unicode-script
```

```cp
import {
    Script,
    UscDecoded,
    uscScriptIndex,
    uscIndexToScript,
    scriptOf,
    scriptName,
    isLatin,
    isHan,
} from "unicode-script";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `Script` | `Common, Latin, Greek, Cyrillic, Han, Hiragana, Katakana, Hangul ...` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `UscDecoded` | `class` | Fields: `cp: u32, len: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `uscScriptIndex` | `(s: Script): usize` | Invokes `uscScriptIndex` and returns `usize`. |
| `uscIndexToScript` | `(idx: usize): Script` | Invokes `uscIndexToScript` and returns `Script`. |
| `scriptOf` | `(cp: u32): Script` | Invokes `scriptOf` and returns `Script`. |
| `scriptName` | `(s: Script): string` | Invokes `scriptName` and returns `string`. |
| `isLatin` | `(cp: u32): bool` | Checks predicate `isLatin` and returns `bool`. |
| `isHan` | `(cp: u32): bool` | Checks predicate `isHan` and returns `bool`. |
| `isCjkScript` | `(s: Script): bool` | Checks predicate `isCjkScript` and returns `bool`. |
| `uscDecodeUtf8` | `(text: string, pos: usize): UscDecoded` | Invokes `uscDecodeUtf8` and returns `UscDecoded`. |
| `dominantScript` | `(text: string): Script` | Invokes `dominantScript` and returns `Script`. |

## Minimal Example

```cp
import { uscDecodeUtf8 } from "unicode-script";

pub fn main() {
    let result = uscDecodeUtf8("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
