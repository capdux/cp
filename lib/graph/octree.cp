// Origin: Pure-CP Stdlib (Zero-Alloc 3D Octree Spatial Index: AABB/Sphere Queries, Pruned Nearest-Neighbor & Slab Ray Traversal) | Created: 2026-11-22

/// Sentinel index indicating no child node or end of an entry linked list.
pub const OCTREE_NO_INDEX: i32 = -1;

/// Default maximum points per leaf before subdivision.
pub const OCTREE_DEFAULT_LEAF_CAP: usize = 8;

/// Default maximum tree subdivision depth.
pub const OCTREE_DEFAULT_MAX_DEPTH: u8 = 8;

/// Hard upper bound on subdivision depth so fixed-size iterative traversal stacks never overflow.
pub const OCTREE_MAX_DEPTH_LIMIT: u8 = 16;

const TRAVERSAL_STACK_CAP: usize = 128;

/// 3D floating-point vector.
pub struct Vec3 {
    x: f64,
    y: f64,
    z: f64,

    static fn init(x: f64, y: f64, z: f64): Vec3 {
        return Vec3 { x: x, y: y, z: z };
    }

    fn add(other: Vec3): Vec3 {
        return Vec3 {
            x: _.x + other.x,
            y: _.y + other.y,
            z: _.z + other.z,
        };
    }

    fn sub(other: Vec3): Vec3 {
        return Vec3 {
            x: _.x - other.x,
            y: _.y - other.y,
            z: _.z - other.z,
        };
    }

    fn scale(t: f64): Vec3 {
        return Vec3 {
            x: _.x * t,
            y: _.y * t,
            z: _.z * t,
        };
    }

    fn dot(other: Vec3): f64 {
        return _.x * other.x + _.y * other.y + _.z * other.z;
    }

    fn distanceSq(other: Vec3): f64 {
        const dx = _.x - other.x;
        const dy = _.y - other.y;
        const dz = _.z - other.z;
        return dx * dx + dy * dy + dz * dz;
    }
}

/// 3D axis-aligned bounding box (`min <= max` per axis).
pub struct Aabb3 {
    min: Vec3,
    max: Vec3,

    /// Constructs a validated AABB; returns `null` if any `min` coordinate exceeds `max`.
    static fn init(min: Vec3, max: Vec3): Aabb3? {
        if (min.x > max.x || min.y > max.y || min.z > max.z) {
            return null;
        }
        return Aabb3 { min: min, max: max };
    }

    /// Returns the geometric center of the box.
    fn center(): Vec3 {
        return Vec3 {
            x: (_.min.x + _.max.x) * 0.5,
            y: (_.min.y + _.max.y) * 0.5,
            z: (_.min.z + _.max.z) * 0.5,
        };
    }

    /// Inclusive containment test (`min <= p <= max`).
    fn containsPoint(p: Vec3): bool {
        return p.x >= _.min.x && p.x <= _.max.x &&
            p.y >= _.min.y && p.y <= _.max.y &&
            p.z >= _.min.z && p.z <= _.max.z;
    }

    /// Inclusive overlap test against another AABB.
    fn intersectsAabb(other: Aabb3): bool {
        return _.min.x <= other.max.x && _.max.x >= other.min.x &&
            _.min.y <= other.max.y && _.max.y >= other.min.y &&
            _.min.z <= other.max.z && _.max.z >= other.min.z;
    }

    /// Squared Euclidean distance from `p` to the closest point on or inside this AABB (`0.0` if inside).
    fn distanceSqToPoint(p: Vec3): f64 {
        let dx: f64 = 0.0;
        if (p.x < _.min.x) {
            dx = _.min.x - p.x;
        } else if (p.x > _.max.x) {
            dx = p.x - _.max.x;
        }

        let dy: f64 = 0.0;
        if (p.y < _.min.y) {
            dy = _.min.y - p.y;
        } else if (p.y > _.max.y) {
            dy = p.y - _.max.y;
        }

        let dz: f64 = 0.0;
        if (p.z < _.min.z) {
            dz = _.min.z - p.z;
        } else if (p.z > _.max.z) {
            dz = p.z - _.max.z;
        }

        return dx * dx + dy * dy + dz * dz;
    }

    /// Maps point `p` to one of 8 octants (`0..7`): bit0=`x>=cx`, bit1=`y>=cy`, bit2=`z>=cz`.
    fn octantIndex(p: Vec3): usize {
        const c = _.center();
        let idx: usize = 0;
        if (p.x >= c.x) {
            idx |= 1;
        }
        if (p.y >= c.y) {
            idx |= 2;
        }
        if (p.z >= c.z) {
            idx |= 4;
        }
        return idx;
    }

