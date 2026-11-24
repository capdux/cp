// Source: cp/lib/kdtree/kdtree.cp (Multi-Dimensional K-D Tree Nearest Neighbor, K-NN & Range Search)
// Pure CP, zero-C-dependency K-D Tree spatial index supporting arbitrary dimensions (dims >= 1),
// variance-based median-split balanced construction, dynamic incremental insertion,
// pruned exact nearest neighbor, max-heap K-NN, spherical radius search, and AABB range search.

const std = @import("std");

const NULL_NODE: usize = std.math.maxInt(usize);

/// Error set for `cap:kdtree` operations.
pub err KdError {
    InvalidDimension,
    DimensionMismatch,
    InvalidRadius,
    InvalidBounds,
    EmptyTree,
    OutOfMemory,
}

/// Point entry stored in or returned from a `KdTree<V>`.
pub struct KdPointEntry<V> {
    pub coords: []const f64;
    pub value: V;
}

/// Nearest-neighbor or radius-search result carrying squared and Euclidean distances.
pub struct KdNeighbor<V> {
    pub coords: []const f64;
    pub value: V;
    pub dist_sq: f64;
    pub dist: f64;
}

struct KdNode<V> {
    coords: []f64;
    value: V;
    split_dim: usize;
    left: usize = NULL_NODE;
    right: usize = NULL_NODE;
}

struct KnnCand {
    node_idx: usize;
    dist_sq: f64;
}

fn squaredDist(a: []const f64, b: []const f64): f64 {
    let sum: f64 = 0.0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        const d = a[i] - b[i];
        sum += d * d;
    }
    return sum;
}

fn pointInBox(coords: []const f64, min_bounds: []const f64, max_bounds: []const f64): bool {
    let i: usize = 0;
    while (i < coords.len) : (i += 1) {
        const c = coords[i];
        if (c < min_bounds[i] || c > max_bounds[i]) {
            return false;
        }
    }
    return true;
}

fn pushMaxHeap(alloc: std.mem.Allocator, heap: *std.ArrayListUnmanaged(KnnCand), item: KnnCand): void ! KdError {
    heap.append(alloc, item) catch return KdError.OutOfMemory;
    let idx = heap.items.len - 1;
    while (idx > 0) {
        const parent = (idx - 1) / 2;
        if (heap.items[idx].dist_sq > heap.items[parent].dist_sq) {
            const tmp = heap.items[idx];
            heap.items[idx] = heap.items[parent];
            heap.items[parent] = tmp;
            idx = parent;
        } else {
            break;
        }
    }
}

fn siftDownMaxHeap(heap: *std.ArrayListUnmanaged(KnnCand)) {
    const n = heap.items.len;
    let idx: usize = 0;
    while (true) {
        const left = idx * 2 + 1;
        const right = idx * 2 + 2;
        let largest = idx;
        if (left < n && heap.items[left].dist_sq > heap.items[largest].dist_sq) {
            largest = left;
        }
        if (right < n && heap.items[right].dist_sq > heap.items[largest].dist_sq) {
            largest = right;
        }
        if (largest == idx) break;
        const tmp = heap.items[idx];
        heap.items[idx] = heap.items[largest];
        heap.items[largest] = tmp;
        idx = largest;
    }
}

/// Multi-dimensional K-D Tree spatial index supporting exact nearest-neighbor,
/// K-NN, spherical radius, and axis-aligned hyperrectangle (AABB) range queries.
pub struct KdTree<V> {
    allocator: std.mem.Allocator;
    dims_val: usize;
    nodes: std.ArrayListUnmanaged(KdNode(V)) = .empty;
    root: usize = NULL_NODE;

    /// Initializes an empty `KdTree<V>` for points of dimensionality `dim_count >= 1`.
    pub static fn initEmpty(allocator: std.mem.Allocator, dim_count: usize): KdTree<V> ! KdError {
        if (dim_count == 0) return KdError.InvalidDimension;
        return KdTree<V>{
            allocator: allocator,
            dims_val: dim_count,
            nodes: .empty,
            root: NULL_NODE,
        };
    }

