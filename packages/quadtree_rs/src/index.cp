// packages/@cap/quadtree-2d/src/index.cp
// Zero-allocation pooled 2D QuadTree spatial index (range & radius queries).
// MIT clean-room pure CP implementation.

/// 2D axis-aligned bounding rectangle.
pub struct Rect2 {
    pub min_x: f64,
    pub min_y: f64,
    pub max_x: f64,
    pub max_y: f64,

    pub static fn new(min_x: f64, min_y: f64, max_x: f64, max_y: f64): Rect2 {
        return Rect2{
            min_x: min_x,
            min_y: min_y,
            max_x: max_x,
            max_y: max_y,
        };
    }

    pub fn contains(x: f64, y: f64): bool {
        return x >= _.min_x && x <= _.max_x && y >= _.min_y && y <= _.max_y;
    }

    pub fn intersects(other: Rect2): bool {
        return _.min_x <= other.max_x && _.max_x >= other.min_x && _.min_y <= other.max_y && _.max_y >= other.min_y;
    }
}

/// Intrusive linked-list point item stored in a caller-owned pool.
pub struct QuadItem {
    pub x: f64,
    pub y: f64,
    pub id: u32,
    pub next_item: i32,
}

/// QuadTree node stored in a caller-owned pool.
pub struct QuadNode {
    pub bounds: Rect2,
    pub first_item: i32,
    pub count: u32,
    pub children: [4]i32,
    pub is_leaf: bool,
}

fn qtMakeLeaf(bounds: Rect2): QuadNode {
    let ch: [4]i32 = undefined;
    ch[0] = -1 as i32;
    ch[1] = -1 as i32;
    ch[2] = -1 as i32;
    ch[3] = -1 as i32;
    return QuadNode{
        bounds: bounds,
        first_item: -1 as i32,
        count: 0 as u32,
        children: ch,
        is_leaf: true,
    };
}

fn qtChildIndex(b: Rect2, x: f64, y: f64): usize {
    const mx: f64 = (b.min_x + b.max_x) * (0.5 as f64);
    const my: f64 = (b.min_y + b.max_y) * (0.5 as f64);
    let q: usize = 0;
    if (x >= mx) { q += 1; }
    if (y >= my) { q += 2; }
    return q;
}

/// Pooled zero-allocation 2D QuadTree spatial index.
pub struct QuadTree {
    pub node_count: usize,
    pub item_count: usize,
    pub max_leaf_items: u32,
    pub max_depth: u32,

    /// Create an uninitialized `QuadTree` header (call `init` before inserting).
    pub static fn empty(): QuadTree {
        return QuadTree{
            node_count: 0,
            item_count: 0,
            max_leaf_items: 4 as u32,
            max_depth: 8 as u32,
        };
    }

    /// Initialize root node in `nodes[0]` covering `bounds` with leaf capacity `max_leaf` and depth limit `max_depth`.
    /// Returns `false` if `nodes` is empty or `bounds` is invalid.
    pub fn init(*_, nodes: []QuadNode, bounds: Rect2, max_leaf: u32, max_depth: u32): bool {
        if (nodes.len == 0 || bounds.min_x > bounds.max_x || bounds.min_y > bounds.max_y) {
            _.node_count = 0;
            _.item_count = 0;
            return false;
        }
        nodes[0] = qtMakeLeaf(bounds);
        _.node_count = 1;
        _.item_count = 0;
        if (max_leaf == (0 as u32)) {
            _.max_leaf_items = 4 as u32;
        } else {
            _.max_leaf_items = max_leaf;
        }
        _.max_depth = max_depth;
        return true;
    }

