[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# boxen

Zero-allocation CLI terminal box renderer with 6 Unicode/ASCII border styles, embedded titles, alignment, CJK double-width & ANSI escape awareness (boxen parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:boxen` ([https://github.com/sindresorhus/boxen](https://github.com/sindresorhus/boxen))

| Metadata | Value |
| --- | --- |
| Package | `boxen` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add boxen
```

```cp
import {
    BorderStyle,
    TextAlign,
    BoxOptions,
    BorderChars,
    Utf8Step,
    borderForStyle,
    decodeUtf8At,
    codepointWidth,
} from "boxen";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `enum` | `BorderStyle` | `Single, Double, Round, Bold, Classic, SingleDouble` |
| `enum` | `TextAlign` | `Left, Center, Right` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `BoxOptions` | `class` | Fields: `style: BorderStyle, text_align: TextAlign, padding_x: u8, padding_y: u8, margin_left: u8, title: string, +1 more` · Methods: `defaults` |
| `BorderChars` | `class` | Fields: `tl: string, top: string, tr: string, left: string, right: string, bl: string, +2 more` |
| `Utf8Step` | `class` | Fields: `cp: u32, width: usize` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `borderForStyle` | `(style: BorderStyle): BorderChars` | Invokes `borderForStyle` and returns `BorderChars`. |
| `decodeUtf8At` | `(s: string, i: usize): Utf8Step` | Parses input via `decodeUtf8At` and returns `Utf8Step`. |
| `codepointWidth` | `(cp: u32): usize` | Invokes `codepointWidth` and returns `usize`. |
| `displayWidth` | `(s: string): usize` | Invokes `displayWidth` and returns `usize`. |
| `pushByte` | `(out: []u8, pos: usize, b: u8): usize` | Mutates or appends state via `pushByte`. |
| `pushStr` | `(out: []u8, pos: usize, s: string): usize` | Mutates or appends state via `pushStr`. |
| `pushRepeat` | `(out: []u8, pos: usize, s: string, count: usize): usize` | Mutates or appends state via `pushRepeat`. |
| `pushSpaces` | `(out: []u8, pos: usize, count: usize): usize` | Mutates or appends state via `pushSpaces`. |
| `renderBox` | `(text: string, opts: BoxOptions, out: []u8): string` | Invokes `renderBox` and returns `string`. |
| `box` | `(text: string, out: []u8): string` | Invokes `box` and returns `string`. |
| `titledBox` | `(title: string, text: string, style: BorderStyle, out: []u8): string` | Invokes `titledBox` and returns `string`. |

## Minimal Example

```cp
import { decodeUtf8At } from "boxen";

pub fn main() {
    let result = decodeUtf8At("example", 0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
