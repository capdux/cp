[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# badge-maker

Zero-heap Shields.io-style SVG status badge generator (flat, flat-square, plastic) with XML escaping and named colors

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:badge-maker` ([https://github.com/badges/shields](https://github.com/badges/shields))（原库名 `badge-maker` → CP 包名 `badge-maker`）

| Metadata | Value |
| --- | --- |
| Package | `badge-maker` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add badge-maker
```

```cp
import {
    BadgeStyle,
    BadgeOptions,
    BadgeWriter,
    defaultBadge,
    resolveBadgeColor,
    estimateTextWidth,
    renderBadge,
    makeBadge,
} from "badge-maker";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `BadgeStyle` | `Flat, FlatSquare, Plastic` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `BadgeOptions` | `class` | Fields: `label: string, message: string, labelColor: string, color: string, style: BadgeStyle` |
| `BadgeWriter` | `class` | Fields: `out: []u8, pos: usize` · Methods: `putStr, writeXmlEscaped, writeUsize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `defaultBadge` | `(label: string, message: string, color: string): BadgeOptions` | Invokes `defaultBadge` and returns `BadgeOptions`. |
| `resolveBadgeColor` | `(name: string): string` | Invokes `resolveBadgeColor` and returns `string`. |
| `estimateTextWidth` | `(s: string): usize` | Invokes `estimateTextWidth` and returns `usize`. |
| `renderBadge` | `(opts: BadgeOptions, out: []u8): string` | Invokes `renderBadge` and returns `string`. |
| `makeBadge` | `(label: string, message: string, color: string, out: []u8): string` | Invokes `makeBadge` and returns `string`. |

## Minimal Example

```cp
import { defaultBadge } from "badge-maker";

pub fn main() {
    let result = defaultBadge("example", "example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
