// Type definitions for CP

export class KdPt {
    x: f64,
    y: f64,
    id: u32,
}

export class KdNode {
    pt: KdPt,
    left: i32,
    right: i32,
    axis: u8,
}

export class NearestResult {
    found: bool,
    id: u32,
    dist_sq: f64,
}

export class KdBuildFrame {
    lo: usize,
    hi: usize,
    axis: u8,
    parent: i32,
    is_left: bool,
}

export class KdSearchFrame {
    idx: i32,
    min_dist_sq: f64,
}

export fn kdCoord(pt: KdPt, axis: u8): f64;
export fn kdSwap(pts: []KdPt, i: usize, j: usize);
export fn kdInsertionSort(pts: []KdPt, lo: usize, hi: usize, axis: u8);
export fn kdSelectMedian(pts: []KdPt, lo_in: usize, hi_in: usize, target: usize, axis: u8);
export fn buildKdTree(pts: []KdPt, nodes: []KdNode): i32;
export fn nearestNeighbor(nodes: []KdNode, root: i32, qx: f64, qy: f64): NearestResult;
export fn rangeQueryRect(nodes: []KdNode, root: i32, min_x: f64, min_y: f64, max_x: f64, max_y: f64, out_ids: []u32): usize;
export fn radiusQuery(nodes: []KdNode, root: i32, cx: f64, cy: f64, radius: f64, out_ids: []u32): usize;
