# color_quant@1.0.0 (Agent Quickref)

> Zero-allocation pure CP 256-color palette quantizer and nearest-color mapper (image-rs color_quant parity) | License: MIT
> Import: `import { ... } from "color_quant";` | Full types: `src/index.d.cp`

## Functions
- `export fn clqtEmptyPalette(): ColorPalette;`
- `export fn clqtBucketIndex(px: Rgb8): usize;`
- `export fn buildUniformPalette(): ColorPalette;`
- `export fn quantizeHistogram(pixels: &[]Rgb8, max_colors: usize): ColorPalette;`
- `export fn mapPixelsToPalette(pixels: &[]Rgb8, pal: ColorPalette, out_indices: []u8): usize;`
