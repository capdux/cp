# d3-shape — AI Agent Quick Ref

Pure CP SVG shape and pie chart path generator (line, step, arc, pie) (`v3.2.0`, `ISC`)

## Import

```cp
import {
    Point2D,
    PieSlice,
    d3shWriteByte,
    d3shWriteStr,
    d3shWriteU64,
    d3shWriteFloat,
    d3shWritePair,
    computePie,
} from "d3-shape";
```

## Core Types & Constants

```cp
class Point2D { x: f64, y: f64 }
class PieSlice { value: f64, index: usize, start_angle: f64, end_angle: f64, /* +1 fields */ }
```

## API Signatures

```cp
fn d3shWriteByte(out: []u8, pos: usize, b: u8): usize;
fn d3shWriteStr(out: []u8, pos: usize, s: string): usize;
fn d3shWriteU64(out: []u8, pos: usize, val: u64): usize;
fn d3shWriteFloat(out: []u8, pos: usize, val: f64): usize;
fn d3shWritePair(out: []u8, pos: usize, x: f64, y: f64): usize;
fn computePie(values: &[]f64, start_angle: f64, end_angle: f64, pad_angle: f64, out: []PieSlice): usize;
fn linePath(pts: &[]Point2D, out: []u8): usize;
fn stepLinePath(pts: &[]Point2D, out: []u8): usize;
fn arcPath(inner_r: f64, outer_r: f64, start_angle: f64, end_angle: f64, out: []u8): usize;
```

## Minimal Usage

```cp
import { d3shWriteStr } from "d3-shape";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = d3shWriteStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"d3-shape"`; install via `cap add d3-shape`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
