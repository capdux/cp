// @cap/bvh — Zero-allocation Static 3D AABB Bounding Volume Hierarchy (BVH)
// Clean-room MIT implementation in pure CP.
// Supports longest-axis median-split BVH construction (up to 128 items / 256 nodes),
// AABB overlap queries, and branch-and-bound slab raycasting.

pub const MAX_ITEMS: usize = 128;
const NONE_IDX: i32 = -1;

fn minF64(a: f64, b: f64) -> f64 {
    if (a < b) {
        return a;
    }
    return b;
}

fn maxF64(a: f64, b: f64) -> f64 {
    if (a > b) {
        return a;
    }
    return b;
}

pub struct Aabb {
    pub min_x: f64,
    pub min_y: f64,
    pub min_z: f64,
    pub max_x: f64,
    pub max_y: f64,
    pub max_z: f64,

    /// Returns the union AABB enclosing both `this` and `other`.
    pub fn merge(other: Aabb) -> Aabb {
        return Aabb {
            min_x: minF64(_.min_x, other.min_x),
            min_y: minF64(_.min_y, other.min_y),
            min_z: minF64(_.min_z, other.min_z),
            max_x: maxF64(_.max_x, other.max_x),
            max_y: maxF64(_.max_y, other.max_y),
            max_z: maxF64(_.max_z, other.max_z),
        };
    }

    /// Returns true if `this` overlaps or touches `other` along all 3 axes.
    pub fn overlaps(other: Aabb) -> bool {
        return _.min_x <= other.max_x
            && _.max_x >= other.min_x
            && _.min_y <= other.max_y
            && _.max_y >= other.min_y
            && _.min_z <= other.max_z
            && _.max_z >= other.min_z;
    }

    /// Returns true if point `(x, y, z)` lies inside or on the boundary of this AABB.
    pub fn containsPoint(x: f64, y: f64, z: f64) -> bool {
        return x >= _.min_x && x <= _.max_x
            && y >= _.min_y && y <= _.max_y
            && z >= _.min_z && z <= _.max_z;
    }

    /// Returns the surface area of this AABB.
    pub fn surfaceArea() -> f64 {
        const ex = maxF64(0.0, _.max_x - _.min_x);
        const ey = maxF64(0.0, _.max_y - _.min_y);
        const ez = maxF64(0.0, _.max_z - _.min_z);
        return 2.0 * (ex * ey + ey * ez + ez * ex);
    }

    /// Slab ray-AABB intersection test for ray `O + t * D` with `0 <= t <= max_t`.
    /// Returns the entry distance `t >= 0.0` on hit, or `-1.0` on miss.
    pub fn rayHit(ox: f64, oy: f64, oz: f64, dx: f64, dy: f64, dz: f64, max_t: f64) -> f64 {
        if (max_t < 0.0) {
            return -1.0;
        }
        let tmin: f64 = 0.0;
        let tmax: f64 = max_t;

        // X slab
        if (dx == 0.0) {
            if (ox < _.min_x || ox > _.max_x) {
                return -1.0;
            }
        } else {
            let tx1 = (_.min_x - ox) / dx;
            let tx2 = (_.max_x - ox) / dx;
            if (tx1 > tx2) {
                const tmp = tx1;
                tx1 = tx2;
                tx2 = tmp;
            }
            if (tx1 > tmin) {
                tmin = tx1;
            }
            if (tx2 < tmax) {
                tmax = tx2;
            }
            if (tmax < tmin) {
                return -1.0;
            }
        }

        // Y slab
        if (dy == 0.0) {
            if (oy < _.min_y || oy > _.max_y) {
                return -1.0;
            }
        } else {
            let ty1 = (_.min_y - oy) / dy;
            let ty2 = (_.max_y - oy) / dy;
            if (ty1 > ty2) {
                const tmp = ty1;
                ty1 = ty2;
                ty2 = tmp;
            }
            if (ty1 > tmin) {
                tmin = ty1;
            }
            if (ty2 < tmax) {
                tmax = ty2;
            }
            if (tmax < tmin) {
                return -1.0;
            }
        }

        // Z slab
        if (dz == 0.0) {
            if (oz < _.min_z || oz > _.max_z) {
                return -1.0;
            }
        } else {
            let tz1 = (_.min_z - oz) / dz;
            let tz2 = (_.max_z - oz) / dz;
            if (tz1 > tz2) {
                const tmp = tz1;
                tz1 = tz2;
                tz2 = tmp;
            }
            if (tz1 > tmin) {
                tmin = tz1;
            }
            if (tz2 < tmax) {
                tmax = tz2;
            }
            if (tmax < tmin) {
                return -1.0;
            }
        }

        return tmin;
    }
}

