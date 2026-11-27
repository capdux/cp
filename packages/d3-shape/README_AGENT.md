# d3-shape@3.2.0 (Agent Quickref)

> Pure CP SVG shape and pie chart path generator (line, step, arc, pie) | License: ISC
> Import: `import { ... } from "d3-shape";` | Full types: `src/index.d.cp`

## Functions
- `export fn d3shWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn d3shWriteStr(out: []u8, pos: usize, s: string): usize;`
- `export fn d3shWriteU64(out: []u8, pos: usize, val: u64): usize;`
- `export fn d3shWriteFloat(out: []u8, pos: usize, val: f64): usize;`
- `export fn d3shWritePair(out: []u8, pos: usize, x: f64, y: f64): usize;`
- `export fn computePie(values: &[]f64, start_angle: f64, end_angle: f64, pad_angle: f64, out: []PieSlice): usize;`
- `export fn linePath(pts: &[]Point2D, out: []u8): usize;`
- `export fn stepLinePath(pts: &[]Point2D, out: []u8): usize;`
- `export fn arcPath(inner_r: f64, outer_r: f64, start_angle: f64, end_angle: f64, out: []u8): usize;`
