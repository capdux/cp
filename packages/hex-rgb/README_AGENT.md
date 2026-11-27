# hex-rgb — AI Agent Quick Ref

Zero-allocation CSS hex color (#RGB, #RGBA, #RRGGBB, #RRGGBBAA) parser, formatter, Porter-Duff source-over blender, luminance, and RGB inverter for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Rgba,
    parseHexColor,
    formatHex6,
    formatHex8,
    packRgba32,
    unpackRgba32,
    invertRgb,
    isDark,
} from "hex-rgb";
```

## Core Types & Constants

```cp
class Rgba { r: u8, g: u8, b: u8, a: u8, /* +1 fields */ }
```

## API Signatures

```cp
fn parseHexColor(input: string): Rgba;
fn formatHex6(c: Rgba, uppercase: bool, out: []u8): usize;
fn formatHex8(c: Rgba, uppercase: bool, out: []u8): usize;
fn packRgba32(c: Rgba): u32;
fn unpackRgba32(v: u32): Rgba;
fn invertRgb(c: Rgba): Rgba;
fn isDark(c: Rgba): bool;
fn hcClampU8(v: f64): u8;
fn blendOver(fg: Rgba, bg: Rgba): Rgba;
```

## Minimal Usage

```cp
import { parseHexColor } from "hex-rgb";

pub fn main() {
    let result = parseHexColor("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"hex-rgb"`; install via `cap add hex-rgb`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