pub struct BvhItem {
    pub bounds: Aabb,
    pub id: u32,
}

pub struct RayHit {
    pub id: u32,
    pub t: f64,
    pub hit: bool,
}

struct BvhNode {
    bounds: Aabb,
    left: i32,
    right: i32,
    item_id: u32,
    is_leaf: bool,
}

struct BuildTask {
    lo: usize,
    hi: usize,
    slot: usize,
}

struct RayFrame {
    idx: i32,
    t_enter: f64,
}

fn centroidOnAxis(b: Aabb, axis: u8) -> f64 {
    if (axis == 0) {
        return 0.5 * (b.min_x + b.max_x);
    }
    if (axis == 1) {
        return 0.5 * (b.min_y + b.max_y);
    }
    return 0.5 * (b.min_z + b.max_z);
}

fn sortItemsByAxis(items: []BvhItem, lo: usize, hi: usize, axis: u8) {
    let i = lo + 1;
    while (i < hi) {
        const key = items[i];
        const key_c = centroidOnAxis(key.bounds, axis);
        let j = i;
        while (j > lo && centroidOnAxis(items[j - 1].bounds, axis) > key_c) {
            items[j] = items[j - 1];
            j -= 1;
        }
        items[j] = key;
        i += 1;
    }
}

pub struct Bvh {
    nodes: [256]BvhNode,
    node_count: usize,
    item_count: usize,

    /// Builds a balanced 3D BVH from `items` (truncates to `MAX_ITEMS = 128`).
    pub static fn build(items: []const BvhItem) -> Bvh {
        let nodes: [256]BvhNode = undefined;
        let n = items.len;
        if (n > MAX_ITEMS) {
            n = MAX_ITEMS;
        }
        if (n == 0) {
            return Bvh {
                nodes: nodes,
                node_count: 0,
                item_count: 0,
            };
        }

        let work: [128]BvhItem = undefined;
        let k: usize = 0;
        while (k < n) {
            work[k] = items[k];
            k += 1;
        }

        let stack: [256]BuildTask = undefined;
        let sp: usize = 0;
        let next_slot: usize = 1;

        stack[sp] = BuildTask { lo: 0, hi: n, slot: 0 };
        sp += 1;

        while (sp > 0) {
            sp -= 1;
            const task = stack[sp];

            // Compute union AABB of [lo, hi)
            let box_u = work[task.lo].bounds;
            let idx = task.lo + 1;
            while (idx < task.hi) {
                box_u = box_u.merge(work[idx].bounds);
                idx += 1;
            }

            if (task.hi - task.lo == 1) {
                nodes[task.slot] = BvhNode {
                    bounds: box_u,
                    left: NONE_IDX,
                    right: NONE_IDX,
                    item_id: work[task.lo].id,
                    is_leaf: true,
                };
            } else {
                const ex = box_u.max_x - box_u.min_x;
                const ey = box_u.max_y - box_u.min_y;
                const ez = box_u.max_z - box_u.min_z;
                let axis: u8 = 0;
                if (ey >= ex && ey >= ez) {
                    axis = 1;
                } else if (ez >= ex && ez >= ey) {
                    axis = 2;
                }

                sortItemsByAxis(work[0..n], task.lo, task.hi, axis);
                const mid = task.lo + (task.hi - task.lo) / 2;

                const lslot = next_slot;
                next_slot += 1;
                const rslot = next_slot;
                next_slot += 1;

                nodes[task.slot] = BvhNode {
                    bounds: box_u,
                    left: lslot as i32,
                    right: rslot as i32,
                    item_id: 0,
                    is_leaf: false,
                };

                stack[sp] = BuildTask { lo: mid, hi: task.hi, slot: rslot };
                sp += 1;
                stack[sp] = BuildTask { lo: task.lo, hi: mid, slot: lslot };
                sp += 1;
            }
        }

        return Bvh {
            nodes: nodes,
            node_count: next_slot,
            item_count: n,
        };
    }

