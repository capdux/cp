// Source: cp/lib/rtree/rtree.cp (2D Axis-Aligned Bounding Box R-Tree Spatial Index)
// Pure CP, zero-C-dependency 2D AABB R-Tree spatial index supporting configurable node capacity,
// ChooseLeaf minimal-enlargement insertion, Quadratic Split, MBR-tightening removal,
// overlap/point queries, and Best-First Branch-and-Bound k-nearest-neighbor search.

const std = @import("std");

const NULL_NODE: usize = std.math.maxInt(usize);
const DEFAULT_MAX_ENTRIES: usize = 16;

/// Error set for `cap:rtree` operations.
pub err RTreeError {
    InvalidBox,
    InvalidCapacity,
    OutOfMemory,
}

/// 2D Axis-Aligned Bounding Box (AABB) with double-precision coordinates.
pub struct Rect {
    pub min_x: f64;
    pub min_y: f64;
    pub max_x: f64;
    pub max_y: f64;

    /// Constructs a validated `Rect`, returning `RTreeError.InvalidBox` if any coordinate
    /// is non-finite or `min > max`.
    pub static fn init(min_x: f64, min_y: f64, max_x: f64, max_y: f64): Rect ! RTreeError {
        if (!std.math.isFinite(min_x) || !std.math.isFinite(min_y) || !std.math.isFinite(max_x) || !std.math.isFinite(max_y)) {
            return RTreeError.InvalidBox;
        }
        if (min_x > max_x || min_y > max_y) {
            return RTreeError.InvalidBox;
        }
        return Rect{
            min_x: min_x,
            min_y: min_y,
            max_x: max_x,
            max_y: max_y,
        };
    }

    /// Returns `true` if all coordinates are finite and `min_x <= max_x` and `min_y <= max_y`.
    pub fn isValid(): bool {
        if (!std.math.isFinite(_.min_x) || !std.math.isFinite(_.min_y) || !std.math.isFinite(_.max_x) || !std.math.isFinite(_.max_y)) {
            return false;
        }
        return _.min_x <= _.max_x && _.min_y <= _.max_y;
    }

    /// Area of the rectangle: `(max_x - min_x) * (max_y - min_y)`.
    pub fn area(): f64 {
        return (_.max_x - _.min_x) * (_.max_y - _.min_y);
    }

    /// Returns `true` if `this` and `other` intersect (including touching edges/corners).
    pub fn overlaps(other: Rect): bool {
        return !(_.max_x < other.min_x || other.max_x < _.min_x || _.max_y < other.min_y || other.max_y < _.min_y);
    }

    /// Returns `true` if `(x, y)` lies inside or on the boundary of `this`.
    pub fn containsPoint(x: f64, y: f64): bool {
        return x >= _.min_x && x <= _.max_x && y >= _.min_y && y <= _.max_y;
    }

    /// Returns `true` if `other` is completely contained within `this`.
    pub fn containsRect(other: Rect): bool {
        return _.min_x <= other.min_x && _.max_x >= other.max_x && _.min_y <= other.min_y && _.max_y >= other.max_y;
    }

    /// Returns the minimal bounding rectangle enclosing both `this` and `other`.
    pub fn enlarge(other: Rect): Rect {
        return Rect{
            min_x: @min(_.min_x, other.min_x),
            min_y: @min(_.min_y, other.min_y),
            max_x: @max(_.max_x, other.max_x),
            max_y: @max(_.max_y, other.max_y),
        };
    }

    /// Area increase required to enlarge `this` to enclose `other`.
    pub fn enlargementCost(other: Rect): f64 {
        return _.enlarge(other).area() - _.area();
    }

    /// Minimum squared Euclidean distance from point `(x, y)` to `this` (`0.0` if inside).
    pub fn distanceSqToPoint(x: f64, y: f64): f64 {
        let dx: f64 = 0.0;
        if (x < _.min_x) {
            dx = _.min_x - x;
        } else if (x > _.max_x) {
            dx = x - _.max_x;
        }
        let dy: f64 = 0.0;
        if (y < _.min_y) {
            dy = _.min_y - y;
        } else if (y > _.max_y) {
            dy = y - _.max_y;
        }
        return dx * dx + dy * dy;
    }
}

/// Spatial entry stored in an `RTree<V>`.
pub struct RTreeEntry<V> {
    pub id: u64;
    pub box_rect: Rect;
    pub value: V;
}