    /// Builds a balanced `KdTree<V>` from `entries` using variance-based dimension selection
    /// and median partitioning.
    pub static fn build(allocator: std.mem.Allocator, dim_count: usize, entries: []const KdPointEntry<V>): KdTree<V> ! KdError {
        if (dim_count == 0) return KdError.InvalidDimension;
        for (e of entries) {
            if (e.coords.len != dim_count) return KdError.DimensionMismatch;
        }

        let tree = KdTree<V>{
            allocator: allocator,
            dims_val: dim_count,
            nodes: .empty,
            root: NULL_NODE,
        };
        if (entries.len == 0) {
            return tree;
        }

        errdefer tree.deinit();

        tree.nodes.ensureTotalCapacity(allocator, entries.len) catch return KdError.OutOfMemory;

        let indices = allocator.alloc(usize, entries.len) catch return KdError.OutOfMemory;
        defer allocator.free(indices);
        let i: usize = 0;
        while (i < entries.len) : (i += 1) {
            indices[i] = i;
        }

        tree.root = tree.buildSubtree(entries, indices, 0, entries.len, 0)?;
        return tree;
    }

    /// Releases all node storage and owned coordinate buffers.
    pub fn deinit(*_) {
        for (node of _.nodes.items) {
            _.allocator.free(node.coords);
        }
        _.nodes.deinit(_.allocator);
        _.root = NULL_NODE;
    }

    fn chooseSplitDim(entries: []const KdPointEntry<V>, indices: []const usize, lo: usize, hi: usize, depth: usize): usize {
        const count = hi - lo;
        if (count <= 1) return depth % _.dims_val;

        let best_dim: usize = depth % _.dims_val;
        let best_var: f64 = -1.0;
        const n_f64 = f64(count);

        let d: usize = 0;
        while (d < _.dims_val) : (d += 1) {
            let sum: f64 = 0.0;
            let k = lo;
            while (k < hi) : (k += 1) {
                sum += entries[indices[k]].coords[d];
            }
            const mean = sum / n_f64;
            let var_sum: f64 = 0.0;
            k = lo;
            while (k < hi) : (k += 1) {
                const diff = entries[indices[k]].coords[d] - mean;
                var_sum += diff * diff;
            }
            if (var_sum > best_var) {
                best_var = var_sum;
                best_dim = d;
            }
        }
        return best_dim;
    }

    static fn partitionMedian(entries: []const KdPointEntry<V>, indices: []usize, lo: usize, hi: usize, target: usize, dim: usize) {
        let left = lo;
        let right = hi - 1;
        while (left < right) {
            const mid = left + (right - left) / 2;
            const pivot_val = entries[indices[mid]].coords[dim];

            const tmp_m = indices[mid];
            indices[mid] = indices[right];
            indices[right] = tmp_m;

            let store = left;
            let i = left;
            while (i < right) : (i += 1) {
                if (entries[indices[i]].coords[dim] < pivot_val) {
                    const tmp = indices[store];
                    indices[store] = indices[i];
                    indices[i] = tmp;
                    store += 1;
                }
            }
            const tmp_r = indices[store];
            indices[store] = indices[right];
            indices[right] = tmp_r;

            if (store == target) {
                return;
            } else if (store < target) {
                left = store + 1;
            } else {
                if (store == 0) return;
                right = store - 1;
            }
        }
    }

