[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# anstyle

Zero-heap ANSI terminal styling model (16-color, 256-color, truecolor RGB, SGR effects) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:anstyle` ([https://github.com/rust-cli/anstyle](https://github.com/rust-cli/anstyle)) / `crates.io:console` ([https://github.com/console-rs/console](https://github.com/console-rs/console)) / `crates.io:colored` ([https://github.com/colored-rs/colored](https://github.com/colored-rs/colored))

| Metadata | Value |
| --- | --- |
| Package | `anstyle` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add anstyle
```

```cp
import {
    Color,
    Effects,
    Style,
    writeByte,
    writeU8Dec,
    writeParam,
} from "anstyle";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Color` | `class` | Fields: `kind: u8, r: u8, g: u8, b: u8` · Methods: `none, ansi, ansi256, rgb` |
| `Effects` | `class` | Fields: `bits: u16` · Methods: `new, isEmpty` |
| `Style` | `class` | Fields: `fg: Color, bg: Color, effects: Effects` · Methods: `new, isPlain, fgColor, bgColor, bold, dimmed, +9 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `writeByte` | `(out: []u8, pos: usize, b: u8): usize` | Invokes `writeByte` and returns `usize`. |
| `writeU8Dec` | `(out: []u8, pos: usize, val: u8): usize` | Invokes `writeU8Dec` and returns `usize`. |
| `writeParam` | `(out: []u8, pos: usize, has_prev: bool, val: u8): usize` | Invokes `writeParam` and returns `usize`. |

## Minimal Example

```cp
import { writeByte } from "anstyle";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = writeByte(out_buf[0..256], 0, 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
