// @cap/unist-util-visit — Zero-heap arena-based unist AST visitor in pure CP
// Clean-room MIT implementation of npm `unist-util-visit` (#507, 111.7M/wk),
// `unist-util-visit-parents` (#508), and `unist-util-is` (#524).

pub const UNIST_NONE: u32 = 0xFFFFFFFF;
pub const UNIST_MAX_NODES: usize = 128;

pub struct UnistNode {
    pub kind: string,
    pub value: string,
    pub parent: u32,      // index of parent node, or UNIST_NONE for root
    pub first_child: u32, // index of first child, or UNIST_NONE
    pub next_sibling: u32, // index of next sibling, or UNIST_NONE
}

fn uuvStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub struct UnistTree {
    pub nodes: [128]UnistNode,
    pub len: usize,

    /// Adds a node under `parent_idx` (`UNIST_NONE` for root). Returns new node index or `UNIST_NONE` if full.
    pub fn addNode(*_, kind: string, value: string, parent_idx: u32): u32 {
        if (_.len >= UNIST_MAX_NODES) {
            return UNIST_NONE;
        }
        const idx = _.len as u32;
        _.nodes[_.len] = UnistNode {
            kind: kind,
            value: value,
            parent: parent_idx,
            first_child: UNIST_NONE,
            next_sibling: UNIST_NONE,
        };
        _.len += 1;

        if (parent_idx != UNIST_NONE && (parent_idx as usize) < _.len) {
            const p = parent_idx as usize;
            if (_.nodes[p].first_child == UNIST_NONE) {
                _.nodes[p].first_child = idx;
            } else {
                let cur = _.nodes[p].first_child;
                while (_.nodes[cur as usize].next_sibling != UNIST_NONE) {
                    cur = _.nodes[cur as usize].next_sibling;
                }
                _.nodes[cur as usize].next_sibling = idx;
            }
        }
        return idx;
    }

    /// Checks whether node `idx` matches `target_kind` (`""` matches all nodes — `unist-util-is`).
    pub fn isNode(idx: u32, target_kind: string): bool {
        if ((idx as usize) >= _.len) {
            return false;
        }
        if (target_kind.len == 0) {
            return true;
        }
        return uuvStrEq(_.nodes[idx as usize].kind, target_kind);
    }

    /// Iterative pre-order DFS traversal collecting matching node indices into `out`.
    /// Pass `target_kind = ""` to visit all nodes. Returns number of matched nodes.
    pub fn visitPreOrder(root_idx: u32, target_kind: string, out: []u32): usize {
        if ((root_idx as usize) >= _.len) {
            return 0;
        }
        let stack: [128]u32 = [0; 128];
        let sp: usize = 0;
        stack[0] = root_idx;
        sp = 1;

        let count: usize = 0;
        while (sp > 0) {
            sp -= 1;
            const cur = stack[sp];
            if (_.isNode(cur, target_kind)) {
                if (count < out.len) {
                    out[count] = cur;
                }
                count += 1;
            }
            // Collect children then push in reverse so first_child is popped first
            let children: [128]u32 = [0; 128];
            let c_len: usize = 0;
            let ch = _.nodes[cur as usize].first_child;
            while (ch != UNIST_NONE && c_len < 128) {
                children[c_len] = ch;
                c_len += 1;
                ch = _.nodes[ch as usize].next_sibling;
            }
            let k = c_len;
            while (k > 0 && sp < 128) {
                k -= 1;
                stack[sp] = children[k];
                sp += 1;
            }
        }
        return count;
    }

    /// Collects the ancestor chain (root -> ... -> parent) of `node_idx` into `out` (`unist-util-visit-parents`).
    pub fn getAncestors(node_idx: u32, out: []u32): usize {
        if ((node_idx as usize) >= _.len) {
            return 0;
        }
        let rev: [128]u32 = [0; 128];
        let rlen: usize = 0;
        let cur = _.nodes[node_idx as usize].parent;
        while (cur != UNIST_NONE && (cur as usize) < _.len && rlen < 128) {
            rev[rlen] = cur;
            rlen += 1;
            cur = _.nodes[cur as usize].parent;
        }
        let written: usize = 0;
        let k = rlen;
        while (k > 0 && written < out.len) {
            k -= 1;
            out[written] = rev[k];
            written += 1;
        }
        return written;
    }
}

/// Creates an empty `UnistTree`.
pub fn newUnistTree(): UnistTree {
    const empty = UnistNode {
        kind: "",
        value: "",
        parent: UNIST_NONE,
        first_child: UNIST_NONE,
        next_sibling: UNIST_NONE,
    };
    return UnistTree {
        nodes: [empty; 128],
        len: 0,
    };
}

export {
    UNIST_NONE,
    UNIST_MAX_NODES,
    UnistNode,
    UnistTree,
    newUnistTree,
};