/// Nearest-neighbor query result pairing an `RTreeEntry<V>` with its squared distance.
pub struct NearestRect<V> {
    pub entry: RTreeEntry<V>;
    pub dist_sq: f64;
}

struct RTreeNode<V> {
    is_leaf: bool = true;
    parent: usize = NULL_NODE;
    mbr: Rect = Rect{ min_x: 0.0, min_y: 0.0, max_x: 0.0, max_y: 0.0 };
    entries: std.ArrayListUnmanaged(RTreeEntry(V)) = .empty;
    children: std.ArrayListUnmanaged(usize) = .empty;
}

struct BfsItem {
    is_entry: bool;
    dist_sq: f64;
    node_idx: usize;
    entry_idx: usize;
}

fn pushMinHeap(alloc: std.mem.Allocator, heap: *std.ArrayListUnmanaged(BfsItem), item: BfsItem): void ! RTreeError {
    heap.append(alloc, item) catch return RTreeError.OutOfMemory;
    let idx = heap.items.len - 1;
    while (idx > 0) {
        const parent = (idx - 1) / 2;
        if (heap.items[idx].dist_sq < heap.items[parent].dist_sq) {
            const tmp = heap.items[idx];
            heap.items[idx] = heap.items[parent];
            heap.items[parent] = tmp;
            idx = parent;
        } else {
            break;
        }
    }
}

