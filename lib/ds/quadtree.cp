// Source: cp/lib/quadtree/quadtree.cp (Dynamic Split/Merge QuadTree 2D Spatial Index Engine)
// Pure CP, zero-C-dependency 2D spatial index supporting dynamic split/merge,
// rectangular AOI queries, circular radius queries, and priority-queue pruned K-NN.

const std = @import("std");

const NULL_NODE: usize = std.math.maxInt(usize);

/// Error set for QuadTree spatial index operations.
pub err QuadError {
    OutOfBounds,
    InvalidBounds,
    InvalidConfig,
    OutOfMemory,
}

/// Stored 2D spatial entry with a user-defined identifier and coordinates.
pub struct QuadEntry {
    id: u64;
    x: f64;
    y: f64;
}

/// Query result entry carrying squared Euclidean distance to the query point.
pub struct QuadHit {
    id: u64;
    x: f64;
    y: f64;
    dist_sq: f64;
}

struct QuadNode {
    min_x: f64;
    min_y: f64;
    max_x: f64;
    max_y: f64;
    mid_x: f64;
    mid_y: f64;
    depth: usize;
    is_divided: bool = false;
    children: [4]usize = [NULL_NODE, NULL_NODE, NULL_NODE, NULL_NODE];
    entries: std.ArrayList(QuadEntry) = .empty;
}

struct NodeCand {
    node_idx: usize;
    min_dist_sq: f64;
}

fn isPointInBounds(x: f64, y: f64, min_x: f64, min_y: f64, max_x: f64, max_y: f64): bool {
    if (!std.math.isFinite(x) || !std.math.isFinite(y)) return false;
    return x >= min_x && x <= max_x && y >= min_y && y <= max_y;
}

fn nodeBoxDistSq(node: QuadNode, qx: f64, qy: f64): f64 {
    let dx: f64 = 0.0;
    if (qx < node.min_x) {
        dx = node.min_x - qx;
    } else if (qx > node.max_x) {
        dx = qx - node.max_x;
    }
    let dy: f64 = 0.0;
    if (qy < node.min_y) {
        dy = node.min_y - qy;
    } else if (qy > node.max_y) {
        dy = qy - node.max_y;
    }
    return dx * dx + dy * dy;
}

fn nodeIntersectsRect(node: QuadNode, min_x: f64, min_y: f64, max_x: f64, max_y: f64): bool {
    return node.min_x <= max_x && node.max_x >= min_x && node.min_y <= max_y && node.max_y >= min_y;
}

fn childQuadrant(node: QuadNode, x: f64, y: f64): usize {
    const east: usize = if (x >= node.mid_x) 1 else 0;
    const north: usize = if (y >= node.mid_y) 2 else 0;
    return east + north;
}

fn hitIsBetter(a: QuadHit, b: QuadHit): bool {
    if (a.dist_sq < b.dist_sq) return true;
    if (a.dist_sq > b.dist_sq) return false;
    return a.id < b.id;
}

fn pushNodeMinHeap(alloc: std.mem.Allocator, heap: *std.ArrayList(NodeCand), item: NodeCand): void ! QuadError {
    heap.append(alloc, item) catch return QuadError.OutOfMemory;
    let idx = heap.items.len - 1;
    while (idx > 0) {
        const parent = (idx - 1) / 2;
        if (heap.items[idx].min_dist_sq < heap.items[parent].min_dist_sq) {
            const tmp = heap.items[idx];
            heap.items[idx] = heap.items[parent];
            heap.items[parent] = tmp;
            idx = parent;
        } else {
            break;
        }
    }
}

fn popNodeMinHeap(heap: *std.ArrayList(NodeCand)): NodeCand {
    const top = heap.items[0];
    const last = heap.items[heap.items.len - 1];
    heap.items.len -= 1;
    if (heap.items.len > 0) {
        heap.items[0] = last;
        let idx: usize = 0;
        const n = heap.items.len;
        while (true) {
            const left = idx * 2 + 1;
            const right = idx * 2 + 2;
            let smallest = idx;
            if (left < n && heap.items[left].min_dist_sq < heap.items[smallest].min_dist_sq) {
                smallest = left;
            }
            if (right < n && heap.items[right].min_dist_sq < heap.items[smallest].min_dist_sq) {
                smallest = right;
            }
            if (smallest == idx) break;
            const tmp = heap.items[idx];
            heap.items[idx] = heap.items[smallest];
            heap.items[smallest] = tmp;
            idx = smallest;
        }
    }
    return top;
}