    /// Insert point `(x, y)` with payload `id` into the QuadTree.
    /// Automatically subdivides leaf nodes into 4 quadrants when `count >= max_leaf_items`
    /// and `depth < max_depth` (if the node pool has room for 4 new children).
    /// Returns `false` if tree is uninitialized, point is outside root bounds, or `items` pool is full.
    pub fn insert(*_, nodes: []QuadNode, items: []QuadItem, x: f64, y: f64, id: u32): bool {
        if (_.node_count == 0 || _.item_count >= items.len) {
            return false;
        }
        if (!nodes[0].bounds.contains(x, y)) {
            return false;
        }

        let cur: usize = 0;
        let depth: u32 = 0 as u32;

        while (true) {
            const nodeSnap = nodes[cur];
            if (!nodeSnap.is_leaf) {
                const q = qtChildIndex(nodeSnap.bounds, x, y);
                const childIdx = nodeSnap.children[q];
                if (childIdx < (0 as i32)) {
                    return false;
                }
                cur = childIdx as usize;
                depth += 1 as u32;
                continue;
            }

            // Subdivide if leaf is full, below max_depth, and node pool has space for 4 children.
            if (nodeSnap.count >= _.max_leaf_items && depth < _.max_depth && _.node_count + 4 <= nodes.len) {
                const b = nodeSnap.bounds;
                const mx: f64 = (b.min_x + b.max_x) * (0.5 as f64);
                const my: f64 = (b.min_y + b.max_y) * (0.5 as f64);

                const c0 = _.node_count + 0;
                const c1 = _.node_count + 1;
                const c2 = _.node_count + 2;
                const c3 = _.node_count + 3;
                _.node_count += 4;

                nodes[c0] = qtMakeLeaf(Rect2.new(b.min_x, b.min_y, mx, my));
                nodes[c1] = qtMakeLeaf(Rect2.new(mx, b.min_y, b.max_x, my));
                nodes[c2] = qtMakeLeaf(Rect2.new(b.min_x, my, mx, b.max_y));
                nodes[c3] = qtMakeLeaf(Rect2.new(mx, my, b.max_x, b.max_y));

                // Re-distribute existing items from this leaf into its 4 children.
                let itIdx = nodeSnap.first_item;
                while (itIdx >= (0 as i32)) {
                    const it = items[itIdx as usize];
                    const nextIt = it.next_item;
                    const q = qtChildIndex(b, it.x, it.y);
                    let targetNodeIdx = c0;
                    if (q == 1) { targetNodeIdx = c1; }
                    else if (q == 2) { targetNodeIdx = c2; }
                    else if (q == 3) { targetNodeIdx = c3; }

                    let childNode = nodes[targetNodeIdx];
                    items[itIdx as usize] = QuadItem{
                        x: it.x,
                        y: it.y,
                        id: it.id,
                        next_item: childNode.first_item,
                    };
                    childNode.first_item = itIdx;
                    childNode.count += 1 as u32;
                    nodes[targetNodeIdx] = childNode;

                    itIdx = nextIt;
                }

                let ch: [4]i32 = undefined;
                ch[0] = c0 as i32;
                ch[1] = c1 as i32;
                ch[2] = c2 as i32;
                ch[3] = c3 as i32;
                nodes[cur] = QuadNode{
                    bounds: b,
                    first_item: -1 as i32,
                    count: 0 as u32,
                    children: ch,
                    is_leaf: false,
                };
                continue;
            }

            // Attach new item to the head of this leaf's linked list.
            const newIdx = _.item_count;
            _.item_count += 1;
            items[newIdx] = QuadItem{
                x: x,
                y: y,
                id: id,
                next_item: nodeSnap.first_item,
            };
            let updatedLeaf = nodeSnap;
            updatedLeaf.first_item = newIdx as i32;
            updatedLeaf.count += 1 as u32;
            nodes[cur] = updatedLeaf;
            return true;
        }

        return false;
    }

    /// Collect all item IDs inside rectangular `range` into `out_ids`. Returns number of IDs written.
    pub fn queryRect(nodes: []QuadNode, items: []QuadItem, range: Rect2, out_ids: []u32): usize {
        if (_.node_count == 0 || out_ids.len == 0) {
            return 0;
        }

        let stack: [128]usize = undefined;
        let sp: usize = 0;
        stack[0] = 0;
        sp = 1;

        let found: usize = 0;
        while (sp > 0 && found < out_ids.len) {
            sp -= 1;
            const nodeIdx = stack[sp];
            const nd = nodes[nodeIdx];
            if (!nd.bounds.intersects(range)) {
                continue;
            }

            if (nd.is_leaf) {
                let itIdx = nd.first_item;
                while (itIdx >= (0 as i32) && found < out_ids.len) {
                    const it = items[itIdx as usize];
                    if (range.contains(it.x, it.y)) {
                        out_ids[found] = it.id;
                        found += 1;
                    }
                    itIdx = it.next_item;
                }
            } else {
                let ci: usize = 0;
                while (ci < 4) {
                    const child = nd.children[ci];
                    if (child >= (0 as i32) && sp < 128) {
                        stack[sp] = child as usize;
                        sp += 1;
                    }
                    ci += 1;
                }
            }
        }

        return found;
    }

    /// Collect all item IDs within Euclidean `radius` of `(cx, cy)` into `out_ids`. Returns number of IDs written.
    pub fn queryRadius(nodes: []QuadNode, items: []QuadItem, cx: f64, cy: f64, radius: f64, out_ids: []u32): usize {
        if (_.node_count == 0 || radius < (0.0 as f64) || out_ids.len == 0) {
            return 0;
        }

        const bbox = Rect2.new(cx - radius, cy - radius, cx + radius, cy + radius);
        const rSq: f64 = radius * radius;

        let stack: [128]usize = undefined;
        let sp: usize = 0;
        stack[0] = 0;
        sp = 1;

        let found: usize = 0;
        while (sp > 0 && found < out_ids.len) {
            sp -= 1;
            const nodeIdx = stack[sp];
            const nd = nodes[nodeIdx];
            if (!nd.bounds.intersects(bbox)) {
                continue;
            }

            if (nd.is_leaf) {
                let itIdx = nd.first_item;
                while (itIdx >= (0 as i32) && found < out_ids.len) {
                    const it = items[itIdx as usize];
                    const dx: f64 = it.x - cx;
                    const dy: f64 = it.y - cy;
                    if (((dx * dx) + (dy * dy)) <= rSq) {
                        out_ids[found] = it.id;
                        found += 1;
                    }
                    itIdx = it.next_item;
                }
            } else {
                let ci: usize = 0;
                while (ci < 4) {
                    const child = nd.children[ci];
                    if (child >= (0 as i32) && sp < 128) {
                        stack[sp] = child as usize;
                        sp += 1;
                    }
                    ci += 1;
                }
            }
        }

        return found;
    }
}

export {
    Rect2,
    QuadItem,
    QuadNode,
    QuadTree,
};