fn popMinHeap(heap: *std.ArrayListUnmanaged(BfsItem)): BfsItem {
    const top = heap.items[0];
    const last = heap.items.len - 1;
    if (last > 0) {
        heap.items[0] = heap.items[last];
    }
    heap.items.len = last;
    if (last > 0) {
        let idx: usize = 0;
        while (true) {
            const left = idx * 2 + 1;
            const right = idx * 2 + 2;
            let smallest = idx;
            if (left < last && heap.items[left].dist_sq < heap.items[smallest].dist_sq) {
                smallest = left;
            }
            if (right < last && heap.items[right].dist_sq < heap.items[smallest].dist_sq) {
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

/// Dynamic 2D Axis-Aligned Bounding Box R-Tree spatial index.
/// Uses an index-pool of nodes, `ChooseLeaf` minimal-enlargement insertion,
/// Quadratic Split, and Best-First Branch-and-Bound k-NN search.
pub struct RTree<V> {
    allocator: std.mem.Allocator;
    max_entries: usize;
    min_entries: usize;
    nodes: std.ArrayListUnmanaged(RTreeNode(V)) = .empty;
    free_slots: std.ArrayListUnmanaged(usize) = .empty;
    root: usize = NULL_NODE;
    count: usize = 0;

    /// Initializes an empty `RTree<V>` with default maximum node capacity (`16`).
    pub static fn init(allocator: std.mem.Allocator): RTree<V> ! RTreeError {
        return RTree<V>.initWithCapacity(allocator, DEFAULT_MAX_ENTRIES);
    }

    /// Initializes an empty `RTree<V>` with configurable maximum node capacity `max_entries >= 4`.
    pub static fn initWithCapacity(allocator: std.mem.Allocator, max_entries: usize): RTree<V> ! RTreeError {
        if (max_entries < 4) {
            return RTreeError.InvalidCapacity;
        }
        return RTree<V>{
            allocator: allocator,
            max_entries: max_entries,
            min_entries: max_entries / 2,
            nodes: .empty,
            free_slots: .empty,
            root: NULL_NODE,
            count: 0,
        };
    }

    /// Releases all memory owned by the R-Tree.
    pub fn deinit(*_) {
        for (node of *_.nodes.items) {
            node.entries.deinit(_.allocator);
            node.children.deinit(_.allocator);
        }
        _.nodes.deinit(_.allocator);
        _.free_slots.deinit(_.allocator);
        _.root = NULL_NODE;
        _.count = 0;
    }

    /// Removes all entries and resets the tree while retaining pool capacity.
    pub fn clear(*_) {
        for (node of *_.nodes.items) {
            node.entries.clearRetainingCapacity();
            node.children.clearRetainingCapacity();
        }
        _.nodes.clearRetainingCapacity();
        _.free_slots.clearRetainingCapacity();
        _.root = NULL_NODE;
        _.count = 0;
    }

    /// Returns the number of entries stored in the tree.
    pub fn len(): usize {
        return _.count;
    }

    /// Returns `true` if the tree contains no entries.
    pub fn isEmpty(): bool {
        return _.count == 0;
    }

    /// Returns the overall minimum bounding rectangle (MBR) of the tree, or `null` if empty.
    pub fn bounds(): Rect? {
        if (_.root == NULL_NODE || _.count == 0) {
            return null;
        }
        return _.nodes.items[_.root].mbr;
    }

    fn allocNode(*_, is_leaf: bool): usize ! RTreeError {
        if (_.free_slots.items.len > 0) {
            const idx = _.free_slots.items[_.free_slots.items.len - 1];
            _.free_slots.items.len -= 1;
            _.nodes.items[idx].is_leaf = is_leaf;
            _.nodes.items[idx].parent = NULL_NODE;
            _.nodes.items[idx].mbr = Rect{ min_x: 0.0, min_y: 0.0, max_x: 0.0, max_y: 0.0 };
            _.nodes.items[idx].entries.clearRetainingCapacity();
            _.nodes.items[idx].children.clearRetainingCapacity();
            return idx;
        }
        const idx = _.nodes.items.len;
        _.nodes.append(_.allocator, RTreeNode(V){
            is_leaf: is_leaf,
            parent: NULL_NODE,
            mbr: Rect{ min_x: 0.0, min_y: 0.0, max_x: 0.0, max_y: 0.0 },
            entries: .empty,
            children: .empty,
        }) catch return RTreeError.OutOfMemory;
        return idx;
    }

    fn releaseSlot(*_, idx: usize) {
        _.nodes.items[idx].parent = NULL_NODE;
        _.nodes.items[idx].entries.clearRetainingCapacity();
        _.nodes.items[idx].children.clearRetainingCapacity();
        _.free_slots.append(_.allocator, idx) catch {};
    }

    fn recomputeMbr(*_, idx: usize) {
        if (_.nodes.items[idx].is_leaf) {
            const items = _.nodes.items[idx].entries.items;
            if (items.len == 0) return;
            let box_r = items[0].box_rect;
            let i: usize = 1;
            while (i < items.len) : (i += 1) {
                box_r = box_r.enlarge(items[i].box_rect);
            }
            _.nodes.items[idx].mbr = box_r;
        } else {
            const kids = _.nodes.items[idx].children.items;
            if (kids.len == 0) return;
            let box_r = _.nodes.items[kids[0]].mbr;
            let i: usize = 1;
            while (i < kids.len) : (i += 1) {
                box_r = box_r.enlarge(_.nodes.items[kids[i]].mbr);
            }
            _.nodes.items[idx].mbr = box_r;
        }
    }

    fn tightenAncestors(*_, start_idx: usize) {
        let curr = start_idx;
        while (curr != NULL_NODE) {
            _.recomputeMbr(curr);
            curr = _.nodes.items[curr].parent;
        }
    }

    fn chooseLeaf(*_, target: Rect): usize {
        let curr = _.root;
        while (!_.nodes.items[curr].is_leaf) {
            const kids = _.nodes.items[curr].children.items;
            let best_child = kids[0];
            let best_cost = _.nodes.items[best_child].mbr.enlargementCost(target);
            let best_area = _.nodes.items[best_child].mbr.area();
            let i: usize = 1;
            while (i < kids.len) : (i += 1) {
                const c = kids[i];
                const cost = _.nodes.items[c].mbr.enlargementCost(target);
                const c_area = _.nodes.items[c].mbr.area();
                if (cost < best_cost || (cost == best_cost && c_area < best_area)) {
                    best_cost = cost;
                    best_area = c_area;
                    best_child = c;
                }
            }
            curr = best_child;
        }
        return curr;
    }

    fn quadraticPickSeeds(rects: []const Rect, out_a: *usize, out_b: *usize) {
        let best_waste: f64 = -std.math.inf(f64);
        let seed_a: usize = 0;
        let seed_b: usize = 1;
        let i: usize = 0;
        while (i < rects.len) : (i += 1) {
            let j: usize = i + 1;
            while (j < rects.len) : (j += 1) {
                const combined = rects[i].enlarge(rects[j]);
                const waste = combined.area() - rects[i].area() - rects[j].area();
                if (waste > best_waste) {
                    best_waste = waste;
                    seed_a = i;
                    seed_b = j;
                }
            }
        }
        out_a.* = seed_a;
        out_b.* = seed_b;
    }

    fn splitNode(*_, node_idx: usize): usize ! RTreeError {
        const is_leaf = _.nodes.items[node_idx].is_leaf;
        const n: usize = if (is_leaf) _.nodes.items[node_idx].entries.items.len else _.nodes.items[node_idx].children.items.len;

        let rects = _.allocator.alloc(Rect, n) catch return RTreeError.OutOfMemory;
        defer _.allocator.free(rects);
        let assigned = _.allocator.alloc(bool, n) catch return RTreeError.OutOfMemory;
        defer _.allocator.free(assigned);

        let i: usize = 0;
        while (i < n) : (i += 1) {
            assigned[i] = false;
            if (is_leaf) {
                rects[i] = _.nodes.items[node_idx].entries.items[i].box_rect;
            } else {
                const child_idx = _.nodes.items[node_idx].children.items[i];
                rects[i] = _.nodes.items[child_idx].mbr;
            }
        }

        let seed_a: usize = 0;
        let seed_b: usize = 1;
        RTree<V>.quadraticPickSeeds(rects, &seed_a, &seed_b);

        let group_a: std.ArrayListUnmanaged(usize) = .empty;
        defer group_a.deinit(_.allocator);
        let group_b: std.ArrayListUnmanaged(usize) = .empty;
        defer group_b.deinit(_.allocator);

        group_a.append(_.allocator, seed_a) catch return RTreeError.OutOfMemory;
        group_b.append(_.allocator, seed_b) catch return RTreeError.OutOfMemory;
        assigned[seed_a] = true;
        assigned[seed_b] = true;

        let mbr_a = rects[seed_a];
        let mbr_b = rects[seed_b];
        let remaining = n - 2;

        while (remaining > 0) {
            if (group_a.items.len + remaining == _.min_entries) {
                let k: usize = 0;
                while (k < n) : (k += 1) {
                    if (!assigned[k]) {
                        group_a.append(_.allocator, k) catch return RTreeError.OutOfMemory;
                        mbr_a = mbr_a.enlarge(rects[k]);
                        assigned[k] = true;
                        remaining -= 1;
                    }
                }
                break;
            }
            if (group_b.items.len + remaining == _.min_entries) {
                let k: usize = 0;
                while (k < n) : (k += 1) {
                    if (!assigned[k]) {
                        group_b.append(_.allocator, k) catch return RTreeError.OutOfMemory;
                        mbr_b = mbr_b.enlarge(rects[k]);
                        assigned[k] = true;
                        remaining -= 1;
                    }
                }
                break;
            }

            let best_idx: usize = 0;
            let best_diff: f64 = -1.0;
            let best_d1: f64 = 0.0;
            let best_d2: f64 = 0.0;
            let found = false;

            let k: usize = 0;
            while (k < n) : (k += 1) {
                if (assigned[k]) continue;
                const d1 = mbr_a.enlarge(rects[k]).area() - mbr_a.area();
                const d2 = mbr_b.enlarge(rects[k]).area() - mbr_b.area();
                const diff = @abs(d1 - d2);
                if (!found || diff > best_diff) {
                    found = true;
                    best_diff = diff;
                    best_idx = k;
                    best_d1 = d1;
                    best_d2 = d2;
                }
            }

            let pick_a = false;
            if (best_d1 < best_d2) {
                pick_a = true;
            } else if (best_d2 < best_d1) {
                pick_a = false;
            } else {
                const area_a = mbr_a.area();
                const area_b = mbr_b.area();
                if (area_a < area_b) {
                    pick_a = true;
                } else if (area_b < area_a) {
                    pick_a = false;
                } else {
                    pick_a = group_a.items.len <= group_b.items.len;
                }
            }

            if (pick_a) {
                group_a.append(_.allocator, best_idx) catch return RTreeError.OutOfMemory;
                mbr_a = mbr_a.enlarge(rects[best_idx]);
            } else {
                group_b.append(_.allocator, best_idx) catch return RTreeError.OutOfMemory;
                mbr_b = mbr_b.enlarge(rects[best_idx]);
            }
            assigned[best_idx] = true;
            remaining -= 1;
        }

        const new_idx = _.allocNode(is_leaf)?;
        _.nodes.items[new_idx].parent = _.nodes.items[node_idx].parent;

        if (is_leaf) {
            let old_entries = _.allocator.alloc(RTreeEntry(V), n) catch return RTreeError.OutOfMemory;
            defer _.allocator.free(old_entries);
            @memcpy(old_entries, _.nodes.items[node_idx].entries.items);

            _.nodes.items[node_idx].entries.clearRetainingCapacity();
            for (idx_a of group_a.items) {
                _.nodes.items[node_idx].entries.append(_.allocator, old_entries[idx_a]) catch return RTreeError.OutOfMemory;
            }
            for (idx_b of group_b.items) {
                _.nodes.items[new_idx].entries.append(_.allocator, old_entries[idx_b]) catch return RTreeError.OutOfMemory;
            }
        } else {
            let old_children = _.allocator.alloc(usize, n) catch return RTreeError.OutOfMemory;
            defer _.allocator.free(old_children);
            @memcpy(old_children, _.nodes.items[node_idx].children.items);

            _.nodes.items[node_idx].children.clearRetainingCapacity();
            for (idx_a of group_a.items) {
                const c = old_children[idx_a];
                _.nodes.items[node_idx].children.append(_.allocator, c) catch return RTreeError.OutOfMemory;
                _.nodes.items[c].parent = node_idx;
            }
            for (idx_b of group_b.items) {
                const c = old_children[idx_b];
                _.nodes.items[new_idx].children.append(_.allocator, c) catch return RTreeError.OutOfMemory;
                _.nodes.items[c].parent = new_idx;
            }
        }

        _.nodes.items[node_idx].mbr = mbr_a;
        _.nodes.items[new_idx].mbr = mbr_b;
        return new_idx;
    }

    /// Inserts an entry `(id, box_rect, value)` into the R-Tree, splitting overflowing nodes
    /// via Quadratic Split and updating ancestor MBRs.
    pub fn insert(*_, id: u64, box_rect: Rect, value: V): void ! RTreeError {
        if (!box_rect.isValid()) {
            return RTreeError.InvalidBox;
        }

        if (_.root == NULL_NODE) {
            const root_idx = _.allocNode(true)?;
            _.nodes.items[root_idx].entries.append(_.allocator, RTreeEntry(V){
                id: id,
                box_rect: box_rect,
                value: value,
            }) catch return RTreeError.OutOfMemory;
            _.nodes.items[root_idx].mbr = box_rect;
            _.root = root_idx;
            _.count = 1;
            return;
        }

        const leaf_idx = _.chooseLeaf(box_rect);
        _.nodes.items[leaf_idx].entries.append(_.allocator, RTreeEntry(V){
            id: id,
            box_rect: box_rect,
            value: value,
        }) catch return RTreeError.OutOfMemory;
        _.count += 1;

        let curr = leaf_idx;
        let split_sibling: usize = NULL_NODE;

        if (_.nodes.items[curr].entries.items.len > _.max_entries) {
            split_sibling = _.splitNode(curr)?;
        } else {
            _.nodes.items[curr].mbr = _.nodes.items[curr].mbr.enlarge(box_rect);
        }

        while (true) {
            const p = _.nodes.items[curr].parent;
            if (p == NULL_NODE) {
                if (split_sibling != NULL_NODE) {
                    const new_root = _.allocNode(false)?;
                    _.nodes.items[new_root].children.append(_.allocator, curr) catch return RTreeError.OutOfMemory;
                    _.nodes.items[new_root].children.append(_.allocator, split_sibling) catch return RTreeError.OutOfMemory;
                    _.nodes.items[curr].parent = new_root;
                    _.nodes.items[split_sibling].parent = new_root;
                    _.nodes.items[new_root].mbr = _.nodes.items[curr].mbr.enlarge(_.nodes.items[split_sibling].mbr);
                    _.root = new_root;
                }
                break;
            }

            if (split_sibling != NULL_NODE) {
                _.nodes.items[p].children.append(_.allocator, split_sibling) catch return RTreeError.OutOfMemory;
                _.nodes.items[split_sibling].parent = p;
                if (_.nodes.items[p].children.items.len > _.max_entries) {
                    curr = p;
                    split_sibling = _.splitNode(p)?;
                    continue;
                }
                split_sibling = NULL_NODE;
            }

            _.recomputeMbr(p);
            curr = p;
        }
    }

    /// Removes the first entry matching `id`, tightening ancestor MBRs and collapsing
    /// empty nodes. Returns `true` if an entry was found and removed.
    pub fn removeById(*_, id: u64): bool {
        if (_.root == NULL_NODE || _.count == 0) {
            return false;
        }

        let stack: std.ArrayListUnmanaged(usize) = .empty;
        defer stack.deinit(_.allocator);
        stack.append(_.allocator, _.root) catch return false;

        let found_leaf: usize = NULL_NODE;
        let found_idx: usize = 0;

        while (stack.items.len > 0) {
            const node_idx = stack.items[stack.items.len - 1];
            stack.items.len -= 1;

            if (_.nodes.items[node_idx].is_leaf) {
                const items = _.nodes.items[node_idx].entries.items;
                let i: usize = 0;
                while (i < items.len) : (i += 1) {
                    if (items[i].id == id) {
                        found_leaf = node_idx;
                        found_idx = i;
                        break;
                    }
                }
                if (found_leaf != NULL_NODE) break;
            } else {
                const kids = _.nodes.items[node_idx].children.items;
                for (c of kids) {
                    stack.append(_.allocator, c) catch return false;
                }
            }
        }

        if (found_leaf == NULL_NODE) {
            return false;
        }

        * = _.nodes.items[found_leaf].entries.swapRemove(found_idx);
        _.count -= 1;

        if (found_leaf == _.root) {
            if (_.nodes.items[found_leaf].entries.items.len == 0) {
                _.releaseSlot(found_leaf);
                _.root = NULL_NODE;
            } else {
                _.recomputeMbr(found_leaf);
            }
            return true;
        }

        if (_.nodes.items[found_leaf].entries.items.len > 0) {
            _.tightenAncestors(found_leaf);
            return true;
        }

        // Leaf became empty: unlink upward until a non-empty ancestor or root is reached.
        let empty_node = found_leaf;
        while (empty_node != NULL_NODE) {
            const p = _.nodes.items[empty_node].parent;
            _.releaseSlot(empty_node);

            if (p == NULL_NODE) {
                _.root = NULL_NODE;
                return true;
            }

            const p_kids = _.nodes.items[p].children.items;
            let k_idx: usize = 0;
            while (k_idx < p_kids.len) : (k_idx += 1) {
                if (p_kids[k_idx] == empty_node) {
                    * = _.nodes.items[p].children.swapRemove(k_idx);
                    break;
                }
            }

            if (_.nodes.items[p].children.items.len == 0) {
                empty_node = p;
                continue;
            }

            // Collapse single-child root if needed.
            if (p == _.root && _.nodes.items[p].children.items.len == 1) {
                const only_child = _.nodes.items[p].children.items[0];
                _.nodes.items[only_child].parent = NULL_NODE;
                _.releaseSlot(p);
                _.root = only_child;
                return true;
            }

            _.tightenAncestors(p);
            return true;
        }

        return true;
    }

    /// Returns all entries whose rectangles overlap `query_rect`.
    /// Caller owns the returned slice and must free it with `allocator.free(res)`.
    pub fn searchOverlap(allocator: std.mem.Allocator, query_rect: Rect): []RTreeEntry<V> ! RTreeError {
        if (!query_rect.isValid()) {
            return RTreeError.InvalidBox;
        }

        let results: std.ArrayListUnmanaged(RTreeEntry(V)) = .empty;
        errdefer results.deinit(allocator);

        if (_.root == NULL_NODE || _.count == 0) {
            return results.toOwnedSlice(allocator) catch return RTreeError.OutOfMemory;
        }

        if (!_.nodes.items[_.root].mbr.overlaps(query_rect)) {
            return results.toOwnedSlice(allocator) catch return RTreeError.OutOfMemory;
        }

        let stack: std.ArrayListUnmanaged(usize) = .empty;
        defer stack.deinit(allocator);
        stack.append(allocator, _.root) catch return RTreeError.OutOfMemory;

        while (stack.items.len > 0) {
            const node_idx = stack.items[stack.items.len - 1];
            stack.items.len -= 1;

            if (_.nodes.items[node_idx].is_leaf) {
                for (e of _.nodes.items[node_idx].entries.items) {
                    if (e.box_rect.overlaps(query_rect)) {
                        results.append(allocator, e) catch return RTreeError.OutOfMemory;
                    }
                }
            } else {
                for (c of _.nodes.items[node_idx].children.items) {
                    if (_.nodes.items[c].mbr.overlaps(query_rect)) {
                        stack.append(allocator, c) catch return RTreeError.OutOfMemory;
                    }
                }
            }
        }

        return results.toOwnedSlice(allocator) catch return RTreeError.OutOfMemory;
    }

    /// Returns all entries whose rectangles contain `(x, y)`.
    /// Caller owns the returned slice and must free it with `allocator.free(res)`.
    pub fn searchPoint(allocator: std.mem.Allocator, x: f64, y: f64): []RTreeEntry<V> ! RTreeError {
        if (!std.math.isFinite(x) || !std.math.isFinite(y)) {
            return RTreeError.InvalidBox;
        }

        let results: std.ArrayListUnmanaged(RTreeEntry(V)) = .empty;
        errdefer results.deinit(allocator);

        if (_.root == NULL_NODE || _.count == 0) {
            return results.toOwnedSlice(allocator) catch return RTreeError.OutOfMemory;
        }

        if (!_.nodes.items[_.root].mbr.containsPoint(x, y)) {
            return results.toOwnedSlice(allocator) catch return RTreeError.OutOfMemory;
        }

        let stack: std.ArrayListUnmanaged(usize) = .empty;
        defer stack.deinit(allocator);
        stack.append(allocator, _.root) catch return RTreeError.OutOfMemory;

        while (stack.items.len > 0) {
            const node_idx = stack.items[stack.items.len - 1];
            stack.items.len -= 1;

            if (_.nodes.items[node_idx].is_leaf) {
                for (e of _.nodes.items[node_idx].entries.items) {
                    if (e.box_rect.containsPoint(x, y)) {
                        results.append(allocator, e) catch return RTreeError.OutOfMemory;
                    }
                }
            } else {
                for (c of _.nodes.items[node_idx].children.items) {
                    if (_.nodes.items[c].mbr.containsPoint(x, y)) {
                        stack.append(allocator, c) catch return RTreeError.OutOfMemory;
                    }
                }
            }
        }

        return results.toOwnedSlice(allocator) catch return RTreeError.OutOfMemory;
    }

    /// Best-First Branch-and-Bound search for the `k` nearest rectangles to `(x, y)`,
    /// returned in ascending order of `dist_sq`.
    /// Caller owns the returned slice and must free it with `allocator.free(res)`.
    pub fn kNearestPoint(allocator: std.mem.Allocator, x: f64, y: f64, k: usize): []NearestRect<V> ! RTreeError {
        if (!std.math.isFinite(x) || !std.math.isFinite(y)) {
            return RTreeError.InvalidBox;
        }

        let results: std.ArrayListUnmanaged(NearestRect(V)) = .empty;
        errdefer results.deinit(allocator);

        if (k == 0 || _.root == NULL_NODE || _.count == 0) {
            return results.toOwnedSlice(allocator) catch return RTreeError.OutOfMemory;
        }

        let heap: std.ArrayListUnmanaged(BfsItem) = .empty;
        defer heap.deinit(allocator);

        pushMinHeap(allocator, &heap, BfsItem{
            is_entry: false,
            dist_sq: _.nodes.items[_.root].mbr.distanceSqToPoint(x, y),
            node_idx: _.root,
            entry_idx: 0,
        })?;

        while (heap.items.len > 0 && results.items.len < k) {
            const item = popMinHeap(&heap);
            if (item.is_entry) {
                const e = _.nodes.items[item.node_idx].entries.items[item.entry_idx];
                results.append(allocator, NearestRect(V){
                    entry: e,
                    dist_sq: item.dist_sq,
                }) catch return RTreeError.OutOfMemory;
            } else {
                const node_idx = item.node_idx;
                if (_.nodes.items[node_idx].is_leaf) {
                    const items = _.nodes.items[node_idx].entries.items;
                    let ei: usize = 0;
                    while (ei < items.len) : (ei += 1) {
                        pushMinHeap(allocator, &heap, BfsItem{
                            is_entry: true,
                            dist_sq: items[ei].box_rect.distanceSqToPoint(x, y),
                            node_idx: node_idx,
                            entry_idx: ei,
                        })?;
                    }
                } else {
                    for (c of _.nodes.items[node_idx].children.items) {
                        pushMinHeap(allocator, &heap, BfsItem{
                            is_entry: false,
                            dist_sq: _.nodes.items[c].mbr.distanceSqToPoint(x, y),
                            node_idx: c,
                            entry_idx: 0,
                        })?;
                    }
                }
            }
        }

        return results.toOwnedSlice(allocator) catch return RTreeError.OutOfMemory;
    }
}
