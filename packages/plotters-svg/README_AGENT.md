# plotters-svg — AI Agent Quick Ref

Zero-heap pure CP SVG vector graphics drawing backend (plotters-svg parity) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    SvgColor,
    SvgCanvas,
    psvgWriteByte,
    psvgWriteStr,
    psvgWriteU32,
    psvgWriteI32,
    psvgWriteRgb,
    psvgWriteXmlEscaped,
} from "plotters-svg";
```

## Core Types & Constants

```cp
class SvgColor { r: u8, g: u8, b: u8 } // methods: rgb(...) +5
class SvgCanvas { buf: []u8, pos: usize, overflowed: bool } // methods: new(...) +10
```

## API Signatures

```cp
fn psvgWriteByte(buf: []u8, pos: usize, b: u8): usize;
fn psvgWriteStr(buf: []u8, pos: usize, s: string): usize;
fn psvgWriteU32(buf: []u8, pos: usize, val: u32): usize;
fn psvgWriteI32(buf: []u8, pos: usize, val: i32): usize;
fn psvgWriteRgb(buf: []u8, pos: usize, c: SvgColor): usize;
fn psvgWriteXmlEscaped(buf: []u8, pos: usize, text: string): usize;
```

## Minimal Usage

```cp
import { psvgWriteStr } from "plotters-svg";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = psvgWriteStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"plotters-svg"`; install via `cap add plotters-svg`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
