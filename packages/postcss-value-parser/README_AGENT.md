# postcss-value-parser — AI Agent Quick Ref

Zero-allocation CSS dimension, length, angle, time, and resolution parser and unit converter for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    CssUnitKind,
    CssDimension,
    CssContext,
    invalidDim,
    parseUnit,
    isValidUnitToken,
    parseCssDimension,
    toPixels,
} from "postcss-value-parser";
```

## Core Types & Constants

```cp
enum CssUnitKind { Px, Cm, Mm, In, Pt, Pc, ... }
class CssDimension { valid: bool, value: f64, unit: CssUnitKind, unit_str: string }
class CssContext { root_font_px: f64, parent_font_px: f64, viewport_w: f64, viewport_h: f64 } // methods: defaultCtx(...)
```

## API Signatures

```cp
fn invalidDim(): CssDimension;
fn parseUnit(u: string): CssUnitKind;
fn isValidUnitToken(u: string): bool;
fn parseCssDimension(raw: string): CssDimension;
fn toPixels(dim: CssDimension, ctx: CssContext): f64?;
fn toDegrees(dim: CssDimension): f64?;
fn toMilliseconds(dim: CssDimension): f64?;
fn unitCategory(u: CssUnitKind): string;
```

## Minimal Usage

```cp
import { parseUnit } from "postcss-value-parser";

pub fn main() {
    let result = parseUnit("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"postcss-value-parser"`; install via `cap add postcss-value-parser`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
