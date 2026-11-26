// Type definitions for CP

export fn sqrtF64(x: f64): f64;
export fn absF64(x: f64): f64;
export fn lerpPt2(a: Pt2, b: Pt2, t: f64): Pt2;
export fn distPt2(a: Pt2, b: Pt2): f64;
export class Pt2 {
    x: f64,
    y: f64,
    fn new(x: f64, y: f64): Pt2;
}

export class QuadBezier {
    p0: Pt2,
    p1: Pt2,
    p2: Pt2,
    fn new(p0: Pt2, p1: Pt2, p2: Pt2): QuadBezier;
    fn eval(t: f64): Pt2;
    fn deriv(t: f64): Pt2;
    fn arcLength(steps: u32): f64;
}

export class CubicBezier {
    p0: Pt2,
    p1: Pt2,
    p2: Pt2,
    p3: Pt2,
    fn new(p0: Pt2, p1: Pt2, p2: Pt2, p3: Pt2): CubicBezier;
    fn eval(t: f64): Pt2;
    fn deriv(t: f64): Pt2;
    fn arcLength(steps: u32): f64;
}

export class CubicSplit {
    left: CubicBezier,
    right: CubicBezier,
}

export fn splitCubic(c: CubicBezier, t: f64): CubicSplit;
export fn evalUnitCubic(p1: f64, p2: f64, t: f64): f64;
export fn derivUnitCubic(p1: f64, p2: f64, t: f64): f64;
export fn cubicBezierEasing(x1: f64, y1: f64, x2: f64, y2: f64, x: f64): f64;