    fn buildSubtree(*_, entries: []const KdPointEntry<V>, indices: []usize, lo: usize, hi: usize, depth: usize): usize ! KdError {
        if (lo >= hi) return NULL_NODE;

        const split_dim = _.chooseSplitDim(entries, indices, lo, hi, depth);
        const mid = lo + (hi - lo) / 2;
        if (hi - lo > 1) {
            KdTree<V>.partitionMedian(entries, indices, lo, hi, mid, split_dim);
        }

        const entry = entries[indices[mid]];
        const coords_copy = _.allocator.dupe(f64, entry.coords) catch return KdError.OutOfMemory;
        errdefer _.allocator.free(coords_copy);

        const node_idx = _.nodes.items.len;
        _.nodes.append(_.allocator, KdNode<V>{
            coords: coords_copy,
            value: entry.value,
            split_dim: split_dim,
            left: NULL_NODE,
            right: NULL_NODE,
        }) catch return KdError.OutOfMemory;

        const left_child = _.buildSubtree(entries, indices, lo, mid, depth + 1)?;
        const right_child = _.buildSubtree(entries, indices, mid + 1, hi, depth + 1)?;

        _.nodes.items[node_idx].left = left_child;
        _.nodes.items[node_idx].right = right_child;
        return node_idx;
    }

    /// Dynamically inserts a point `(coords, value)` into the K-D Tree.
    pub fn insert(*_, coords: []const f64, value: V): void ! KdError {
        if (coords.len != _.dims_val) return KdError.DimensionMismatch;

        const coords_copy = _.allocator.dupe(f64, coords) catch return KdError.OutOfMemory;
        errdefer _.allocator.free(coords_copy);

        const new_idx = _.nodes.items.len;

        if (_.root == NULL_NODE) {
            _.nodes.append(_.allocator, KdNode<V>{
                coords: coords_copy,
                value: value,
                split_dim: 0,
                left: NULL_NODE,
                right: NULL_NODE,
            }) catch return KdError.OutOfMemory;
            _.root = new_idx;
            return;
        }

        let curr = _.root;
        let parent = NULL_NODE;
        let go_left = true;
        let next_split_dim: usize = 0;

        while (curr != NULL_NODE) {
            parent = curr;
            const node = _.nodes.items[curr];
            const s = node.split_dim;
            next_split_dim = (s + 1) % _.dims_val;
            if (coords[s] < node.coords[s]) {
                go_left = true;
                curr = node.left;
            } else {
                go_left = false;
                curr = node.right;
            }
        }

        _.nodes.append(_.allocator, KdNode<V>{
            coords: coords_copy,
            value: value,
            split_dim: next_split_dim,
            left: NULL_NODE,
            right: NULL_NODE,
        }) catch return KdError.OutOfMemory;

        if (go_left) {
            _.nodes.items[parent].left = new_idx;
        } else {
            _.nodes.items[parent].right = new_idx;
        }
    }

    fn nearestRec(node_idx: usize, query: []const f64, best_idx: *usize, best_dist_sq: *f64) {
        if (node_idx == NULL_NODE) return;

        const node = _.nodes.items[node_idx];
        const d_sq = squaredDist(node.coords, query);
        if (d_sq < best_dist_sq.*) {
            best_dist_sq.* = d_sq;
            best_idx.* = node_idx;
        }

        const diff = query[node.split_dim] - node.coords[node.split_dim];
        const near_child = if (diff < 0.0) node.left else node.right;
        const far_child = if (diff < 0.0) node.right else node.left;

        _.nearestRec(near_child, query, best_idx, best_dist_sq);
        if (diff * diff <= best_dist_sq.*) {
            _.nearestRec(far_child, query, best_idx, best_dist_sq);
        }
    }

    /// Finds the exact nearest neighbor to `query`, or `null` if the tree is empty.
    pub fn nearest(query: []const f64): KdNeighbor<V>? ! KdError {
        if (query.len != _.dims_val) return KdError.DimensionMismatch;
        if (_.root == NULL_NODE) return null;

        let best_idx: usize = NULL_NODE;
        let best_dist_sq: f64 = std.math.inf(f64);
        _.nearestRec(_.root, query, *best_idx, *best_dist_sq);

        if (best_idx == NULL_NODE) return null;
        const node = _.nodes.items[best_idx];
        return KdNeighbor<V>{
            coords: node.coords,
            value: node.value,
            dist_sq: best_dist_sq,
            dist: std.math.sqrt(best_dist_sq),
        };
    }

