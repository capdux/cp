# `d3-path` — AI Agent Quick Reference

- **Package**: `d3-path` (`1.0.0`, MIT)
- **Origin**: `npm:d3-path`
- **Import**: `import { SvgPath, d3pPushStr, d3pPushU64, d3pPushNum } from "d3-path";`

## Types & Methods

```cp
class SvgPath {
    buf: [2048]u8,
    len: usize,
    has_subpath: bool,
    x0: f64, y0: f64,
    x1: f64, y1: f64,
    fn new(): SvgPath,
    fn byteLen(): usize,
    fn reset(_: auto),
    fn moveTo(_: auto, x: f64, y: f64),
    fn lineTo(_: auto, x: f64, y: f64),
    fn quadraticCurveTo(_: auto, cpx: f64, cpy: f64, x: f64, y: f64),
    fn bezierCurveTo(_: auto, cp1x: f64, cp1y: f64, cp2x: f64, cp2y: f64, x: f64, y: f64),
    fn rect(_: auto, x: f64, y: f64, w: f64, h: f64),
    fn closePath(_: auto),
    fn writeTo(out: []u8): usize,
}
```

## Low-Level Helpers

```cp
fn d3pPushStr(buf: []u8, pos: usize, s: string): usize
fn d3pPushU64(buf: []u8, pos: usize, val: u64): usize
fn d3pPushNum(buf: []u8, pos: usize, val: f64): usize
```

## Minimal Pattern

```cp
let p = SvgPath.new();
p.moveTo(0.0, 0.0);
p.lineTo(10.0, 20.5);
p.closePath();
let out: [128]u8 = undefined;
let n = p.writeTo(out[0..128]);
let d = out[0..n] as string;
```

## Pitfalls

1. **Buffer cap (2048 bytes)**: `SvgPath.buf` is fixed at 2048 bytes; check `p.byteLen()` for very complex paths and reuse with `p.reset()`.
2. **Reading output**: Either call `p.writeTo(out)` or slice `p.buf[0..p.byteLen()] as string`.
3. **3-decimal precision**: `d3pPushNum` rounds `f64` to 3 decimal places and trims trailing zeros.
4. **No `arc`/`arcTo` yet**: Only `moveTo`, `lineTo`, `quadraticCurveTo`, `bezierCurveTo`, `rect`, and `closePath` are exported.
