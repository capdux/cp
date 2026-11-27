[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# wcag-contrast

Zero-allocation W3C WCAG 2.1/2.2 color contrast ratio calculator, AA/AAA compliance grader, and accessible foreground color recommender (wcag-contrast / Rust contrast parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:wcag-contrast` ([https://github.com/tmcw/wcag-contrast](https://github.com/tmcw/wcag-contrast))（原库名 `wcag-contrast` → CP 包名 `wcag-contrast`）

| Metadata | Value |
| --- | --- |
| Package | `wcag-contrast` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add wcag-contrast
```

```cp
import {
    WcagLevel,
    Rgb,
    hexNibble,
    nibbleToHexLower,
    parseHex,
    formatHex,
    linearizeChannel,
    relativeLuminance,
} from "wcag-contrast";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `WcagLevel` | `Fail, AALarge, AA, AAA` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Rgb` | `class` | Fields: `r: u8, g: u8, b: u8` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `hexNibble` | `(c: u8): i8` | Invokes `hexNibble` and returns `i8`. |
| `nibbleToHexLower` | `(n: u8): u8` | Invokes `nibbleToHexLower` and returns `u8`. |
| `parseHex` | `(hex: string): Rgb?` | Parses input via `parseHex` and returns `Rgb?`. |
| `formatHex` | `(c: Rgb, out: []u8): string` | Serializes/formats data via `formatHex` returning `string`. |
| `linearizeChannel` | `(channel: u8): f64` | Invokes `linearizeChannel` and returns `f64`. |
| `relativeLuminance` | `(c: Rgb): f64` | Invokes `relativeLuminance` and returns `f64`. |
| `contrastRatio` | `(fg: Rgb, bg: Rgb): f64` | Invokes `contrastRatio` and returns `f64`. |
| `contrastRatioHex` | `(fg_hex: string, bg_hex: string): f64?` | Invokes `contrastRatioHex` and returns `f64?`. |
| `meetsAA` | `(ratio: f64, large_text: bool): bool` | Invokes `meetsAA` and returns `bool`. |
| `meetsAAA` | `(ratio: f64, large_text: bool): bool` | Invokes `meetsAAA` and returns `bool`. |
| `gradeContrast` | `(ratio: f64): WcagLevel` | Invokes `gradeContrast` and returns `WcagLevel`. |
| `bestTextColor` | `(bg: Rgb): Rgb` | Invokes `bestTextColor` and returns `Rgb`. |
| `suggestAccessibleHex` | `(bg_hex: string): string` | Invokes `suggestAccessibleHex` and returns `string`. |

## Minimal Example

```cp
import { parseHex } from "wcag-contrast";

pub fn main() {
    let result = parseHex("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
