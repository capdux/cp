// @cap/kdtree-2d — Pure CP zero-allocation 2D K-D Tree spatial index
// Balanced in-place median-split construction + iterative explicit-stack NN, rectangle & radius queries.

pub struct KdPt {
    pub x: f64,
    pub y: f64,
    pub id: u32,
}

pub struct KdNode {
    pub pt: KdPt,
    pub left: i32,
    pub right: i32,
    pub axis: u8,
}

pub struct NearestResult {
    pub found: bool,
    pub id: u32,
    pub dist_sq: f64,
}

struct KdBuildFrame {
    lo: usize,
    hi: usize,
    axis: u8,
    parent: i32,
    is_left: bool,
}

struct KdSearchFrame {
    idx: i32,
    min_dist_sq: f64,
}

fn kdCoord(pt: KdPt, axis: u8): f64 {
    if (axis == 0) {
        return pt.x;
    }
    return pt.y;
}

fn kdSwap(pts: []KdPt, i: usize, j: usize) {
    if (i == j) {
        return;
    }
    let tmp = pts[i];
    pts[i] = pts[j];
    pts[j] = tmp;
}

fn kdInsertionSort(pts: []KdPt, lo: usize, hi: usize, axis: u8) {
    let i: usize = lo + 1;
    while (i <= hi) {
        let key = pts[i];
        let key_c = kdCoord(key, axis);
        let j: usize = i;
        while (j > lo && kdCoord(pts[j - 1], axis) > key_c) {
            pts[j] = pts[j - 1];
            j = j - 1;
        }
        pts[j] = key;
        i = i + 1;
    }
}

fn kdSelectMedian(pts: []KdPt, lo_in: usize, hi_in: usize, target: usize, axis: u8) {
    let lo = lo_in;
    let hi = hi_in;
    while (hi > lo) {
        if (hi - lo <= 16) {
            kdInsertionSort(pts, lo, hi, axis);
            return;
        }
        // Median-of-three pivot selection
        let mid = lo + (hi - lo) / 2;
        if (kdCoord(pts[mid], axis) < kdCoord(pts[lo], axis)) {
            kdSwap(pts, lo, mid);
        }
        if (kdCoord(pts[hi], axis) < kdCoord(pts[lo], axis)) {
            kdSwap(pts, lo, hi);
        }
        if (kdCoord(pts[mid], axis) < kdCoord(pts[hi], axis)) {
            kdSwap(pts, mid, hi);
        }
        let pivot = kdCoord(pts[hi], axis);

        let store = lo;
        let i = lo;
        while (i < hi) {
            if (kdCoord(pts[i], axis) < pivot) {
                kdSwap(pts, store, i);
                store = store + 1;
            }
            i = i + 1;
        }
        kdSwap(pts, store, hi);

        if (store == target) {
            return;
        }
        if (store < target) {
            lo = store + 1;
        } else {
            if (store == 0) {
                return;
            }
            hi = store - 1;
        }
    }
}

pub fn buildKdTree(pts: []KdPt, nodes: []KdNode): i32 {
    if (pts.len == 0 || nodes.len < pts.len) {
        return -1;
    }

    let stack: [128]KdBuildFrame = undefined;
    let sp: usize = 0;
    stack[0] = KdBuildFrame {
        lo: 0,
        hi: pts.len - 1,
        axis: 0,
        parent: -1,
        is_left: false,
    };
    sp = 1;

    let root: i32 = -1;
    let next_node: usize = 0;

    while (sp > 0) {
        sp = sp - 1;
        let f = stack[sp];
        let mid = f.lo + (f.hi - f.lo) / 2;
        kdSelectMedian(pts, f.lo, f.hi, mid, f.axis);

        let node_idx = next_node;
        next_node = next_node + 1;
        nodes[node_idx] = KdNode {
            pt: pts[mid],
            left: -1,
            right: -1,
            axis: f.axis,
        };

        if (f.parent < 0) {
            root = node_idx as i32;
        } else {
            let pidx = f.parent as usize;
            let pnode = nodes[pidx];
            if (f.is_left) {
                pnode.left = node_idx as i32;
            } else {
                pnode.right = node_idx as i32;
            }
            nodes[pidx] = pnode;
        }

        let next_axis: u8 = 1;
        if (f.axis == 1) {
            next_axis = 0;
        }

        if (mid + 1 <= f.hi && sp < 128) {
            stack[sp] = KdBuildFrame {
                lo: mid + 1,
                hi: f.hi,
                axis: next_axis,
                parent: node_idx as i32,
                is_left: false,
            };
            sp = sp + 1;
        }
        if (f.lo < mid && sp < 128) {
            stack[sp] = KdBuildFrame {
                lo: f.lo,
                hi: mid - 1,
                axis: next_axis,
                parent: node_idx as i32,
                is_left: true,
            };
            sp = sp + 1;
        }
    }

    return root;
}

