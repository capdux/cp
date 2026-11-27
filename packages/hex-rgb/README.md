[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# hex-rgb

Zero-allocation CSS hex color (#RGB, #RGBA, #RRGGBB, #RRGGBBAA) parser, formatter, Porter-Duff source-over blender, luminance, and RGB inverter for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:hex-rgb` ([https://github.com/sindresorhus/hex-rgb](https://github.com/sindresorhus/hex-rgb)) / `npm:colord` ([https://github.com/omgovich/colord](https://github.com/omgovich/colord))（原库名 `hex-rgb` → CP 包名 `hex-rgb`）

| Metadata | Value |
| --- | --- |
| Package | `hex-rgb` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add hex-rgb
```

```cp
import {
    Rgba,
    parseHexColor,
    formatHex6,
    formatHex8,
    packRgba32,
    unpackRgba32,
    invertRgb,
    isDark,
} from "hex-rgb";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Rgba` | `class` | Fields: `r: u8, g: u8, b: u8, a: u8, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `parseHexColor` | `(input: string): Rgba` | Parses input via `parseHexColor` and returns `Rgba`. |
| `formatHex6` | `(c: Rgba, uppercase: bool, out: []u8): usize` | Serializes/formats data via `formatHex6` returning `usize`. |
| `formatHex8` | `(c: Rgba, uppercase: bool, out: []u8): usize` | Serializes/formats data via `formatHex8` returning `usize`. |
| `packRgba32` | `(c: Rgba): u32` | Invokes `packRgba32` and returns `u32`. |
| `unpackRgba32` | `(v: u32): Rgba` | Invokes `unpackRgba32` and returns `Rgba`. |
| `invertRgb` | `(c: Rgba): Rgba` | Invokes `invertRgb` and returns `Rgba`. |
| `isDark` | `(c: Rgba): bool` | Checks predicate `isDark` and returns `bool`. |
| `hcClampU8` | `(v: f64): u8` | Invokes `hcClampU8` and returns `u8`. |
| `blendOver` | `(fg: Rgba, bg: Rgba): Rgba` | Invokes `blendOver` and returns `Rgba`. |

## Minimal Example

```cp
import { parseHexColor } from "hex-rgb";

pub fn main() {
    let result = parseHexColor("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