fn siftDownHitMaxHeap(heap: *std.ArrayList(QuadHit), start: usize) {
    let idx = start;
    const n = heap.items.len;
    while (true) {
        const left = idx * 2 + 1;
        const right = idx * 2 + 2;
        let worst = idx;
        // Max-heap keeps the worst (largest dist_sq, or larger id on tie) at index 0.
        if (left < n && hitIsBetter(heap.items[worst], heap.items[left])) {
            worst = left;
        }
        if (right < n && hitIsBetter(heap.items[worst], heap.items[right])) {
            worst = right;
        }
        if (worst == idx) break;
        const tmp = heap.items[idx];
        heap.items[idx] = heap.items[worst];
        heap.items[worst] = tmp;
        idx = worst;
    }
}

fn pushHitMaxHeap(alloc: std.mem.Allocator, heap: *std.ArrayList(QuadHit), item: QuadHit): void ! QuadError {
    heap.append(alloc, item) catch return QuadError.OutOfMemory;
    let idx = heap.items.len - 1;
    while (idx > 0) {
        const parent = (idx - 1) / 2;
        if (hitIsBetter(heap.items[parent], heap.items[idx])) {
            const tmp = heap.items[idx];
            heap.items[idx] = heap.items[parent];
            heap.items[parent] = tmp;
            idx = parent;
        } else {
            break;
        }
    }
}

fn sortHitsAsc(hits: []QuadHit) {
    if (hits.len <= 1) return;
    // Insertion/selection-free in-place heapsort or simple O(K log K) heapsort on slice.
    let i = hits.len / 2;
    while (i > 0) {
        i -= 1;
        siftDownSliceMax(hits, i, hits.len);
    }
    let end = hits.len;
    while (end > 1) {
        end -= 1;
        const tmp = hits[0];
        hits[0] = hits[end];
        hits[end] = tmp;
        siftDownSliceMax(hits, 0, end);
    }
}

fn siftDownSliceMax(hits: []QuadHit, start: usize, n: usize) {
    let idx = start;
    while (true) {
        const left = idx * 2 + 1;
        const right = idx * 2 + 2;
        let worst = idx;
        if (left < n && hitIsBetter(hits[worst], hits[left])) {
            worst = left;
        }
        if (right < n && hitIsBetter(hits[worst], hits[right])) {
            worst = right;
        }
        if (worst == idx) break;
        const tmp = hits[idx];
        hits[idx] = hits[worst];
        hits[worst] = tmp;
        idx = worst;
    }
}

/// Dynamic split/merge 2D QuadTree spatial index backed by an arena-style node pool.
pub struct QuadTree {
    alloc: std.mem.Allocator;
    nodes: std.ArrayList(QuadNode) = .empty;
    free_slots: std.ArrayList(usize) = .empty;
    root: usize = 0;
    bucket_capacity: usize;
    max_depth: usize;
    total_len: usize = 0;
    active_nodes: usize = 0;

    /// Initializes a QuadTree covering `[min_x, max_x] x [min_y, max_y]`.
    pub static fn init(
        alloc: std.mem.Allocator,
        min_x: f64,
        min_y: f64,
        max_x: f64,
        max_y: f64,
        bucket_capacity: usize,
        max_depth: usize,
    ): QuadTree ! QuadError {
        if (!std.math.isFinite(min_x) || !std.math.isFinite(min_y) || !std.math.isFinite(max_x) || !std.math.isFinite(max_y)) {
            return QuadError.InvalidBounds;
        }
        if (!(min_x < max_x) || !(min_y < max_y)) {
            return QuadError.InvalidBounds;
        }
        if (bucket_capacity == 0) {
            return QuadError.InvalidConfig;
        }

        let tree = QuadTree{
            alloc: alloc,
            nodes: .empty,
            free_slots: .empty,
            root: 0,
            bucket_capacity: bucket_capacity,
            max_depth: max_depth,
            total_len: 0,
            active_nodes: 0,
        };
        errdefer tree.deinit();

        const root_idx = tree.allocNode(min_x, min_y, max_x, max_y, 0)?;
        tree.root = root_idx;
        return tree;
    }

    /// Releases all node storage and internal entry lists.
    pub fn deinit(*_) {
        for (*node of _.nodes.items) {
            node.entries.deinit(_.alloc);
        }
        _.nodes.deinit(_.alloc);
        _.free_slots.deinit(_.alloc);
        _.root = NULL_NODE;
        _.total_len = 0;
        _.active_nodes = 0;
    }

    /// Returns the total number of entries currently indexed.
    pub fn len(): usize {
        return _.total_len;
    }

