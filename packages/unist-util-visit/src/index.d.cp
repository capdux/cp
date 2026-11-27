// Type definitions for CP

export class UnistNode {
    kind: string,
    value: string,
    parent: u32,
    first_child: u32,
    next_sibling: u32,
}

export fn uuvStrEq(a: string, b: string): bool;
export class UnistTree {
    nodes: [128]UnistNode,
    len: usize,
    fn addNode(_: auto, kind: string, value: string, parent_idx: u32): u32;
    fn isNode(idx: u32, target_kind: string): bool;
    fn visitPreOrder(root_idx: u32, target_kind: string, out: []u32): usize;
    fn getAncestors(node_idx: u32, out: []u32): usize;
}

export fn newUnistTree(): UnistTree;
