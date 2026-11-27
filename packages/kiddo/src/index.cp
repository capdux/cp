// @cap/kd-tree — Zero-allocation Static 2D KD-Tree Spatial Index
// Clean-room MIT implementation in pure CP.
// Supports balanced median-split construction (up to 256 points), exact nearest-neighbor
// branch-and-bound search, and Euclidean radius range queries.

pub const MAX_POINTS: usize = 256;
const NONE_IDX: i32 = -1;

pub struct KdPoint2D {
    pub x: f64,
    pub y: f64,
    pub id: u32,
}

pub struct NearestResult {
    pub id: u32,
    pub dist_sq: f64,
    pub found: bool,
}

struct KdNode {
    x: f64,
    y: f64,
    id: u32,
    axis: u8,
    left: i32,
    right: i32,
}

struct BuildTask {
    lo: usize,
    hi: usize,
    depth: usize,
    slot: usize,
}

struct SearchFrame {
    idx: i32,
    plane_dist_sq: f64,
}

fn coordAt(p: KdPoint2D, axis: u8) -> f64 {
    if (axis == 0) {
        return p.x;
    }
    return p.y;
}

fn sortRangeByAxis(pts: []KdPoint2D, lo: usize, hi: usize, axis: u8) {
    let i = lo + 1;
    while (i < hi) {
        const key = pts[i];
        const key_val = coordAt(key, axis);
        let j = i;
        while (j > lo && coordAt(pts[j - 1], axis) > key_val) {
            pts[j] = pts[j - 1];
            j -= 1;
        }
        pts[j] = key;
        i += 1;
    }
}

pub struct KdTree2D {
    nodes: [256]KdNode,
    node_count: usize,

    /// Builds a balanced 2D KD-Tree from `points` (truncates to `MAX_POINTS = 256`).
    pub static fn build(points: []const KdPoint2D) -> KdTree2D {
        let nodes: [256]KdNode = undefined;
        let n = points.len;
        if (n > MAX_POINTS) {
            n = MAX_POINTS;
        }

        if (n == 0) {
            return KdTree2D {
                nodes: nodes,
                node_count: 0,
            };
        }

        let work: [256]KdPoint2D = undefined;
        let k: usize = 0;
        while (k < n) {
            work[k] = points[k];
            k += 1;
        }

        let stack: [256]BuildTask = undefined;
        let sp: usize = 0;
        let next_slot: usize = 1;

        stack[sp] = BuildTask {
            lo: 0,
            hi: n,
            depth: 0,
            slot: 0,
        };
        sp += 1;

        while (sp > 0) {
            sp -= 1;
            const task = stack[sp];
            const axis: u8 = (task.depth & 1) as u8;
            sortRangeByAxis(work[0..n], task.lo, task.hi, axis);

            const mid = task.lo + (task.hi - task.lo) / 2;
            const pt = work[mid];

            let left_idx: i32 = NONE_IDX;
            let right_idx: i32 = NONE_IDX;

            if (task.lo < mid) {
                const lslot = next_slot;
                next_slot += 1;
                left_idx = lslot as i32;
                stack[sp] = BuildTask {
                    lo: task.lo,
                    hi: mid,
                    depth: task.depth + 1,
                    slot: lslot,
                };
                sp += 1;
            }

            if (mid + 1 < task.hi) {
                const rslot = next_slot;
                next_slot += 1;
                right_idx = rslot as i32;
                stack[sp] = BuildTask {
                    lo: mid + 1,
                    hi: task.hi,
                    depth: task.depth + 1,
                    slot: rslot,
                };
                sp += 1;
            }

            nodes[task.slot] = KdNode {
                x: pt.x,
                y: pt.y,
                id: pt.id,
                axis: axis,
                left: left_idx,
                right: right_idx,
            };
        }

        return KdTree2D {
            nodes: nodes,
            node_count: n,
        };
    }

    /// Returns the number of indexed points.
    pub fn len() -> usize {
        return _.node_count;
    }

    /// Returns true if the tree contains no points.
    pub fn isEmpty() -> bool {
        return _.node_count == 0;
    }

    /// Finds the exact nearest neighbor to `(qx, qy)` using branch-and-bound pruning.
    pub fn nearest(qx: f64, qy: f64) -> NearestResult {
        if (_.node_count == 0) {
            return NearestResult {
                id: 0,
                dist_sq: 0.0,
                found: false,
            };
        }

        let stack: [64]SearchFrame = undefined;
        let sp: usize = 0;
        stack[sp] = SearchFrame { idx: 0, plane_dist_sq: 0.0 };
        sp += 1;

        let has_best = false;
        let best_id: u32 = 0;
        let best_dist_sq: f64 = 0.0;

        while (sp > 0) {
            sp -= 1;
            const frame = stack[sp];
            if (has_best && frame.plane_dist_sq >= best_dist_sq) {
                continue;
            }

            const node = _.nodes[frame.idx as usize];
            const dx = qx - node.x;
            const dy = qy - node.y;
            const d2 = dx * dx + dy * dy;

            if (!has_best || d2 < best_dist_sq) {
                best_dist_sq = d2;
                best_id = node.id;
                has_best = true;
            }

            let diff = dx;
            if (node.axis == 1) {
                diff = dy;
            }

            let near_child = node.left;
            let far_child = node.right;
            if (diff > 0.0) {
                near_child = node.right;
                far_child = node.left;
            }

            const diff_sq = diff * diff;
            if (far_child != NONE_IDX && diff_sq < best_dist_sq) {
                stack[sp] = SearchFrame {
                    idx: far_child,
                    plane_dist_sq: diff_sq,
                };
                sp += 1;
            }
            if (near_child != NONE_IDX) {
                stack[sp] = SearchFrame {
                    idx: near_child,
                    plane_dist_sq: 0.0,
                };
                sp += 1;
            }
        }

        return NearestResult {
            id: best_id,
            dist_sq: best_dist_sq,
            found: has_best,
        };
    }

    /// Finds all points within Euclidean distance `<= radius` from `(qx, qy)`.
    /// Writes up to `out_ids.len` matching IDs into `out_ids` and returns the total match count.
    pub fn withinRadius(qx: f64, qy: f64, radius: f64, out_ids: []u32) -> usize {
        if (_.node_count == 0 || radius < 0.0) {
            return 0;
        }

        const r2 = radius * radius;
        let stack: [64]i32 = undefined;
        let sp: usize = 0;
        stack[sp] = 0;
        sp += 1;

        let count: usize = 0;
        while (sp > 0) {
            sp -= 1;
            const idx = stack[sp];
            const node = _.nodes[idx as usize];
            const dx = qx - node.x;
            const dy = qy - node.y;
            const d2 = dx * dx + dy * dy;

            if (d2 <= r2) {
                if (count < out_ids.len) {
                    out_ids[count] = node.id;
                }
                count += 1;
            }

            let diff = dx;
            if (node.axis == 1) {
                diff = dy;
            }

            if (node.left != NONE_IDX && diff <= radius) {
                stack[sp] = node.left;
                sp += 1;
            }
            if (node.right != NONE_IDX && diff >= -radius) {
                stack[sp] = node.right;
                sp += 1;
            }
        }

        return count;
    }
}