    /// Returns the number of active nodes (internal + leaf) in the tree.
    pub fn nodeCount(): usize {
        return _.active_nodes;
    }

    priv fn allocNode(*_, min_x: f64, min_y: f64, max_x: f64, max_y: f64, depth: usize): usize ! QuadError {
        const node = QuadNode{
            min_x: min_x,
            min_y: min_y,
            max_x: max_x,
            max_y: max_y,
            mid_x: min_x + (max_x - min_x) * 0.5,
            mid_y: min_y + (max_y - min_y) * 0.5,
            depth: depth,
            is_divided: false,
            children: [NULL_NODE, NULL_NODE, NULL_NODE, NULL_NODE],
            entries: .empty,
        };
        if (_.free_slots.items.len > 0) {
            const slot = _.free_slots.items[_.free_slots.items.len - 1];
            _.free_slots.items.len -= 1;
            _.nodes.items[slot] = node;
            _.active_nodes += 1;
            return slot;
        }
        _.nodes.append(_.alloc, node) catch return QuadError.OutOfMemory;
        _.active_nodes += 1;
        return _.nodes.items.len - 1;
    }

    priv fn freeNode(*_, idx: usize) {
        _.nodes.items[idx].entries.deinit(_.alloc);
        _.nodes.items[idx].entries = .empty;
        _.nodes.items[idx].is_divided = false;
        _.nodes.items[idx].children = [NULL_NODE, NULL_NODE, NULL_NODE, NULL_NODE];
        _.free_slots.append(_.alloc, idx) catch {};
        _.active_nodes -= 1;
    }

    priv fn canSubdivide(node: QuadNode, new_x: f64, new_y: f64): bool {
        if (node.mid_x <= node.min_x || node.mid_x >= node.max_x) return false;
        if (node.mid_y <= node.min_y || node.mid_y >= node.max_y) return false;
        for (e of node.entries.items) {
            if (e.x != new_x || e.y != new_y) return true;
        }
        return false;
    }

    priv fn subdivide(*_, node_idx: usize): void ! QuadError {
        const n = _.nodes.items[node_idx];
        const next_depth = n.depth + 1;

        const sw = _.allocNode(n.min_x, n.min_y, n.mid_x, n.mid_y, next_depth)?;
        errdefer _.freeNode(sw);
        const se = _.allocNode(n.mid_x, n.min_y, n.max_x, n.mid_y, next_depth)?;
        errdefer _.freeNode(se);
        const nw = _.allocNode(n.min_x, n.mid_y, n.mid_x, n.max_y, next_depth)?;
        errdefer _.freeNode(nw);
        const ne = _.allocNode(n.mid_x, n.mid_y, n.max_x, n.max_y, next_depth)?;
        errdefer _.freeNode(ne);

        const child_ids: [4]usize = [sw, se, nw, ne];
        const parent_snapshot = _.nodes.items[node_idx];
        for (e of parent_snapshot.entries.items) {
            const q = childQuadrant(parent_snapshot, e.x, e.y);
            const c_idx = child_ids[q];
            _.nodes.items[c_idx].entries.append(_.alloc, e) catch return QuadError.OutOfMemory;
        }

        _.nodes.items[node_idx].entries.deinit(_.alloc);
        _.nodes.items[node_idx].entries = .empty;
        _.nodes.items[node_idx].children = child_ids;
        _.nodes.items[node_idx].is_divided = true;
    }

    /// Inserts `(id, x, y)` into the QuadTree, subdividing leaves as needed.
    pub fn insert(*_, id: u64, x: f64, y: f64): void ! QuadError {
        const root_node = _.nodes.items[_.root];
        if (!isPointInBounds(x, y, root_node.min_x, root_node.min_y, root_node.max_x, root_node.max_y)) {
            return QuadError.OutOfBounds;
        }

        let cur_idx = _.root;
        while (true) {
            const cur = _.nodes.items[cur_idx];
            if (!cur.is_divided) {
                if (cur.entries.items.len < _.bucket_capacity || cur.depth >= _.max_depth || !QuadTree.canSubdivide(cur, x, y)) {
                    _.nodes.items[cur_idx].entries.append(_.alloc, QuadEntry{ id: id, x: x, y: y }) catch return QuadError.OutOfMemory;
                    _.total_len += 1;
                    return;
                }
                _.subdivide(cur_idx)?;
            }
            const updated_cur = _.nodes.items[cur_idx];
            const q = childQuadrant(updated_cur, x, y);
            cur_idx = updated_cur.children[q];
        }
    }

