// Type definitions for CP

export const SIMPLIFY_MAX_STACK_PTS: usize;

export class Pt2 {
    x: f64,
    y: f64,
}

export type Point2D = Pt2;

export class Point3D {
    x: f64,
    y: f64,
    z: f64,
}

export fn getSqDist(p1: Pt2, p2: Pt2): f64;
export fn pointToSegmentDistSq(p: Pt2, a: Pt2, b: Pt2): f64;
export fn getSqSegDist(p: Pt2, p1: Pt2, p2: Pt2): f64;
export fn getSqDist3D(p1: Point3D, p2: Point3D): f64;
export fn getSqSegDist3D(p: Point3D, a: Point3D, b: Point3D): f64;
export fn simplifyRadialDist(points: []const Pt2, sqTolerance: f64, out: []Pt2): usize;
export fn simplifyPolyline(pts: []Pt2, epsilon: f64, keep_mask: []u8, out: []Pt2): usize;
export fn simplifyDouglasPeucker(points: []const Pt2, sqTolerance: f64, out: []Pt2): usize;
export fn simplify(points: []const Pt2, tolerance: f64, highQuality: bool, out: []Pt2): usize;
export fn simplifyRadialDist3D(points: []const Point3D, sqTolerance: f64, out: []Point3D): usize;
export fn simplifyDouglasPeucker3D(points: []const Point3D, sqTolerance: f64, out: []Point3D): usize;
export fn simplify3D(points: []const Point3D, tolerance: f64, highQuality: bool, out: []Point3D): usize;