    /// Returns the sub-box for child octant `0..7`.
    fn childAabb(octant: usize): Aabb3 {
        const c = _.center();
        const min_x = if ((octant & 1) != 0) c.x else _.min.x;
        const max_x = if ((octant & 1) != 0) _.max.x else c.x;
        const min_y = if ((octant & 2) != 0) c.y else _.min.y;
        const max_y = if ((octant & 2) != 0) _.max.y else c.y;
        const min_z = if ((octant & 4) != 0) c.z else _.min.z;
        const max_z = if ((octant & 4) != 0) _.max.z else c.z;
        return Aabb3 {
            min: Vec3 { x: min_x, y: min_y, z: min_z },
            max: Vec3 { x: max_x, y: max_y, z: max_z },
        };
    }

    /// Returns an expanded copy inflated by `radius` on all 6 faces.
    fn expanded(radius: f64): Aabb3 {
        return Aabb3 {
            min: Vec3 { x: _.min.x - radius, y: _.min.y - radius, z: _.min.z - radius },
            max: Vec3 { x: _.max.x + radius, y: _.max.y + radius, z: _.max.z + radius },
        };
    }
}

/// 3D ray defined by `origin + t * dir` (`t >= 0`).
pub struct Ray3 {
    origin: Vec3,
    dir: Vec3,

    static fn init(origin: Vec3, dir: Vec3): Ray3 {
        return Ray3 { origin: origin, dir: dir };
    }

    fn pointAt(t: f64): Vec3 {
        return Vec3 {
            x: _.origin.x + _.dir.x * t,
            y: _.origin.y + _.dir.y * t,
            z: _.origin.z + _.dir.z * t,
        };
    }
}

/// Parametric intersection interval `[t_min, t_max]` returned by `rayIntersectsAabb`.
pub struct RayHitRange {
    t_min: f64,
    t_max: f64,
}

/// Slab-method Ray-AABB intersection test clamped to `[0.0, max_t]`.
/// Returns `RayHitRange { t_min, t_max }` on hit, or `null` if no intersection exists.
pub fn rayIntersectsAabb(ray: Ray3, box3: Aabb3, max_t: f64): RayHitRange? {
    if (max_t < 0.0) {
        return null;
    }

    let tmin: f64 = 0.0;
    let tmax: f64 = max_t;

    // X slab
    if (ray.dir.x == 0.0) {
        if (ray.origin.x < box3.min.x || ray.origin.x > box3.max.x) {
            return null;
        }
    } else {
        const inv_x = 1.0 / ray.dir.x;
        let t1 = (box3.min.x - ray.origin.x) * inv_x;
        let t2 = (box3.max.x - ray.origin.x) * inv_x;
        if (t1 > t2) {
            const tmp = t1;
            t1 = t2;
            t2 = tmp;
        }
        if (t1 > tmin) {
            tmin = t1;
        }
        if (t2 < tmax) {
            tmax = t2;
        }
        if (tmin > tmax) {
            return null;
        }
    }

    // Y slab
    if (ray.dir.y == 0.0) {
        if (ray.origin.y < box3.min.y || ray.origin.y > box3.max.y) {
            return null;
        }
    } else {
        const inv_y = 1.0 / ray.dir.y;
        let t1 = (box3.min.y - ray.origin.y) * inv_y;
        let t2 = (box3.max.y - ray.origin.y) * inv_y;
        if (t1 > t2) {
            const tmp = t1;
            t1 = t2;
            t2 = tmp;
        }
        if (t1 > tmin) {
            tmin = t1;
        }
        if (t2 < tmax) {
            tmax = t2;
        }
        if (tmin > tmax) {
            return null;
        }
    }

    // Z slab
    if (ray.dir.z == 0.0) {
        if (ray.origin.z < box3.min.z || ray.origin.z > box3.max.z) {
            return null;
        }
    } else {
        const inv_z = 1.0 / ray.dir.z;
        let t1 = (box3.min.z - ray.origin.z) * inv_z;
        let t2 = (box3.max.z - ray.origin.z) * inv_z;
        if (t1 > t2) {
            const tmp = t1;
            t1 = t2;
            t2 = tmp;
        }
        if (t1 > tmin) {
            tmin = t1;
        }
        if (t2 < tmax) {
            tmax = t2;
        }
        if (tmin > tmax) {
            return null;
        }
    }

    return RayHitRange { t_min: tmin, t_max: tmax };
}