    priv fn tryCollapse(*_, node_idx: usize): bool {
        const parent = _.nodes.items[node_idx];
        if (!parent.is_divided) return false;

        let total_entries: usize = 0;
        for (c_idx of parent.children) {
            const child = _.nodes.items[c_idx];
            if (child.is_divided) return false;
            total_entries += child.entries.items.len;
        }
        if (total_entries > _.bucket_capacity) return false;

        if (total_entries > 0) {
            _.nodes.items[node_idx].entries.ensureTotalCapacity(_.alloc, total_entries) catch return false;
            for (c_idx of parent.children) {
                for (e of _.nodes.items[c_idx].entries.items) {
                    _.nodes.items[node_idx].entries.appendAssumeCapacity(e);
                }
            }
        }
        for (c_idx of parent.children) {
            _.freeNode(c_idx);
        }
        _.nodes.items[node_idx].children = [NULL_NODE, NULL_NODE, NULL_NODE, NULL_NODE];
        _.nodes.items[node_idx].is_divided = false;
        return true;
    }

    /// Removes the entry matching `(id, x, y)` and collapses empty/underfull sibling quads.
    pub fn remove(*_, id: u64, x: f64, y: f64): bool {
        const root_node = _.nodes.items[_.root];
        if (!isPointInBounds(x, y, root_node.min_x, root_node.min_y, root_node.max_x, root_node.max_y)) {
            return false;
        }

        let path: std.ArrayList(usize) = .empty;
        defer path.deinit(_.alloc);

        let cur_idx = _.root;
        while (true) {
            const cur = _.nodes.items[cur_idx];
            if (!cur.is_divided) break;
            path.append(_.alloc, cur_idx) catch return false;
            const q = childQuadrant(cur, x, y);
            cur_idx = cur.children[q];
        }

        let found = false;
        const leaf_len = _.nodes.items[cur_idx].entries.items.len;
        for (i of 0..leaf_len) {
            const e = _.nodes.items[cur_idx].entries.items[i];
            if (e.id == id && e.x == x && e.y == y) {
                * = _.nodes.items[cur_idx].entries.swapRemove(i);
                _.total_len -= 1;
                found = true;
                break;
            }
        }
        if (!found) return false;

        let p = path.items.len;
        while (p > 0) {
            p -= 1;
            if (!_.tryCollapse(path.items[p])) {
                break;
            }
        }
        return true;
    }

    /// Moves an existing entry from `(old_x, old_y)` to `(new_x, new_y)`.
    pub fn update(*_, id: u64, old_x: f64, old_y: f64, new_x: f64, new_y: f64): bool ! QuadError {
        const root_node = _.nodes.items[_.root];
        if (!isPointInBounds(new_x, new_y, root_node.min_x, root_node.min_y, root_node.max_x, root_node.max_y)) {
            return QuadError.OutOfBounds;
        }
        if (!_.remove(id, old_x, old_y)) {
            return false;
        }
        _.insert(id, new_x, new_y)?;
        return true;
    }

    /// Queries all entries lying inside `[min_x, max_x] x [min_y, max_y]`.
    pub fn queryRect(alloc: std.mem.Allocator, min_x: f64, min_y: f64, max_x: f64, max_y: f64): []QuadEntry ! QuadError {
        if (!std.math.isFinite(min_x) || !std.math.isFinite(min_y) || !std.math.isFinite(max_x) || !std.math.isFinite(max_y)) {
            return QuadError.InvalidBounds;
        }
        if (!(min_x <= max_x) || !(min_y <= max_y)) {
            return QuadError.InvalidBounds;
        }

        let results: std.ArrayList(QuadEntry) = .empty;
        errdefer results.deinit(alloc);

        let stack: std.ArrayList(usize) = .empty;
        defer stack.deinit(alloc);
        stack.append(alloc, _.root) catch return QuadError.OutOfMemory;

        while (stack.items.len > 0) {
            const idx = stack.items[stack.items.len - 1];
            stack.items.len -= 1;
            const node = _.nodes.items[idx];
            if (!nodeIntersectsRect(node, min_x, min_y, max_x, max_y)) {
                continue;
            }
            if (!node.is_divided) {
                for (e of node.entries.items) {
                    if (e.x >= min_x && e.x <= max_x && e.y >= min_y && e.y <= max_y) {
                        results.append(alloc, e) catch return QuadError.OutOfMemory;
                    }
                }
            } else {
                for (c_idx of node.children) {
                    if (nodeIntersectsRect(_.nodes.items[c_idx], min_x, min_y, max_x, max_y)) {
                        stack.append(alloc, c_idx) catch return QuadError.OutOfMemory;
                    }
                }
            }
        }

        return results.toOwnedSlice(alloc) catch return QuadError.OutOfMemory;
    }