    /// Returns the number of indexed items.
    pub fn len() -> usize {
        return _.item_count;
    }

    /// Returns true if the BVH contains no items.
    pub fn isEmpty() -> bool {
        return _.item_count == 0;
    }

    /// Finds all items whose AABB overlaps `query`.
    /// Writes up to `out_ids.len` matching item IDs into `out_ids` and returns total match count.
    pub fn queryAabb(query: Aabb, out_ids: []u32) -> usize {
        if (_.node_count == 0) {
            return 0;
        }
        let stack: [64]i32 = undefined;
        let sp: usize = 0;
        stack[sp] = 0;
        sp += 1;

        let count: usize = 0;
        while (sp > 0) {
            sp -= 1;
            const node = _.nodes[stack[sp] as usize];
            if (!node.bounds.overlaps(query)) {
                continue;
            }
            if (node.is_leaf) {
                if (count < out_ids.len) {
                    out_ids[count] = node.item_id;
                }
                count += 1;
            } else {
                if (node.right != NONE_IDX) {
                    stack[sp] = node.right;
                    sp += 1;
                }
                if (node.left != NONE_IDX) {
                    stack[sp] = node.left;
                    sp += 1;
                }
            }
        }
        return count;
    }

    /// Casts ray `O + t * D` (`0 <= t <= max_t`) and returns the closest hit item.
    pub fn raycast(ox: f64, oy: f64, oz: f64, dx: f64, dy: f64, dz: f64, max_t: f64) -> RayHit {
        if (_.node_count == 0 || max_t < 0.0) {
            return RayHit { id: 0, t: 0.0, hit: false };
        }
        const root_t = _.nodes[0].bounds.rayHit(ox, oy, oz, dx, dy, dz, max_t);
        if (root_t < 0.0) {
            return RayHit { id: 0, t: 0.0, hit: false };
        }

        let stack: [64]RayFrame = undefined;
        let sp: usize = 0;
        stack[sp] = RayFrame { idx: 0, t_enter: root_t };
        sp += 1;

        let found = false;
        let best_t = max_t;
        let best_id: u32 = 0;

        while (sp > 0) {
            sp -= 1;
            const frame = stack[sp];
            if (found && frame.t_enter > best_t) {
                continue;
            }

            const node = _.nodes[frame.idx as usize];
            if (node.is_leaf) {
                if (!found || frame.t_enter < best_t) {
                    best_t = frame.t_enter;
                    best_id = node.item_id;
                    found = true;
                }
            } else {
                const l_idx = node.left;
                const r_idx = node.right;
                let tl: f64 = -1.0;
                let tr: f64 = -1.0;
                if (l_idx != NONE_IDX) {
                    tl = _.nodes[l_idx as usize].bounds.rayHit(ox, oy, oz, dx, dy, dz, best_t);
                }
                if (r_idx != NONE_IDX) {
                    tr = _.nodes[r_idx as usize].bounds.rayHit(ox, oy, oz, dx, dy, dz, best_t);
                }

                if (tl >= 0.0 && tr >= 0.0) {
                    // Push farther child first so nearer child is popped first
                    if (tl <= tr) {
                        stack[sp] = RayFrame { idx: r_idx, t_enter: tr };
                        sp += 1;
                        stack[sp] = RayFrame { idx: l_idx, t_enter: tl };
                        sp += 1;
                    } else {
                        stack[sp] = RayFrame { idx: l_idx, t_enter: tl };
                        sp += 1;
                        stack[sp] = RayFrame { idx: r_idx, t_enter: tr };
                        sp += 1;
                    }
                } else if (tl >= 0.0) {
                    stack[sp] = RayFrame { idx: l_idx, t_enter: tl };
                    sp += 1;
                } else if (tr >= 0.0) {
                    stack[sp] = RayFrame { idx: r_idx, t_enter: tr };
                    sp += 1;
                }
            }
        }

        return RayHit {
            id: best_id,
            t: best_t,
            hit: found,
        };
    }
}