/// 3D point payload stored inside the octree.
pub struct OctreePoint {
    pos: Vec3,
    id: u32,

    static fn init(x: f64, y: f64, z: f64, id: u32): OctreePoint {
        return OctreePoint {
            pos: Vec3 { x: x, y: y, z: z },
            id: id,
        };
    }
}

/// Intrusive linked-list slot in the caller-owned entry pool.
pub struct OctreeEntry {
    point: OctreePoint,
    next: i32,
}

/// Fixed-layout octree node stored in the caller-owned node pool.
pub struct OctreeNode {
    bounds: Aabb3,
    children: [8]i32,
    head_entry: i32,
    entry_count: usize,
    depth: u8,
    is_leaf: bool,
}

/// Result returned by `Octree.nearestPoint`.
pub struct NearestResult {
    point: OctreePoint,
    distance_sq: f64,
}

fn makeEmptyLeaf(bounds: Aabb3, depth: u8): OctreeNode {
    return OctreeNode {
        bounds: bounds,
        children: [
            OCTREE_NO_INDEX,
            OCTREE_NO_INDEX,
            OCTREE_NO_INDEX,
            OCTREE_NO_INDEX,
            OCTREE_NO_INDEX,
            OCTREE_NO_INDEX,
            OCTREE_NO_INDEX,
            OCTREE_NO_INDEX,
        ],
        head_entry: OCTREE_NO_INDEX,
        entry_count: 0,
        depth: depth,
        is_leaf: true,
    };
}

/// Zero-allocation 3D Octree backed by caller-owned node and entry pool slices.
pub struct Octree {
    nodes: []OctreeNode,
    entries: []OctreeEntry,
    node_count: usize,
    entry_count: usize,
    leaf_capacity: usize,
    max_depth: u8,

    /// Initializes an octree covering `bounds` using caller-supplied `nodes` and `entries` pools.
    /// Returns `null` if `bounds` is inverted, pools are empty, or `leaf_capacity == 0`.
    fn init(
        bounds: Aabb3,
        nodes: []OctreeNode,
        entries: []OctreeEntry,
        leaf_capacity: usize,
        max_depth: u8,
    ): Octree? {
        if (
            bounds.min.x > bounds.max.x ||
            bounds.min.y > bounds.max.y ||
            bounds.min.z > bounds.max.z
        ) {
            return null;
        }
        if (nodes.len == 0 || entries.len == 0 || leaf_capacity == 0) {
            return null;
        }

        const clamped_depth = if (max_depth > OCTREE_MAX_DEPTH_LIMIT) OCTREE_MAX_DEPTH_LIMIT else max_depth;
        nodes[0] = makeEmptyLeaf(bounds, 0);

        return Octree {
            nodes: nodes,
            entries: entries,
            node_count: 1,
            entry_count: 0,
            leaf_capacity: leaf_capacity,
            max_depth: clamped_depth,
        };
    }

    /// Returns the total number of points currently stored.
    fn count(): usize {
        return _.entry_count;
    }

    /// Returns the number of allocated nodes in the node pool.
    fn nodeCount(): usize {
        return _.node_count;
    }

    /// Clears all points and subdivisions while preserving the root bounding box.
    fn clear(*_) {
        if (_.nodes.len == 0) {
            return;
        }
        const root_bounds = _.nodes[0].bounds;
        _.nodes[0] = makeEmptyLeaf(root_bounds, 0);
        _.node_count = 1;
        _.entry_count = 0;
    }

    fn subdivideLeaf(*_, node_idx: usize): bool {
        if (_.node_count + 8 > _.nodes.len) {
            return false;
        }

        const parent_bounds = _.nodes[node_idx].bounds;
        const child_depth = _.nodes[node_idx].depth + 1;
        const base = _.node_count;

        let oct: usize = 0;
        while (oct < 8) {
            _.nodes[base + oct] = makeEmptyLeaf(parent_bounds.childAabb(oct), child_depth);
            _.nodes[node_idx].children[oct] = (base + oct) as i32;
            oct += 1;
        }
        _.node_count += 8;

        // Redistribute existing leaf entries into the 8 children in-place (zero extra entry slots)
        let cur_entry = _.nodes[node_idx].head_entry;
        _.nodes[node_idx].head_entry = OCTREE_NO_INDEX;
        _.nodes[node_idx].entry_count = 0;
        _.nodes[node_idx].is_leaf = false;

        while (cur_entry != OCTREE_NO_INDEX) {
            const ei = cur_entry as usize;
            const next_entry = _.entries[ei].next;
            const target_oct = parent_bounds.octantIndex(_.entries[ei].point.pos);
            const child_idx = _.nodes[node_idx].children[target_oct] as usize;

            _.entries[ei].next = _.nodes[child_idx].head_entry;
            _.nodes[child_idx].head_entry = cur_entry;
            _.nodes[child_idx].entry_count += 1;

            cur_entry = next_entry;
        }

        return true;
    }

