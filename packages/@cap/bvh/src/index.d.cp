// Type definitions for CP

export fn minF64(a: f64, b: f64): f64;
export fn maxF64(a: f64, b: f64): f64;
export class Aabb {
    min_x: f64,
    min_y: f64,
    min_z: f64,
    max_x: f64,
    max_y: f64,
    max_z: f64,
    fn merge(other: Aabb): Aabb;
    fn overlaps(other: Aabb): bool;
    fn containsPoint(x: f64, y: f64, z: f64): bool;
    fn surfaceArea(): f64;
    fn rayHit(ox: f64, oy: f64, oz: f64, dx: f64, dy: f64, dz: f64, max_t: f64): f64;
}

export class BvhItem {
    bounds: Aabb,
    id: u32,
}

export class RayHit {
    id: u32,
    t: f64,
    hit: bool,
}

export class BvhNode {
    bounds: Aabb,
    left: i32,
    right: i32,
    item_id: u32,
    is_leaf: bool,
}

export class BuildTask {
    lo: usize,
    hi: usize,
    slot: usize,
}

export class RayFrame {
    idx: i32,
    t_enter: f64,
}

export fn centroidOnAxis(b: Aabb, axis: u8): f64;
export fn sortItemsByAxis(items: []BvhItem, lo: usize, hi: usize, axis: u8);
export class Bvh {
    nodes: [256]BvhNode,
    node_count: usize,
    item_count: usize,
    fn build(items: &[]BvhItem): Bvh;
    fn len(): usize;
    fn isEmpty(): bool;
    fn queryAabb(query: Aabb, out_ids: []u32): usize;
    fn raycast(ox: f64, oy: f64, oz: f64, dx: f64, dy: f64, dz: f64, max_t: f64): RayHit;
}

