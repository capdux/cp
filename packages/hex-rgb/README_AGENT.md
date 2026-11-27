# hex-rgb@1.0.0 (Agent Quickref)

> Zero-allocation CSS hex color (#RGB, #RGBA, #RRGGBB, #RRGGBBAA) parser, formatter, Porter-Duff source-over blender, luminance, and RGB inverter for CP. | License: MIT
> Import: `import { ... } from "hex-rgb";` | Full types: `src/index.d.cp`

## Functions
- `export fn parseHexColor(input: string): Rgba;`
- `export fn formatHex6(c: Rgba, uppercase: bool, out: []u8): usize;`
- `export fn formatHex8(c: Rgba, uppercase: bool, out: []u8): usize;`
- `export fn packRgba32(c: Rgba): u32;`
- `export fn unpackRgba32(v: u32): Rgba;`
- `export fn invertRgb(c: Rgba): Rgba;`
- `export fn isDark(c: Rgba): bool;`
- `export fn blendOver(fg: Rgba, bg: Rgba): Rgba;`
