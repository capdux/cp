# bezier-easing — AI Agent Quick Ref

Zero-allocation quadratic/cubic Bezier curves, de Casteljau splitting, arc length, and CSS cubic-bezier easing for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Pt2,
    QuadBezier,
    CubicBezier,
    CubicSplit,
    sqrtF64,
    absF64,
    lerpPt2,
    distPt2,
} from "bezier-easing";
```

## Core Types & Constants

```cp
class Pt2 { x: f64, y: f64 } // methods: new(...)
class QuadBezier { p0: Pt2, p1: Pt2, p2: Pt2 } // methods: new(...) +3
class CubicBezier { p0: Pt2, p1: Pt2, p2: Pt2, p3: Pt2 } // methods: new(...) +3
class CubicSplit { left: CubicBezier, right: CubicBezier }
```

## API Signatures

```cp
fn sqrtF64(x: f64): f64;
fn absF64(x: f64): f64;
fn lerpPt2(a: Pt2, b: Pt2, t: f64): Pt2;
fn distPt2(a: Pt2, b: Pt2): f64;
fn splitCubic(c: CubicBezier, t: f64): CubicSplit;
fn evalUnitCubic(p1: f64, p2: f64, t: f64): f64;
fn derivUnitCubic(p1: f64, p2: f64, t: f64): f64;
fn cubicBezierEasing(x1: f64, y1: f64, x2: f64, y2: f64, x: f64): f64;
```

## Minimal Usage

```cp
import { sqrtF64 } from "bezier-easing";

pub fn main() {
    let result = sqrtF64(0.0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"bezier-easing"`; install via `cap add bezier-easing`.