    /// Inserts a 3D point into the octree, automatically subdividing leaves that exceed `leaf_capacity`.
    /// Returns `false` if the point lies outside root bounds or if the entry pool is full.
    fn insert(*_, point: OctreePoint): bool {
        if (_.node_count == 0 || _.entry_count >= _.entries.len) {
            return false;
        }
        if (!_.nodes[0].bounds.containsPoint(point.pos)) {
            return false;
        }

        let cur: usize = 0;
        while (!_.nodes[cur].is_leaf) {
            const oct = _.nodes[cur].bounds.octantIndex(point.pos);
            const child = _.nodes[cur].children[oct];
            if (child == OCTREE_NO_INDEX) {
                return false;
            }
            cur = child as usize;
        }

        // Allocate entry from static pool and prepend to leaf list
        const ei = _.entry_count;
        _.entries[ei] = OctreeEntry {
            point: point,
            next: _.nodes[cur].head_entry,
        };
        _.nodes[cur].head_entry = ei as i32;
        _.nodes[cur].entry_count += 1;
        _.entry_count += 1;

        // Subdivide if leaf exceeds capacity and depth/node-pool allow
        while (
            _.nodes[cur].is_leaf &&
            _.nodes[cur].entry_count > _.leaf_capacity &&
            _.nodes[cur].depth < _.max_depth
        ) {
            if (!_.subdivideLeaf(cur)) {
                break;
            }
            const oct = _.nodes[cur].bounds.octantIndex(point.pos);
            cur = _.nodes[cur].children[oct] as usize;
        }

        return true;
    }

    /// Queries all points inside `query` AABB, writing matching IDs into `out_ids` and returning count written.
    fn queryBox(query: Aabb3, out_ids: *[]u32): usize {
        if (_.node_count == 0 || out_ids.*.len == 0) {
            return 0;
        }
        if (!_.nodes[0].bounds.intersectsAabb(query)) {
            return 0;
        }

        let stack: [TRAVERSAL_STACK_CAP]usize = [0; TRAVERSAL_STACK_CAP];
        let sp: usize = 0;
        stack[sp] = 0;
        sp += 1;

        let written: usize = 0;
        while (sp > 0) {
            sp -= 1;
            const ni = stack[sp];
            const node = _.nodes[ni];

            if (!node.bounds.intersectsAabb(query)) {
                continue;
            }

            if (node.is_leaf) {
                let cur_e = node.head_entry;
                while (cur_e != OCTREE_NO_INDEX) {
                    const e = _.entries[cur_e as usize];
                    if (query.containsPoint(e.point.pos)) {
                        if (written < out_ids.*.len) {
                            out_ids.*[written] = e.point.id;
                            written += 1;
                        }
                        if (written == out_ids.*.len) {
                            return written;
                        }
                    }
                    cur_e = e.next;
                }
            } else {
                let oct: usize = 0;
                while (oct < 8) {
                    const ci = node.children[oct];
                    if (ci != OCTREE_NO_INDEX) {
                        const cu = ci as usize;
                        if (_.nodes[cu].bounds.intersectsAabb(query) && sp < TRAVERSAL_STACK_CAP) {
                            stack[sp] = cu;
                            sp += 1;
                        }
                    }
                    oct += 1;
                }
            }
        }

        return written;
    }

