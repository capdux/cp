// Type definitions for CP

export class Pt2 {
    x: f64,
    y: f64,
}

export fn absF64(x: f64): f64;
export fn sqrtF64(x: f64): f64;
export fn signedArea(pts: &[]Pt2): f64;
export fn area(pts: &[]Pt2): f64;
export fn isCounterClockwise(pts: &[]Pt2): bool;
export fn perimeter(pts: &[]Pt2): f64;
export fn centroid(pts: &[]Pt2): Pt2;
export fn pointOnSegment(p: Pt2, a: Pt2, b: Pt2): bool;
export fn containsPoint(pts: &[]Pt2, p: Pt2): bool;
export fn isConvex(pts: &[]Pt2): bool;