    fn kNearestRec(
        alloc: std.mem.Allocator,
        node_idx: usize,
        query: []const f64,
        k: usize,
        heap: *std.ArrayListUnmanaged(KnnCand),
    ): void ! KdError {
        if (node_idx == NULL_NODE) return;

        const node = _.nodes.items[node_idx];
        const d_sq = squaredDist(node.coords, query);

        if (heap.items.len < k) {
            pushMaxHeap(alloc, heap, KnnCand{ node_idx: node_idx, dist_sq: d_sq })?;
        } else if (d_sq < heap.items[0].dist_sq) {
            heap.items[0] = KnnCand{ node_idx: node_idx, dist_sq: d_sq };
            siftDownMaxHeap(heap);
        }

        const diff = query[node.split_dim] - node.coords[node.split_dim];
        const near_child = if (diff < 0.0) node.left else node.right;
        const far_child = if (diff < 0.0) node.right else node.left;

        _.kNearestRec(alloc, near_child, query, k, heap)?;

        const tau = if (heap.items.len < k) std.math.inf(f64) else heap.items[0].dist_sq;
        if (diff * diff <= tau) {
            _.kNearestRec(alloc, far_child, query, k, heap)?;
        }
    }

    static fn sortNeighborsAsc(items: []KdNeighbor<V>) {
        if (items.len <= 1) return;
        let i: usize = 1;
        while (i < items.len) : (i += 1) {
            const key = items[i];
            let j = i;
            while (j > 0 && items[j - 1].dist_sq > key.dist_sq) : (j -= 1) {
                items[j] = items[j - 1];
            }
            items[j] = key;
        }
    }

    /// Finds up to `k` exact nearest neighbors to `query`, sorted by ascending distance.
    /// Caller owns the returned slice and must free it with `allocator.free`.
    pub fn kNearest(allocator: std.mem.Allocator, query: []const f64, k: usize): []KdNeighbor<V> ! KdError {
        if (query.len != _.dims_val) return KdError.DimensionMismatch;
        if (_.root == NULL_NODE || k == 0) {
            return allocator.alloc(KdNeighbor(V), 0) catch return KdError.OutOfMemory;
        }

        let heap: std.ArrayListUnmanaged(KnnCand) = .empty;
        defer heap.deinit(allocator);

        _.kNearestRec(allocator, _.root, query, k, *heap)?;

        let out = allocator.alloc(KdNeighbor(V), heap.items.len) catch return KdError.OutOfMemory;
        let i: usize = 0;
        while (i < heap.items.len) : (i += 1) {
            const cand = heap.items[i];
            const node = _.nodes.items[cand.node_idx];
            out[i] = KdNeighbor<V>{
                coords: node.coords,
                value: node.value,
                dist_sq: cand.dist_sq,
                dist: std.math.sqrt(cand.dist_sq),
            };
        }
        KdTree<V>.sortNeighborsAsc(out);
        return out;
    }

    fn withinRadiusRec(
        alloc: std.mem.Allocator,
        node_idx: usize,
        query: []const f64,
        r_sq: f64,
        out: *std.ArrayListUnmanaged(KdNeighbor(V)),
    ): void ! KdError {
        if (node_idx == NULL_NODE) return;

        const node = _.nodes.items[node_idx];
        const d_sq = squaredDist(node.coords, query);
        if (d_sq <= r_sq) {
            out.append(alloc, KdNeighbor<V>{
                coords: node.coords,
                value: node.value,
                dist_sq: d_sq,
                dist: std.math.sqrt(d_sq),
            }) catch return KdError.OutOfMemory;
        }

        const diff = query[node.split_dim] - node.coords[node.split_dim];
        const near_child = if (diff < 0.0) node.left else node.right;
        const far_child = if (diff < 0.0) node.right else node.left;

        _.withinRadiusRec(alloc, near_child, query, r_sq, out)?;
        if (diff * diff <= r_sq) {
            _.withinRadiusRec(alloc, far_child, query, r_sq, out)?;
        }
    }

