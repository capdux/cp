[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# ansi-styles

ANSI escape codes for styling strings in the terminal (clean-room CP implementation of npm ansi-styles)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`ansi-styles`](https://github.com/chalk/ansi-styles)（`MIT`）（包名 `ansi-styles`）。

| Metadata | Value |
| --- | --- |
| Package | `ansi-styles` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add ansi-styles
```

```cp
import {
    StylePair,
    asPushStr,
    asPushU8,
    wrapStyle,
    formatAnsi256,
    formatAnsi16m,
    formatRgbToAnsi16,
    formatRgbToAnsi256,
} from "ansi-styles";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `StylePair` | `class` | Fields: `open: string, close: string` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `asPushStr` | `(out: []u8, pos: usize, s: string): usize` | Invokes `asPushStr` and returns `usize`. |
| `asPushU8` | `(out: []u8, pos: usize, v: u8): usize` | Invokes `asPushU8` and returns `usize`. |
| `wrapStyle` | `(style: StylePair, text: string, out: []u8): usize` | Invokes `wrapStyle` and returns `usize`. |
| `formatAnsi256` | `(code: u8, is_bg: bool, out: []u8): usize` | Serializes/formats data via `formatAnsi256` returning `usize`. |
| `formatAnsi16m` | `(r: u8, g: u8, b: u8, is_bg: bool, out: []u8): usize` | Serializes/formats data via `formatAnsi16m` returning `usize`. |
| `formatRgbToAnsi16` | `(r: u8, g: u8, b: u8, is_bg: bool, out: []u8): usize` | Serializes/formats data via `formatRgbToAnsi16` returning `usize`. |
| `formatRgbToAnsi256` | `(r: u8, g: u8, b: u8, is_bg: bool, out: []u8): usize` | Serializes/formats data via `formatRgbToAnsi256` returning `usize`. |
| `formatHexToAnsi16m` | `(hex: string, is_bg: bool, out: []u8): usize` | Serializes/formats data via `formatHexToAnsi16m` returning `usize`. |
| `formatKeywordToAnsi16m` | `(name: string, is_bg: bool, out: []u8): usize` | Serializes/formats data via `formatKeywordToAnsi16m` returning `usize`. |

## Minimal Example

```cp
import { formatHexToAnsi16m } from "ansi-styles";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatHexToAnsi16m("example", false, out_buf[0..256]);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
