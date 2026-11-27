// Type definitions for CP

export class Rect2 {
    min_x: f64,
    min_y: f64,
    max_x: f64,
    max_y: f64,
    fn new(min_x: f64, min_y: f64, max_x: f64, max_y: f64): Rect2;
    fn contains(x: f64, y: f64): bool;
    fn intersects(other: Rect2): bool;
}

export class QuadItem {
    x: f64,
    y: f64,
    id: u32,
    next_item: i32,
}

export class QuadNode {
    bounds: Rect2,
    first_item: i32,
    count: u32,
    children: [4]i32,
    is_leaf: bool,
}

export fn qtMakeLeaf(bounds: Rect2): QuadNode;
export fn qtChildIndex(b: Rect2, x: f64, y: f64): usize;
export class QuadTree {
    node_count: usize,
    item_count: usize,
    max_leaf_items: u32,
    max_depth: u32,
    fn empty(): QuadTree;
    fn init(_: auto, nodes: []QuadNode, bounds: Rect2, max_leaf: u32, max_depth: u32): bool;
    fn insert(_: auto, nodes: []QuadNode, items: []QuadItem, x: f64, y: f64, id: u32): bool;
    fn queryRect(nodes: []QuadNode, items: []QuadItem, range: Rect2, out_ids: []u32): usize;
    fn queryRadius(nodes: []QuadNode, items: []QuadItem, cx: f64, cy: f64, radius: f64, out_ids: []u32): usize;
}

