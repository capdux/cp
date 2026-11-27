[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# color-name

Zero-allocation 148 W3C CSS Color 4 named colors lookup and exact RGB/HSL/HSV/HEX color space conversion engine for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:color-name` ([https://github.com/colorjs/color-name](https://github.com/colorjs/color-name))（原库名 `color-name` → CP 包名 `color-name`）

| Metadata | Value |
| --- | --- |
| Package | `color-name` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add color-name
```

```cp
import {
    RgbColor,
    ColorEntry,
    cnToLower,
    cnCmpIgnoreCase,
    colorAt,
    lookupColor,
    lookupColorHex,
    reverseLookupColor,
} from "color-name";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `RgbColor` | `class` | Fields: `r: u8, g: u8, b: u8, found: bool` |
| `ColorEntry` | `class` | Fields: `name: string, r: u8, g: u8, b: u8` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cnToLower` | `(b: u8): u8` | Invokes `cnToLower` and returns `u8`. |
| `cnCmpIgnoreCase` | `(a: string, b: string): i8` | Invokes `cnCmpIgnoreCase` and returns `i8`. |
| `colorAt` | `(index: usize): ColorEntry` | Invokes `colorAt` and returns `ColorEntry`. |
| `lookupColor` | `(name: string): RgbColor` | Queries/retrieves data via `lookupColor` returning `RgbColor`. |
| `lookupColorHex` | `(name: string): i32` | Queries/retrieves data via `lookupColorHex` returning `i32`. |
| `reverseLookupColor` | `(r: u8, g: u8, b: u8): string` | Invokes `reverseLookupColor` and returns `string`. |
| `isNamedColor` | `(name: string): bool` | Checks predicate `isNamedColor` and returns `bool`. |

## Minimal Example

```cp
import { isNamedColor } from "color-name";

pub fn main() {
    let result = isNamedColor("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
