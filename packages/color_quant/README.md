[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# color_quant

Zero-allocation pure CP 256-color palette quantizer and nearest-color mapper (image-rs color_quant parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:color_quant` ([https://github.com/image-rs/color_quant](https://github.com/image-rs/color_quant))

| Metadata | Value |
| --- | --- |
| Package | `color_quant` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add color_quant
```

```cp
import {
    Rgb8,
    ColorPalette,
    clqtEmptyPalette,
    clqtBucketIndex,
    buildUniformPalette,
    quantizeHistogram,
    mapPixelsToPalette,
} from "color_quant";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Rgb8` | `class` | Fields: `r: u8, g: u8, b: u8` |
| `ColorPalette` | `class` | Fields: `colors: [256]Rgb8, len: usize` · Methods: `nearestIndex` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `clqtEmptyPalette` | `(): ColorPalette` | Invokes `clqtEmptyPalette` and returns `ColorPalette`. |
| `clqtBucketIndex` | `(px: Rgb8): usize` | Invokes `clqtBucketIndex` and returns `usize`. |
| `buildUniformPalette` | `(): ColorPalette` | Invokes `buildUniformPalette` and returns `ColorPalette`. |
| `quantizeHistogram` | `(pixels: &[]Rgb8, max_colors: usize): ColorPalette` | Invokes `quantizeHistogram` and returns `ColorPalette`. |
| `mapPixelsToPalette` | `(pixels: &[]Rgb8, pal: ColorPalette, out_indices: []u8): usize` | Invokes `mapPixelsToPalette` and returns `usize`. |

## Minimal Example

```cp
import { clqtEmptyPalette } from "color_quant";

pub fn main() {
    let result = clqtEmptyPalette();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
