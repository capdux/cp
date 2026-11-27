# color_quant — AI Agent Quick Ref

Zero-allocation pure CP 256-color palette quantizer and nearest-color mapper (image-rs color_quant parity) (`v1.0.0`, `MIT`)

## Import

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

## Core Types & Constants

```cp
class Rgb8 { r: u8, g: u8, b: u8 }
class ColorPalette { colors: [256]Rgb8, len: usize } // methods: nearestIndex(...)
```

## API Signatures

```cp
fn clqtEmptyPalette(): ColorPalette;
fn clqtBucketIndex(px: Rgb8): usize;
fn buildUniformPalette(): ColorPalette;
fn quantizeHistogram(pixels: &[]Rgb8, max_colors: usize): ColorPalette;
fn mapPixelsToPalette(pixels: &[]Rgb8, pal: ColorPalette, out_indices: []u8): usize;
```

## Minimal Usage

```cp
import { clqtEmptyPalette } from "color_quant";

pub fn main() {
    let result = clqtEmptyPalette();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"color_quant"`; install via `cap add color_quant`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
