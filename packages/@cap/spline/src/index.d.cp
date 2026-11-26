// Type definitions for CP

export class Vec2 {
    x: f64,
    y: f64,
}

export class CubicSpline {
    xs: [64]f64,
    a: [64]f64,
    b: [64]f64,
    c: [64]f64,
    d: [64]f64,
    count: usize,
    valid: bool,
    fn build(xs: &[]f64, ys: &[]f64): CubicSpline;
    fn isValid(): bool;
    fn knotCount(): usize;
    fn findSegment(x: f64): usize;
    fn eval(x: f64): f64;
    fn derivative(x: f64): f64;
}

export fn lerp(a: f64, b: f64, t: f64): f64;
export fn smoothstep(edge0: f64, edge1: f64, x: f64): f64;
export fn bezierQuad(p0: Vec2, p1: Vec2, p2: Vec2, t: f64): Vec2;
export fn bezierCubic(p0: Vec2, p1: Vec2, p2: Vec2, p3: Vec2, t: f64): Vec2;
export fn catmullRom(p0: f64, p1: f64, p2: f64, p3: f64, t: f64): f64;
