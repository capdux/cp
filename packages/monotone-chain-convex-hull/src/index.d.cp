// Type definitions for CP

export class Pt2 {
    x: f64,
    y: f64,
}

export fn absF64(x: f64): f64;
export fn sqrtF64(x: f64): f64;
export fn ptLess(a: Pt2, b: Pt2): bool;
export fn ptEqual(a: Pt2, b: Pt2): bool;
export fn crossOAB(o: Pt2, a: Pt2, b: Pt2): f64;
export fn sortPointsXY(pts: []Pt2);
export fn convexHull(work_pts: []Pt2, out_hull: []Pt2): usize;
export fn hullArea(hull: &[]Pt2): f64;
export fn hullPerimeter(hull: &[]Pt2): f64;