pub fn nearestNeighbor(nodes: []KdNode, root: i32, qx: f64, qy: f64): NearestResult {
    if (root < 0 || (root as usize) >= nodes.len) {
        return NearestResult { found: false, id: 0, dist_sq: 0.0 };
    }

    let stack: [128]KdSearchFrame = undefined;
    let sp: usize = 0;
    stack[0] = KdSearchFrame { idx: root, min_dist_sq: 0.0 };
    sp = 1;

    let best_found = false;
    let best_id: u32 = 0;
    let best_dist_sq: f64 = 0.0;

    while (sp > 0) {
        sp = sp - 1;
        let fr = stack[sp];
        if (fr.idx < 0 || (fr.idx as usize) >= nodes.len) {
            continue;
        }
        if (best_found && fr.min_dist_sq >= best_dist_sq) {
            continue;
        }

        let nd = nodes[fr.idx as usize];
        let dx = qx - nd.pt.x;
        let dy = qy - nd.pt.y;
        let d2 = dx * dx + dy * dy;
        if (!best_found || d2 < best_dist_sq) {
            best_found = true;
            best_dist_sq = d2;
            best_id = nd.pt.id;
        }

        let diff: f64 = 0.0;
        if (nd.axis == 0) {
            diff = qx - nd.pt.x;
        } else {
            diff = qy - nd.pt.y;
        }

        let near_child = nd.right;
        let far_child = nd.left;
        if (diff <= 0.0) {
            near_child = nd.left;
            far_child = nd.right;
        }

        let plane_d2 = diff * diff;
        if (far_child >= 0 && (!best_found || plane_d2 < best_dist_sq) && sp < 128) {
            stack[sp] = KdSearchFrame { idx: far_child, min_dist_sq: plane_d2 };
            sp = sp + 1;
        }
        if (near_child >= 0 && sp < 128) {
            stack[sp] = KdSearchFrame { idx: near_child, min_dist_sq: 0.0 };
            sp = sp + 1;
        }
    }

    return NearestResult {
        found: best_found,
        id: best_id,
        dist_sq: best_dist_sq,
    };
}

pub fn rangeQueryRect(
    nodes: []KdNode,
    root: i32,
    min_x: f64,
    min_y: f64,
    max_x: f64,
    max_y: f64,
    out_ids: []u32
): usize {
    if (root < 0 || (root as usize) >= nodes.len || out_ids.len == 0) {
        return 0;
    }
    if (min_x > max_x || min_y > max_y) {
        return 0;
    }

    let stack: [128]i32 = undefined;
    let sp: usize = 0;
    stack[0] = root;
    sp = 1;

    let count: usize = 0;

    while (sp > 0) {
        sp = sp - 1;
        let idx = stack[sp];
        if (idx < 0 || (idx as usize) >= nodes.len) {
            continue;
        }
        let nd = nodes[idx as usize];
        if (nd.pt.x >= min_x && nd.pt.x <= max_x && nd.pt.y >= min_y && nd.pt.y <= max_y) {
            if (count < out_ids.len) {
                out_ids[count] = nd.pt.id;
                count = count + 1;
            }
        }

        if (nd.axis == 0) {
            if (nd.right >= 0 && max_x >= nd.pt.x && sp < 128) {
                stack[sp] = nd.right;
                sp = sp + 1;
            }
            if (nd.left >= 0 && min_x <= nd.pt.x && sp < 128) {
                stack[sp] = nd.left;
                sp = sp + 1;
            }
        } else {
            if (nd.right >= 0 && max_y >= nd.pt.y && sp < 128) {
                stack[sp] = nd.right;
                sp = sp + 1;
            }
            if (nd.left >= 0 && min_y <= nd.pt.y && sp < 128) {
                stack[sp] = nd.left;
                sp = sp + 1;
            }
        }
    }

    return count;
}

pub fn radiusQuery(
    nodes: []KdNode,
    root: i32,
    cx: f64,
    cy: f64,
    radius: f64,
    out_ids: []u32
): usize {
    if (root < 0 || (root as usize) >= nodes.len || out_ids.len == 0 || radius < 0.0) {
        return 0;
    }

    let r2 = radius * radius;
    let min_x = cx - radius;
    let max_x = cx + radius;
    let min_y = cy - radius;
    let max_y = cy + radius;

    let stack: [128]i32 = undefined;
    let sp: usize = 0;
    stack[0] = root;
    sp = 1;

    let count: usize = 0;

    while (sp > 0) {
        sp = sp - 1;
        let idx = stack[sp];
        if (idx < 0 || (idx as usize) >= nodes.len) {
            continue;
        }
        let nd = nodes[idx as usize];
        let dx = nd.pt.x - cx;
        let dy = nd.pt.y - cy;
        if (dx * dx + dy * dy <= r2) {
            if (count < out_ids.len) {
                out_ids[count] = nd.pt.id;
                count = count + 1;
            }
        }

        if (nd.axis == 0) {
            if (nd.right >= 0 && max_x >= nd.pt.x && sp < 128) {
                stack[sp] = nd.right;
                sp = sp + 1;
            }
            if (nd.left >= 0 && min_x <= nd.pt.x && sp < 128) {
                stack[sp] = nd.left;
                sp = sp + 1;
            }
        } else {
            if (nd.right >= 0 && max_y >= nd.pt.y && sp < 128) {
                stack[sp] = nd.right;
                sp = sp + 1;
            }
            if (nd.left >= 0 && min_y <= nd.pt.y && sp < 128) {
                stack[sp] = nd.left;
                sp = sp + 1;
            }
        }
    }

    return count;
}

export {
    KdPt,
    KdNode,
    NearestResult,
    buildKdTree,
    nearestNeighbor,
    rangeQueryRect,
    radiusQuery
};
