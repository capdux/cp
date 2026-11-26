// Type definitions for CP

export class DisjointSet {
    parent: [256]u16,
    sizes: [256]u16,
    count: usize,
    components: usize,
    fn init(n: usize): DisjointSet;
    fn len(): usize;
    fn componentCount(): usize;
    fn find(_: auto, x: usize): i64;
    fn findReadOnly(x: usize): i64;
    fn connected(_: auto, a: usize, b: usize): bool;
    fn unite(_: auto, a: usize, b: usize): bool;
    fn setSize(_: auto, x: usize): usize;
    fn reset(_: auto, n: usize);
}

