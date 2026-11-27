# color-parse@1.0.0 (Agent Quickref)

> Zero-allocation CSS Color Level 4 functional color parser (rgb/rgba/hsl/hsla with comma and space/slash syntax), formatter, and RGB <-> HSL converter for CP. | License: MIT
> Import: `import { ... } from "color-parse";` | Full types: `src/index.d.cp`

## Functions
- `export fn hslToRgb(h: f64, s: f64, l: f64, a: f64): CssRgba;`
- `export fn rgbToHsl(r: u8, g: u8, b: u8, a: f64): CssHsla;`
- `export fn parseCssColor(input: string): CssRgba;`
- `export fn formatRgb(c: CssRgba, out: []u8): usize;`
