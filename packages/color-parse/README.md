[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# color-parse

Zero-allocation CSS Color Level 4 functional color parser (rgb/rgba/hsl/hsla with comma and space/slash syntax), formatter, and RGB <-> HSL converter for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:color-parse` ([https://github.com/colorjs/color-parse](https://github.com/colorjs/color-parse)) / `crates.io:csscolorparser` ([https://github.com/mazznoer/csscolorparser-rs](https://github.com/mazznoer/csscolorparser-rs))（原库名 `color-parse` → CP 包名 `color-parse`）

| Metadata | Value |
| --- | --- |
| Package | `color-parse` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add color-parse
```

```cp
import {
    CssRgba,
    CssHsla,
    ccAbsF64,
    ccClampF64,
    ccClampU8,
    ccNormHue,
    hslToRgb,
    rgbToHsl,
} from "color-parse";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CssRgba` | `class` | Fields: `r: u8, g: u8, b: u8, a: f64, valid: bool` |
| `CssHsla` | `class` | Fields: `h: f64, s: f64, l: f64, a: f64, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ccAbsF64` | `(x: f64): f64` | Invokes `ccAbsF64` and returns `f64`. |
| `ccClampF64` | `(x: f64, lo: f64, hi: f64): f64` | Invokes `ccClampF64` and returns `f64`. |
| `ccClampU8` | `(v: f64): u8` | Invokes `ccClampU8` and returns `u8`. |
| `ccNormHue` | `(h: f64): f64` | Invokes `ccNormHue` and returns `f64`. |
| `hslToRgb` | `(h: f64, s: f64, l: f64, a: f64): CssRgba` | Invokes `hslToRgb` and returns `CssRgba`. |
| `rgbToHsl` | `(r: u8, g: u8, b: u8, a: f64): CssHsla` | Invokes `rgbToHsl` and returns `CssHsla`. |
| `ccParseNum` | `(raw: &[]u8, out_val: &mut f64, is_pct: &mut bool, allow_deg: bool): bool` | Invokes `ccParseNum` and returns `bool`. |
| `ccSplitArgs` | `(body: &[]u8, out_toks: []&[]u8): usize` | Invokes `ccSplitArgs` and returns `usize`. |
| `parseCssColor` | `(input: string): CssRgba` | Parses input via `parseCssColor` and returns `CssRgba`. |
| `ccWriteRaw` | `(out: []u8, pos: &mut usize, src: &[]u8): bool` | Invokes `ccWriteRaw` and returns `bool`. |
| `ccWriteU8` | `(out: []u8, pos: &mut usize, val: u8): bool` | Invokes `ccWriteU8` and returns `bool`. |
| `ccWriteAlpha` | `(out: []u8, pos: &mut usize, a: f64): bool` | Invokes `ccWriteAlpha` and returns `bool`. |
| `formatRgb` | `(c: CssRgba, out: []u8): usize` | Serializes/formats data via `formatRgb` returning `usize`. |

## Minimal Example

```cp
import { parseCssColor } from "color-parse";

pub fn main() {
    let result = parseCssColor("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