    /// Queries all points within `radius` of `center`, writing matching IDs into `out_ids` and returning count written.
    fn querySphere(center: Vec3, radius: f64, out_ids: *[]u32): usize {
        if (_.node_count == 0 || radius < 0.0 || out_ids.*.len == 0) {
            return 0;
        }
        const r_sq = radius * radius;
        if (_.nodes[0].bounds.distanceSqToPoint(center) > r_sq) {
            return 0;
        }

        let stack: [TRAVERSAL_STACK_CAP]usize = [0; TRAVERSAL_STACK_CAP];
        let sp: usize = 0;
        stack[sp] = 0;
        sp += 1;

        let written: usize = 0;
        while (sp > 0) {
            sp -= 1;
            const ni = stack[sp];
            const node = _.nodes[ni];

            if (node.bounds.distanceSqToPoint(center) > r_sq) {
                continue;
            }

            if (node.is_leaf) {
                let cur_e = node.head_entry;
                while (cur_e != OCTREE_NO_INDEX) {
                    const e = _.entries[cur_e as usize];
                    if (e.point.pos.distanceSq(center) <= r_sq) {
                        if (written < out_ids.*.len) {
                            out_ids.*[written] = e.point.id;
                            written += 1;
                        }
                        if (written == out_ids.*.len) {
                            return written;
                        }
                    }
                    cur_e = e.next;
                }
            } else {
                let oct: usize = 0;
                while (oct < 8) {
                    const ci = node.children[oct];
                    if (ci != OCTREE_NO_INDEX) {
                        const cu = ci as usize;
                        if (_.nodes[cu].bounds.distanceSqToPoint(center) <= r_sq && sp < TRAVERSAL_STACK_CAP) {
                            stack[sp] = cu;
                            sp += 1;
                        }
                    }
                    oct += 1;
                }
            }
        }

        return written;
    }

    /// Finds the single closest point to `target` using best-first child ordering and AABB lower-bound pruning.
    /// Returns `null` if the octree is empty.
    fn nearestPoint(target: Vec3): NearestResult? {
        if (_.node_count == 0 || _.entry_count == 0) {
            return null;
        }

        let stack_node: [TRAVERSAL_STACK_CAP]usize = [0; TRAVERSAL_STACK_CAP];
        let stack_dist: [TRAVERSAL_STACK_CAP]f64 = [0.0; TRAVERSAL_STACK_CAP];
        let sp: usize = 0;
        stack_node[sp] = 0;
        stack_dist[sp] = _.nodes[0].bounds.distanceSqToPoint(target);
        sp += 1;

        let has_best = false;
        let best_dist_sq: f64 = 0.0;
        let best_point = OctreePoint {
            pos: Vec3 { x: 0.0, y: 0.0, z: 0.0 },
            id: 0,
        };

        while (sp > 0) {
            sp -= 1;
            const ni = stack_node[sp];
            const lb_dist = stack_dist[sp];

            if (has_best && lb_dist >= best_dist_sq) {
                continue;
            }

            const node = _.nodes[ni];
            if (node.is_leaf) {
                let cur_e = node.head_entry;
                while (cur_e != OCTREE_NO_INDEX) {
                    const e = _.entries[cur_e as usize];
                    const d_sq = e.point.pos.distanceSq(target);
                    if (!has_best || d_sq < best_dist_sq) {
                        has_best = true;
                        best_dist_sq = d_sq;
                        best_point = e.point;
                    }
                    cur_e = e.next;
                }
            } else {
                // Gather candidate children and sort descending by lower-bound distance so closest pops first
                let cand_idx: [8]usize = [0; 8];
                let cand_dist: [8]f64 = [0.0; 8];
                let cand_n: usize = 0;

                let oct: usize = 0;
                while (oct < 8) {
                    const ci = node.children[oct];
                    if (ci != OCTREE_NO_INDEX) {
                        const cu = ci as usize;
                        const d = _.nodes[cu].bounds.distanceSqToPoint(target);
                        if (!has_best || d < best_dist_sq) {
                            cand_idx[cand_n] = cu;
                            cand_dist[cand_n] = d;
                            cand_n += 1;
                        }
                    }
                    oct += 1;
                }

                // Insertion sort descending by distance so nearest child sits on top of stack
                let i: usize = 1;
                while (i < cand_n) {
                    const key_i = cand_idx[i];
                    const key_d = cand_dist[i];
                    let j: usize = i;
                    while (j > 0 && cand_dist[j - 1] < key_d) {
                        cand_idx[j] = cand_idx[j - 1];
                        cand_dist[j] = cand_dist[j - 1];
                        j -= 1;
                    }
                    cand_idx[j] = key_i;
                    cand_dist[j] = key_d;
                    i += 1;
                }

                let k: usize = 0;
                while (k < cand_n && sp < TRAVERSAL_STACK_CAP) {
                    stack_node[sp] = cand_idx[k];
                    stack_dist[sp] = cand_dist[k];
                    sp += 1;
                    k += 1;
                }
            }
        }

        if (!has_best) {
            return null;
        }
        return NearestResult {
            point: best_point,
            distance_sq: best_dist_sq,
        };
    }

