# plotters-svg@1.0.0 (Agent Quickref)

> Zero-heap pure CP SVG vector graphics drawing backend (plotters-svg parity) | License: MIT
> Import: `import { ... } from "plotters-svg";` | Full types: `src/index.d.cp`

## Functions
- `export fn psvgWriteByte(buf: []u8, pos: usize, b: u8): usize;`
- `export fn psvgWriteStr(buf: []u8, pos: usize, s: string): usize;`
- `export fn psvgWriteU32(buf: []u8, pos: usize, val: u32): usize;`
- `export fn psvgWriteI32(buf: []u8, pos: usize, val: i32): usize;`
- `export fn psvgWriteRgb(buf: []u8, pos: usize, c: SvgColor): usize;`
- `export fn psvgWriteXmlEscaped(buf: []u8, pos: usize, text: string): usize;`
