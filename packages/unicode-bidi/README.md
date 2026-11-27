[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# unicode-bidi

Zero-allocation Unicode Bidirectional Algorithm (UAX #9) classifier, base direction resolver, visual run splitter, and bracket mirroring for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unicode-bidi` ([https://github.com/servo/unicode-bidi](https://github.com/servo/unicode-bidi)) / `npm:bidi-js` ([https://github.com/lojjic/bidi-js](https://github.com/lojjic/bidi-js))

| Metadata | Value |
| --- | --- |
| Package | `unicode-bidi` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add unicode-bidi
```

```cp
import {
    BidiRun,
    Utf8Step,
    decodeUtf8At,
    bidiClass,
    baseDirection,
    hasRtl,
    isLtrOnly,
    resolveCharLevel,
} from "unicode-bidi";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `BidiRun` | `class` | Fields: `start: usize, len: usize, level: u8` |
| `Utf8Step` | `class` | Fields: `cp: u32, len: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `decodeUtf8At` | `(text: string, pos: usize): Utf8Step` | Parses input via `decodeUtf8At` and returns `Utf8Step`. |
| `bidiClass` | `(cp: u32): u8` | Invokes `bidiClass` and returns `u8`. |
| `baseDirection` | `(text: string): u8` | Invokes `baseDirection` and returns `u8`. |
| `hasRtl` | `(text: string): bool` | Checks predicate `hasRtl` and returns `bool`. |
| `isLtrOnly` | `(text: string): bool` | Checks predicate `isLtrOnly` and returns `bool`. |
| `resolveCharLevel` | `(cls: u8, base_level: u8, prev_level: u8): u8` | Invokes `resolveCharLevel` and returns `u8`. |
| `computeVisualRuns` | `(text: string, out_runs: []BidiRun): usize` | Invokes `computeVisualRuns` and returns `usize`. |
| `mirrorAsciiBracket` | `(ch: u8): u8` | Invokes `mirrorAsciiBracket` and returns `u8`. |

## Minimal Example

```cp
import { decodeUtf8At } from "unicode-bidi";

pub fn main() {
    let result = decodeUtf8At("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
