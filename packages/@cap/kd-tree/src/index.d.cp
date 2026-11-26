// Type definitions for CP

export class KdPoint2D {
    x: f64,
    y: f64,
    id: u32,
}

export class NearestResult {
    id: u32,
    dist_sq: f64,
    found: bool,
}

export class KdNode {
    x: f64,
    y: f64,
    id: u32,
    axis: u8,
    left: i32,
    right: i32,
}

export class BuildTask {
    lo: usize,
    hi: usize,
    depth: usize,
    slot: usize,
}

export class SearchFrame {
    idx: i32,
    plane_dist_sq: f64,
}

export fn coordAt(p: KdPoint2D, axis: u8): f64;
export fn sortRangeByAxis(pts: []KdPoint2D, lo: usize, hi: usize, axis: u8);
export class KdTree2D {
    nodes: [256]KdNode,
    node_count: usize,
    fn build(points: &[]KdPoint2D): KdTree2D;
    fn len(): usize;
    fn isEmpty(): bool;
    fn nearest(qx: f64, qy: f64): NearestResult;
    fn withinRadius(qx: f64, qy: f64, radius: f64, out_ids: []u32): usize;
}