    /// Queries points within `point_radius` of ray segment `ray.origin + t * ray.dir` (`0 <= t <= max_t`).
    /// Uses inflated AABB Slab intersection tests to prune non-intersecting subtrees.
    fn queryRay(ray: Ray3, max_t: f64, point_radius: f64, out_ids: *[]u32): usize {
        if (_.node_count == 0 || max_t < 0.0 || point_radius < 0.0 || out_ids.*.len == 0) {
            return 0;
        }
        const dir_len_sq = ray.dir.dot(ray.dir);
        if (dir_len_sq == 0.0) {
            return 0;
        }
        if (rayIntersectsAabb(ray, _.nodes[0].bounds.expanded(point_radius), max_t) == null) {
            return 0;
        }

        const r_sq = point_radius * point_radius;
        let stack: [TRAVERSAL_STACK_CAP]usize = [0; TRAVERSAL_STACK_CAP];
        let sp: usize = 0;
        stack[sp] = 0;
        sp += 1;

        let written: usize = 0;
        while (sp > 0) {
            sp -= 1;
            const ni = stack[sp];
            const node = _.nodes[ni];

            if (rayIntersectsAabb(ray, node.bounds.expanded(point_radius), max_t) == null) {
                continue;
            }

            if (node.is_leaf) {
                let cur_e = node.head_entry;
                while (cur_e != OCTREE_NO_INDEX) {
                    const e = _.entries[cur_e as usize];
                    const to_pt = e.point.pos.sub(ray.origin);
                    let t = to_pt.dot(ray.dir) / dir_len_sq;
                    if (t < 0.0) {
                        t = 0.0;
                    } else if (t > max_t) {
                        t = max_t;
                    }
                    const closest = ray.pointAt(t);
                    if (e.point.pos.distanceSq(closest) <= r_sq) {
                        if (written < out_ids.*.len) {
                            out_ids.*[written] = e.point.id;
                            written += 1;
                        }
                        if (written == out_ids.*.len) {
                            return written;
                        }
                    }
                    cur_e = e.next;
                }
            } else {
                let oct: usize = 0;
                while (oct < 8) {
                    const ci = node.children[oct];
                    if (ci != OCTREE_NO_INDEX) {
                        const cu = ci as usize;
                        if (
                            rayIntersectsAabb(ray, _.nodes[cu].bounds.expanded(point_radius), max_t) != null &&
                            sp < TRAVERSAL_STACK_CAP
                        ) {
                            stack[sp] = cu;
                            sp += 1;
                        }
                    }
                    oct += 1;
                }
            }
        }

        return written;
    }

    /// Collects all candidate point IDs from every leaf node whose AABB is intersected by `ray` within `[0, max_t]`.
    fn queryRayLeaves(ray: Ray3, max_t: f64, out_ids: *[]u32): usize {
        if (_.node_count == 0 || max_t < 0.0 || out_ids.*.len == 0) {
            return 0;
        }
        if (rayIntersectsAabb(ray, _.nodes[0].bounds, max_t) == null) {
            return 0;
        }

        let stack: [TRAVERSAL_STACK_CAP]usize = [0; TRAVERSAL_STACK_CAP];
        let sp: usize = 0;
        stack[sp] = 0;
        sp += 1;

        let written: usize = 0;
        while (sp > 0) {
            sp -= 1;
            const ni = stack[sp];
            const node = _.nodes[ni];

            if (rayIntersectsAabb(ray, node.bounds, max_t) == null) {
                continue;
            }

            if (node.is_leaf) {
                let cur_e = node.head_entry;
                while (cur_e != OCTREE_NO_INDEX) {
                    const e = _.entries[cur_e as usize];
                    if (written < out_ids.*.len) {
                        out_ids.*[written] = e.point.id;
                        written += 1;
                    }
                    if (written == out_ids.*.len) {
                        return written;
                    }
                    cur_e = e.next;
                }
            } else {
                let oct: usize = 0;
                while (oct < 8) {
                    const ci = node.children[oct];
                    if (ci != OCTREE_NO_INDEX) {
                        const cu = ci as usize;
                        if (rayIntersectsAabb(ray, _.nodes[cu].bounds, max_t) != null && sp < TRAVERSAL_STACK_CAP) {
                            stack[sp] = cu;
                            sp += 1;
                        }
                    }
                    oct += 1;
                }
            }
        }

        return written;
    }
}
