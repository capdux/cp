[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# colorette

Zero-heap pure CP terminal ANSI color formatter with nested close-code recovery (clean-room MIT implementation of npm colorette v2)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:colorette` ([https://github.com/jorgebucaran/colorette](https://github.com/jorgebucaran/colorette))

| Metadata | Value |
| --- | --- |
| Package | `colorette` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add colorette
```

```cp
import {
    ColoretteOptions,
    Colorette,
    clrFormatPair,
    isColorSupported,
    createColors,
    createColorsWithOptions,
} from "colorette";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `ColoretteOptions` | `class` | Fields: `use_color: bool` · Methods: `enabled, disabled, fromEnv` |
| `Colorette` | `class` | Fields: `options: ColoretteOptions` · Methods: `new, fromOptions, isEnabled, formatPair, reset, bold, +40 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `clrFormatPair` | `(use_color: bool, pair: StylePair, replace_with: string, text: string, out: []u8): usize` | Invokes `clrFormatPair` and returns `usize`. |
| `isColorSupported` | `(no_color: string, force_color: string, term: string, is_tty: bool): bool` | Checks predicate `isColorSupported` and returns `bool`. |
| `createColors` | `(use_color: bool): Colorette` | Constructs/initializes an instance via `createColors`. |
| `createColorsWithOptions` | `(opts: ColoretteOptions): Colorette` | Constructs/initializes an instance via `createColorsWithOptions`. |

## Minimal Example

```cp
import { isColorSupported } from "colorette";

pub fn main() {
    let result = isColorSupported("example", "example", "example", false);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
