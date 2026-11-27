// Type definitions for CP

export enum TreeStyle {
    Unicode,
    Rounded,
    Ascii
}

export class TreeNode {
    label: string,
    parent: i32,
    first_child: i32,
    last_child: i32,
    next_sibling: i32,
}

export class TreeBuilder {
    nodes: [MAX_NODES]TreeNode,
    count: usize,
    fn init(root_label: string): TreeBuilder;
    fn addChild(_: auto, parent_idx: usize, label: string): usize?;
    fn nodeCount(): usize;
}

export fn appendStr(out: []u8, pos: usize, s: string): usize;
export fn appendByte(out: []u8, pos: usize, c: u8): usize;
export fn branchStr(style: TreeStyle, is_last: bool): string;
export fn guideStr(style: TreeStyle, has_more: bool): string;
export fn renderNodeRec(tree: &mut TreeBuilder, node_idx: usize, depth: usize, ancestors_have_next: &mut [MAX_DEPTH]bool, style: TreeStyle, out: []u8, pos_in: usize): usize;
export fn renderTreeWith(tree: &mut TreeBuilder, style: TreeStyle, out: []u8): string;
export fn renderTree(tree: &mut TreeBuilder, out: []u8): string;
