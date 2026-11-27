# color-parse — AI Agent Quick Ref

Zero-allocation CSS Color Level 4 functional color parser (rgb/rgba/hsl/hsla with comma and space/slash syntax), formatter, and RGB <-> HSL converter for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    CssRgba,
    CssHsla,
    ccAbsF64,
    ccClampF64,
    ccClampU8,
    ccNormHue,
    hslToRgb,
    rgbToHsl,
} from "color-parse";
```

## Core Types & Constants

```cp
class CssRgba { r: u8, g: u8, b: u8, a: f64, /* +1 fields */ }
class CssHsla { h: f64, s: f64, l: f64, a: f64, /* +1 fields */ }
```

## API Signatures

```cp
fn ccAbsF64(x: f64): f64;
fn ccClampF64(x: f64, lo: f64, hi: f64): f64;
fn ccClampU8(v: f64): u8;
fn ccNormHue(h: f64): f64;
fn hslToRgb(h: f64, s: f64, l: f64, a: f64): CssRgba;
fn rgbToHsl(r: u8, g: u8, b: u8, a: f64): CssHsla;
fn ccParseNum(raw: &[]u8, out_val: &mut f64, is_pct: &mut bool, allow_deg: bool): bool;
fn ccSplitArgs(body: &[]u8, out_toks: []&[]u8): usize;
fn parseCssColor(input: string): CssRgba;
fn ccWriteRaw(out: []u8, pos: &mut usize, src: &[]u8): bool;
fn ccWriteU8(out: []u8, pos: &mut usize, val: u8): bool;
fn ccWriteAlpha(out: []u8, pos: &mut usize, a: f64): bool;
fn formatRgb(c: CssRgba, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseCssColor } from "color-parse";

pub fn main() {
    let result = parseCssColor("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"color-parse"`; install via `cap add color-parse`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
