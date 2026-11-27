# bezier-easing@1.0.0 (Agent Quickref)

> Zero-allocation quadratic/cubic Bezier curves, de Casteljau splitting, arc length, and CSS cubic-bezier easing for CP | License: MIT
> Import: `import { ... } from "bezier-easing";` | Full types: `src/index.d.cp`

## Functions
- `export fn sqrtF64(x: f64): f64;`
- `export fn absF64(x: f64): f64;`
- `export fn lerpPt2(a: Pt2, b: Pt2, t: f64): Pt2;`
- `export fn distPt2(a: Pt2, b: Pt2): f64;`
- `export fn splitCubic(c: CubicBezier, t: f64): CubicSplit;`
- `export fn evalUnitCubic(p1: f64, p2: f64, t: f64): f64;`
- `export fn derivUnitCubic(p1: f64, p2: f64, t: f64): f64;`
- `export fn cubicBezierEasing(x1: f64, y1: f64, x2: f64, y2: f64, x: f64): f64;`
