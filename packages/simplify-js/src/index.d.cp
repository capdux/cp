// Type definitions for CP

export class Pt2 {
    x: f64,
    y: f64,
}

export fn pointToSegmentDistSq(p: Pt2, a: Pt2, b: Pt2): f64;
export fn simplifyPolyline(pts: []Pt2, epsilon: f64, keep_mask: []u8, out: []Pt2): usize;
