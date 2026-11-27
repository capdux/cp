[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# postcss-value-parser

Zero-allocation CSS dimension, length, angle, time, and resolution parser and unit converter for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:postcss-value-parser` ([https://github.com/TrySound/postcss-value-parser](https://github.com/TrySound/postcss-value-parser))（原库名 `postcss-value-parser` → CP 包名 `postcss-value-parser`）

| Metadata | Value |
| --- | --- |
| Package | `postcss-value-parser` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add postcss-value-parser
```

```cp
import {
    CssUnitKind,
    CssDimension,
    CssContext,
    invalidDim,
    parseUnit,
    isValidUnitToken,
    parseCssDimension,
    toPixels,
} from "postcss-value-parser";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `CssUnitKind` | `Px, Cm, Mm, In, Pt, Pc, Q, Rem ...` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `CssDimension` | `class` | Fields: `valid: bool, value: f64, unit: CssUnitKind, unit_str: string` |
| `CssContext` | `class` | Fields: `root_font_px: f64, parent_font_px: f64, viewport_w: f64, viewport_h: f64` · Methods: `defaultCtx` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `invalidDim` | `(): CssDimension` | Invokes `invalidDim` and returns `CssDimension`. |
| `parseUnit` | `(u: string): CssUnitKind` | Parses input via `parseUnit` and returns `CssUnitKind`. |
| `isValidUnitToken` | `(u: string): bool` | Checks predicate `isValidUnitToken` and returns `bool`. |
| `parseCssDimension` | `(raw: string): CssDimension` | Parses input via `parseCssDimension` and returns `CssDimension`. |
| `toPixels` | `(dim: CssDimension, ctx: CssContext): f64?` | Invokes `toPixels` and returns `f64?`. |
| `toDegrees` | `(dim: CssDimension): f64?` | Invokes `toDegrees` and returns `f64?`. |
| `toMilliseconds` | `(dim: CssDimension): f64?` | Invokes `toMilliseconds` and returns `f64?`. |
| `unitCategory` | `(u: CssUnitKind): string` | Invokes `unitCategory` and returns `string`. |

## Minimal Example

```cp
import { parseUnit } from "postcss-value-parser";

pub fn main() {
    let result = parseUnit("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
