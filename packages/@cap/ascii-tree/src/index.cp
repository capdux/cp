// @cap/ascii-tree — Zero-allocation Unicode & ASCII tree renderer for CP
// Clean-room MIT implementation inspired by npm `archy` (35M/wk), `treeify`, and Rust `ptree`.

const MAX_NODES: usize = 128;
const MAX_DEPTH: usize = 32;

pub enum TreeStyle {
    Unicode, // ├── └── │
    Rounded, // ├── ╰── │
    Ascii,   // |-- `-- |
}

pub struct TreeNode {
    pub label: string,
    pub parent: i32, // -1 for root
    pub first_child: i32,
    pub last_child: i32,
    pub next_sibling: i32,
}

pub struct TreeBuilder {
    pub nodes: [MAX_NODES]TreeNode,
    pub count: usize,

    pub static fn init(root_label: string) -> TreeBuilder {
        let arr: [MAX_NODES]TreeNode = undefined;
        arr[0] = TreeNode{
            label: root_label,
            parent: -1,
            first_child: -1,
            last_child: -1,
            next_sibling: -1,
        };
        return TreeBuilder{
            nodes: arr,
            count: 1,
        };
    }

    pub fn addChild(*_, parent_idx: usize, label: string) -> usize? {
        if (_.count >= MAX_NODES || parent_idx >= _.count) {
            return null;
        }
        const idx = _.count;
        _.nodes[idx] = TreeNode{
            label: label,
            parent: parent_idx as i32,
            first_child: -1,
            last_child: -1,
            next_sibling: -1,
        };
        const prev_last = _.nodes[parent_idx].last_child;
        if (prev_last < 0) {
            _.nodes[parent_idx].first_child = idx as i32;
        } else {
            _.nodes[prev_last as usize].next_sibling = idx as i32;
        }
        _.nodes[parent_idx].last_child = idx as i32;
        _.count += 1;
        return idx;
    }

    pub fn nodeCount() -> usize {
        return _.count;
    }
}

fn appendStr(out: []u8, pos: usize, s: string) -> usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn appendByte(out: []u8, pos: usize, c: u8) -> usize {
    if (pos < out.len) {
        out[pos] = c;
        return pos + 1;
    }
    return pos;
}

fn branchStr(style: TreeStyle, is_last: bool) -> string {
    switch (style) {
        TreeStyle.Unicode => {
            return if (is_last) { "└── " } else { "├── " };
        },
        TreeStyle.Rounded => {
            return if (is_last) { "╰── " } else { "├── " };
        },
        TreeStyle.Ascii => {
            return if (is_last) { "`-- " } else { "|-- " };
        },
    }
}

fn guideStr(style: TreeStyle, has_more: bool) -> string {
    if (!has_more) {
        return "    ";
    }
    switch (style) {
        TreeStyle.Ascii => {
            return "|   ";
        },
        TreeStyle.Unicode => {
            return "│   ";
        },
        TreeStyle.Rounded => {
            return "│   ";
        },
    }
}

fn renderNodeRec(
    tree: *TreeBuilder,
    node_idx: usize,
    depth: usize,
    ancestors_have_next: *[MAX_DEPTH]bool,
    style: TreeStyle,
    out: []u8,
    pos_in: usize
) -> usize {
    let pos = pos_in;
    if (depth >= MAX_DEPTH) {
        return pos;
    }
    let child = tree.nodes[node_idx].first_child;
    while (child >= 0) {
        const cidx = child as usize;
        const next_sib = tree.nodes[cidx].next_sibling;
        const is_last = next_sib < 0;

        pos = appendByte(out, pos, '\n');
        let d: usize = 0;
        while (d < depth) {
            pos = appendStr(out, pos, guideStr(style, ancestors_have_next[d]));
            d += 1;
        }
        pos = appendStr(out, pos, branchStr(style, is_last));
        pos = appendStr(out, pos, tree.nodes[cidx].label);

        ancestors_have_next[depth] = !is_last;
        pos = renderNodeRec(tree, cidx, depth + 1, ancestors_have_next, style, out, pos);
        child = next_sib;
    }
    return pos;
}

pub fn renderTreeWith(tree: *TreeBuilder, style: TreeStyle, out: []u8) -> string {
    if (tree.count == 0) {
        return out[0..0];
    }
    let pos: usize = 0;
    pos = appendStr(out, pos, tree.nodes[0].label);
    let ancestors: [MAX_DEPTH]bool = undefined;
    let i: usize = 0;
    while (i < MAX_DEPTH) {
        ancestors[i] = false;
        i += 1;
    }
    pos = renderNodeRec(tree, 0 as usize, 0 as usize, &ancestors, style, out, pos);
    return out[0..pos];
}

pub fn renderTree(tree: *TreeBuilder, out: []u8) -> string {
    return renderTreeWith(tree, TreeStyle.Unicode, out);
}

export {
    TreeStyle,
    TreeNode,
    TreeBuilder,
    renderTree,
    renderTreeWith,
};