    /// Queries all entries within Euclidean distance `radius` from `(cx, cy)`.
    pub fn queryCircle(alloc: std.mem.Allocator, cx: f64, cy: f64, radius: f64): []QuadHit ! QuadError {
        if (!std.math.isFinite(cx) || !std.math.isFinite(cy) || !std.math.isFinite(radius) || radius < 0.0) {
            return QuadError.InvalidConfig;
        }

        const r_sq = radius * radius;
        let results: std.ArrayList(QuadHit) = .empty;
        errdefer results.deinit(alloc);

        let stack: std.ArrayList(usize) = .empty;
        defer stack.deinit(alloc);
        stack.append(alloc, _.root) catch return QuadError.OutOfMemory;

        while (stack.items.len > 0) {
            const idx = stack.items[stack.items.len - 1];
            stack.items.len -= 1;
            const node = _.nodes.items[idx];
            if (nodeBoxDistSq(node, cx, cy) > r_sq) {
                continue;
            }
            if (!node.is_divided) {
                for (e of node.entries.items) {
                    const dx = e.x - cx;
                    const dy = e.y - cy;
                    const d2 = dx * dx + dy * dy;
                    if (d2 <= r_sq) {
                        results.append(alloc, QuadHit{
                            id: e.id,
                            x: e.x,
                            y: e.y,
                            dist_sq: d2,
                        }) catch return QuadError.OutOfMemory;
                    }
                }
            } else {
                for (c_idx of node.children) {
                    if (nodeBoxDistSq(_.nodes.items[c_idx], cx, cy) <= r_sq) {
                        stack.append(alloc, c_idx) catch return QuadError.OutOfMemory;
                    }
                }
            }
        }

        return results.toOwnedSlice(alloc) catch return QuadError.OutOfMemory;
    }

    /// Finds up to `k` nearest neighbors to `(qx, qy)` sorted ascending by `dist_sq`,
    /// using best-first priority-queue bounding-box lower-bound pruning.
    pub fn nearestK(alloc: std.mem.Allocator, qx: f64, qy: f64, k: usize): []QuadHit ! QuadError {
        if (!std.math.isFinite(qx) || !std.math.isFinite(qy)) {
            return QuadError.InvalidBounds;
        }
        if (k == 0 || _.total_len == 0) {
            return alloc.alloc(QuadHit, 0) catch return QuadError.OutOfMemory;
        }

        let node_heap: std.ArrayList(NodeCand) = .empty;
        defer node_heap.deinit(alloc);

        let hit_heap: std.ArrayList(QuadHit) = .empty;
        errdefer hit_heap.deinit(alloc);

        const root_d2 = nodeBoxDistSq(_.nodes.items[_.root], qx, qy);
        pushNodeMinHeap(alloc, *node_heap, NodeCand{ node_idx: _.root, min_dist_sq: root_d2 })?;

        while (node_heap.items.len > 0) {
            const cand = popNodeMinHeap(*node_heap);
            if (hit_heap.items.len >= k && cand.min_dist_sq > hit_heap.items[0].dist_sq) {
                break;
            }
            const node = _.nodes.items[cand.node_idx];
            if (!node.is_divided) {
                for (e of node.entries.items) {
                    const dx = e.x - qx;
                    const dy = e.y - qy;
                    const d2 = dx * dx + dy * dy;
                    const h = QuadHit{ id: e.id, x: e.x, y: e.y, dist_sq: d2 };
                    if (hit_heap.items.len < k) {
                        pushHitMaxHeap(alloc, *hit_heap, h)?;
                    } else if (hitIsBetter(h, hit_heap.items[0])) {
                        hit_heap.items[0] = h;
                        siftDownHitMaxHeap(*hit_heap, 0);
                    }
                }
            } else {
                for (c_idx of node.children) {
                    const d2 = nodeBoxDistSq(_.nodes.items[c_idx], qx, qy);
                    if (hit_heap.items.len < k || d2 <= hit_heap.items[0].dist_sq) {
                        pushNodeMinHeap(alloc, *node_heap, NodeCand{ node_idx: c_idx, min_dist_sq: d2 })?;
                    }
                }
            }
        }

        const out = hit_heap.toOwnedSlice(alloc) catch return QuadError.OutOfMemory;
        sortHitsAsc(out);
        return out;
    }
}