    /// Finds all points within Euclidean `radius` of `query`, sorted by ascending distance.
    /// Caller owns the returned slice and must free it with `allocator.free`.
    pub fn withinRadius(allocator: std.mem.Allocator, query: []const f64, radius: f64): []KdNeighbor<V> ! KdError {
        if (query.len != _.dims_val) return KdError.DimensionMismatch;
        if (!std.math.isFinite(radius) || radius < 0.0) return KdError.InvalidRadius;
        if (_.root == NULL_NODE) {
            return allocator.alloc(KdNeighbor(V), 0) catch return KdError.OutOfMemory;
        }

        const r_sq = radius * radius;
        let list: std.ArrayListUnmanaged(KdNeighbor(V)) = .empty;
        errdefer list.deinit(allocator);

        _.withinRadiusRec(allocator, _.root, query, r_sq, *list)?;
        const out = list.toOwnedSlice(allocator) catch return KdError.OutOfMemory;
        KdTree<V>.sortNeighborsAsc(out);
        return out;
    }

    fn rangeSearchRec(
        alloc: std.mem.Allocator,
        node_idx: usize,
        min_bounds: []const f64,
        max_bounds: []const f64,
        out: *std.ArrayListUnmanaged(KdPointEntry(V)),
    ): void ! KdError {
        if (node_idx == NULL_NODE) return;

        const node = _.nodes.items[node_idx];
        if (pointInBox(node.coords, min_bounds, max_bounds)) {
            out.append(alloc, KdPointEntry<V>{
                coords: node.coords,
                value: node.value,
            }) catch return KdError.OutOfMemory;
        }

        const s = node.split_dim;
        const val = node.coords[s];
        if (min_bounds[s] <= val && node.left != NULL_NODE) {
            _.rangeSearchRec(alloc, node.left, min_bounds, max_bounds, out)?;
        }
        if (max_bounds[s] >= val && node.right != NULL_NODE) {
            _.rangeSearchRec(alloc, node.right, min_bounds, max_bounds, out)?;
        }
    }

    /// Finds all points inside the axis-aligned hyperrectangle `[min_bounds, max_bounds]`.
    /// Caller owns the returned slice and must free it with `allocator.free`.
    pub fn rangeSearch(allocator: std.mem.Allocator, min_bounds: []const f64, max_bounds: []const f64): []KdPointEntry<V> ! KdError {
        if (min_bounds.len != _.dims_val || max_bounds.len != _.dims_val) {
            return KdError.DimensionMismatch;
        }
        let d: usize = 0;
        while (d < _.dims_val) : (d += 1) {
            const lo = min_bounds[d];
            const hi = max_bounds[d];
            if (!std.math.isFinite(lo) || !std.math.isFinite(hi) || lo > hi) {
                return KdError.InvalidBounds;
            }
        }
        if (_.root == NULL_NODE) {
            return allocator.alloc(KdPointEntry(V), 0) catch return KdError.OutOfMemory;
        }

        let list: std.ArrayListUnmanaged(KdPointEntry(V)) = .empty;
        errdefer list.deinit(allocator);

        _.rangeSearchRec(allocator, _.root, min_bounds, max_bounds, *list)?;
        return list.toOwnedSlice(allocator) catch return KdError.OutOfMemory;
    }

    /// Returns the total number of points stored in the tree.
    pub fn len(): usize {
        return _.nodes.items.len;
    }

    /// Returns the dimensionality of points in the tree.
    pub fn dims(): usize {
        return _.dims_val;
    }

    /// Returns `true` if the tree contains no points.
    pub fn isEmpty(): bool {
        return _.nodes.items.len == 0;
    }
}
