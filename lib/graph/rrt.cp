// Origin: Pure-CP Stdlib (Zero-Alloc 2D Sampling-Based Motion Planning: RRT, RRT* & Line-of-Sight Path Smoothing) | Created: 2026-11-22

const SPLITMIX_INC: u64 = 0x9E3779B97F4A7C15;
const INV_2_POW_53: f64 = 1.0 / 9007199254740992.0;

/// Sentinel parent index for the root node of an RRT / RRT* tree.
pub const RRT_NO_PARENT: u32 = 0xFFFFFFFF;

/// 2D Euclidean point.
pub struct Point2 {
    x: f64,
    y: f64,
}

/// Axis-aligned 2D planning workspace bounds `[min_x, max_x] x [min_y, max_y]`.
pub struct Bounds2 {
    min_x: f64,
    min_y: f64,
    max_x: f64,
    max_y: f64,
}

/// Circular 2D obstacle centered at `(cx, cy)` with `radius >= 0`.
pub struct CircleObstacle {
    cx: f64,
    cy: f64,
    radius: f64,
}

/// Axis-aligned rectangular 2D obstacle `[min_x, max_x] x [min_y, max_y]`.
pub struct RectObstacle {
    min_x: f64,
    min_y: f64,
    max_x: f64,
    max_y: f64,
}

/// Tree node stored in caller-provided zero-alloc node pool.
pub struct RrtNode {
    pos: Point2,
    parent: u32,
    cost: f64,
}

/// Planning configuration for `planRrt2D` (supports both standard RRT and RRT*).
pub struct RrtConfig {
    step_size: f64,
    goal_tolerance: f64,
    goal_bias: f64,
    rewire_radius: f64,
    max_iterations: usize,
    seed: u64,
    use_rrt_star: bool,

    /// Balanced defaults for standard RRT (`use_rrt_star = false`).
    fn defaultRrt(): RrtConfig {
        return RrtConfig {
            step_size: 0.5,
            goal_tolerance: 0.5,
            goal_bias: 0.1,
            rewire_radius: 0.0,
            max_iterations: 2000,
            seed: 42,
            use_rrt_star: false,
        };
    }

    /// Balanced defaults for asymptotically optimal RRT* (`use_rrt_star = true`).
    fn defaultRrtStar(): RrtConfig {
        return RrtConfig {
            step_size: 0.5,
            goal_tolerance: 0.5,
            goal_bias: 0.1,
            rewire_radius: 1.5,
            max_iterations: 2000,
            seed: 42,
            use_rrt_star: true,
        };
    }
}

/// Summary returned by `planRrt2D`.
pub struct RrtResult {
    found: bool,
    node_count: usize,
    path_len: usize,
    total_cost: f64,
    iterations: usize,
}

struct RngF64 {
    val: f64,
    next_state: u64,
}

fn isFiniteF64(v: f64): bool {
    return (v == v) && ((v - v) == 0.0);
}

fn absF64(v: f64): f64 {
    return if (v < 0.0) -v else v;
}

fn clamp01(v: f64): f64 {
    if (v < 0.0) {
        return 0.0;
    }
    if (v > 1.0) {
        return 1.0;
    }
    return v;
}

fn distSq2(a: Point2, b: Point2): f64 {
    const dx = b.x - a.x;
    const dy = b.y - a.y;
    return dx * dx + dy * dy;
}

fn dist2(a: Point2, b: Point2): f64 {
    return @sqrt(distSq2(a, b));
}

fn pointFinite(p: Point2): bool {
    return isFiniteF64(p.x) && isFiniteF64(p.y);
}

fn pointInBounds(p: Point2, b: Bounds2): bool {
    return p.x >= b.min_x && p.x <= b.max_x && p.y >= b.min_y && p.y <= b.max_y;
}

fn splitmixMix(x: u64): u64 {
    let z: u64 = x;
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    return z ^ (z >> 31);
}

fn nextUniform01(state: u64): RngF64 {
    const ns: u64 = state +% SPLITMIX_INC;
    const raw: u64 = splitmixMix(ns);
    return RngF64 {
        val: ((raw >> 11) as f64) * INV_2_POW_53,
        next_state: ns,
    };
}

/// Returns true if point `p` lies inside or on the boundary of circle `c`.
pub fn pointInCircle(p: Point2, c: CircleObstacle): bool {
    if (!pointFinite(p) || !isFiniteF64(c.cx) || !isFiniteF64(c.cy) || !isFiniteF64(c.radius) || c.radius < 0.0) {
        return false;
    }
    const dx = p.x - c.cx;
    const dy = p.y - c.cy;
    return (dx * dx + dy * dy) <= (c.radius * c.radius);
}

/// Returns true if point `p` lies inside or on the boundary of axis-aligned rectangle `r`.
pub fn pointInRect(p: Point2, r: RectObstacle): bool {
    if (!pointFinite(p) || !isFiniteF64(r.min_x) || !isFiniteF64(r.min_y) || !isFiniteF64(r.max_x) || !isFiniteF64(r.max_y)) {
        return false;
    }
    return p.x >= r.min_x && p.x <= r.max_x && p.y >= r.min_y && p.y <= r.max_y;
}

/// Returns true if closed segment `[a, b]` intersects circle `c`.
pub fn segmentIntersectsCircle(a: Point2, b: Point2, c: CircleObstacle): bool {
    if (!pointFinite(a) || !pointFinite(b) || !isFiniteF64(c.cx) || !isFiniteF64(c.cy) || !isFiniteF64(c.radius) || c.radius < 0.0) {
        return false;
    }
    const abx = b.x - a.x;
    const aby = b.y - a.y;
    const len2 = abx * abx + aby * aby;
    if (len2 <= 1.0e-30) {
        return pointInCircle(a, c);
    }
    const t = clamp01(((c.cx - a.x) * abx + (c.cy - a.y) * aby) / len2);
    const closest_x = a.x + t * abx;
    const closest_y = a.y + t * aby;
    const dx = closest_x - c.cx;
    const dy = closest_y - c.cy;
    return (dx * dx + dy * dy) <= (c.radius * c.radius);
}

/// Returns true if closed segment `[a, b]` intersects axis-aligned rectangle `r` (Liang-Barsky slab test).
pub fn segmentIntersectsRect(a: Point2, b: Point2, r: RectObstacle): bool {
    if (!pointFinite(a) || !pointFinite(b) || !isFiniteF64(r.min_x) || !isFiniteF64(r.min_y) || !isFiniteF64(r.max_x) || !isFiniteF64(r.max_y)) {
        return false;
    }
    if (r.min_x > r.max_x || r.min_y > r.max_y) {
        return false;
    }

    let t_min: f64 = 0.0;
    let t_max: f64 = 1.0;

    // X slab
    const dx = b.x - a.x;
    if (absF64(dx) <= 1.0e-15) {
        if (a.x < r.min_x || a.x > r.max_x) {
            return false;
        }
    } else {
        const inv_dx = 1.0 / dx;
        let t1 = (r.min_x - a.x) * inv_dx;
        let t2 = (r.max_x - a.x) * inv_dx;
        if (t1 > t2) {
            const tmp = t1;
            t1 = t2;
            t2 = tmp;
        }
        if (t1 > t_min) {
            t_min = t1;
        }
        if (t2 < t_max) {
            t_max = t2;
        }
        if (t_min > t_max) {
            return false;
        }
    }

    // Y slab
    const dy = b.y - a.y;
    if (absF64(dy) <= 1.0e-15) {
        if (a.y < r.min_y || a.y > r.max_y) {
            return false;
        }
    } else {
        const inv_dy = 1.0 / dy;
        let t1 = (r.min_y - a.y) * inv_dy;
        let t2 = (r.max_y - a.y) * inv_dy;
        if (t1 > t2) {
            const tmp = t1;
            t1 = t2;
            t2 = tmp;
        }
        if (t1 > t_min) {
            t_min = t1;
        }
        if (t2 < t_max) {
            t_max = t2;
        }
        if (t_min > t_max) {
            return false;
        }
    }

    return t_min <= t_max;
}

/// Returns true iff segment `[a, b]` is finite and does not intersect any circle or rectangle obstacle.
pub fn isSegmentCollisionFree(
    a: Point2,
    b: Point2,
    circles: []const CircleObstacle,
    rects: []const RectObstacle,
): bool {
    if (!pointFinite(a) || !pointFinite(b)) {
        return false;
    }
    let i: usize = 0;
    while (i < circles.len) {
        if (segmentIntersectsCircle(a, b, circles[i])) {
            return false;
        }
        i += 1;
    }
    i = 0;
    while (i < rects.len) {
        if (segmentIntersectsRect(a, b, rects[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isPointCollisionFree(
    p: Point2,
    circles: []const CircleObstacle,
    rects: []const RectObstacle,
): bool {
    if (!pointFinite(p)) {
        return false;
    }
    let i: usize = 0;
    while (i < circles.len) {
        if (pointInCircle(p, circles[i])) {
            return false;
        }
        i += 1;
    }
    i = 0;
    while (i < rects.len) {
        if (pointInRect(p, rects[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Computes total Euclidean length of polyline `path`.
pub fn pathLength(path: []const Point2): f64 {
    if (path.len < 2) {
        return 0.0;
    }
    let total: f64 = 0.0;
    let i: usize = 0;
    while (i + 1 < path.len) {
        total += dist2(path[i], path[i + 1]);
        i += 1;
    }
    return total;
}

/// Greedy line-of-sight shortcut path smoother.
/// Writes a strictly collision-free simplified polyline into `out_path` and returns its vertex count.
pub fn smoothPath(
    path: []const Point2,
    circles: []const CircleObstacle,
    rects: []const RectObstacle,
    out_path: *[]Point2,
): usize {
    if (path.len == 0 || out_path.*.len == 0) {
        return 0;
    }
    out_path.*[0] = path[0];
    if (path.len == 1) {
        return 1;
    }

    let out_len: usize = 1;
    let cur: usize = 0;
    while (cur + 1 < path.len) {
        if (out_len >= out_path.*.len) {
            return 0;
        }
        let next_idx: usize = cur + 1;
        let target: usize = path.len - 1;
        while (target > cur + 1) {
            if (isSegmentCollisionFree(path[cur], path[target], circles, rects)) {
                next_idx = target;
                break;
            }
            target -= 1;
        }
        out_path.*[out_len] = path[next_idx];
        out_len += 1;
        cur = next_idx;
    }
    return out_len;
}

fn validateObstacles(circles: []const CircleObstacle, rects: []const RectObstacle): bool {
    let i: usize = 0;
    while (i < circles.len) {
        const c = circles[i];
        if (!isFiniteF64(c.cx) || !isFiniteF64(c.cy) || !isFiniteF64(c.radius) || c.radius < 0.0) {
            return false;
        }
        i += 1;
    }
    i = 0;
    while (i < rects.len) {
        const r = rects[i];
        if (
            !isFiniteF64(r.min_x) ||
            !isFiniteF64(r.min_y) ||
            !isFiniteF64(r.max_x) ||
            !isFiniteF64(r.max_y) ||
            r.min_x > r.max_x ||
            r.min_y > r.max_y
        ) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Propagates cost reductions down the tree after an RRT* rewire operation (zero-alloc).
fn propagateTreeCosts(nodes_pool: *[]RrtNode, node_count: usize) {
    let pass: usize = 0;
    while (pass < node_count) {
        let changed: bool = false;
        let k: usize = 1;
        while (k < node_count) {
            const p = nodes_pool.*[k].parent;
            if (p != RRT_NO_PARENT && (p as usize) < node_count) {
                const new_c = nodes_pool.*[p as usize].cost + dist2(nodes_pool.*[p as usize].pos, nodes_pool.*[k].pos);
                if (new_c + 1.0e-12 < nodes_pool.*[k].cost) {
                    nodes_pool.*[k].cost = new_c;
                    changed = true;
                }
            }
            k += 1;
        }
        if (!changed) {
            break;
        }
        pass += 1;
    }
}

/// 2D sampling-based motion planner supporting standard RRT and asymptotically optimal RRT*.
/// Writes the collision-free path in `start -> ... -> goal` order into `path_out`.
/// Returns `null` on invalid inputs (NaN, invalid bounds/obstacles, start/goal out of bounds or in collision,
/// invalid config, or insufficient buffers). Returns `found == false` when no path is found within budget.
pub fn planRrt2D(
    start: Point2,
    goal: Point2,
    bounds: Bounds2,
    circles: []const CircleObstacle,
    rects: []const RectObstacle,
    cfg: RrtConfig,
    nodes_pool: *[]RrtNode,
    path_out: *[]Point2,
): RrtResult? {
    if (
        !pointFinite(start) ||
        !pointFinite(goal) ||
        !isFiniteF64(bounds.min_x) ||
        !isFiniteF64(bounds.min_y) ||
        !isFiniteF64(bounds.max_x) ||
        !isFiniteF64(bounds.max_y) ||
        bounds.min_x >= bounds.max_x ||
        bounds.min_y >= bounds.max_y
    ) {
        return null;
    }
    if (!pointInBounds(start, bounds) || !pointInBounds(goal, bounds)) {
        return null;
    }
    if (!validateObstacles(circles, rects)) {
        return null;
    }
    if (!isPointCollisionFree(start, circles, rects) || !isPointCollisionFree(goal, circles, rects)) {
        return null;
    }
    if (
        !isFiniteF64(cfg.step_size) ||
        cfg.step_size <= 0.0 ||
        !isFiniteF64(cfg.goal_tolerance) ||
        cfg.goal_tolerance < 0.0 ||
        !isFiniteF64(cfg.goal_bias) ||
        cfg.goal_bias < 0.0 ||
        cfg.goal_bias > 1.0 ||
        !isFiniteF64(cfg.rewire_radius) ||
        cfg.rewire_radius < 0.0
    ) {
        return null;
    }
    if (nodes_pool.*.len < 2 || path_out.*.len < 2) {
        return null;
    }

    // Initialize root node.
    nodes_pool.*[0] = RrtNode {
        pos: start,
        parent: RRT_NO_PARENT,
        cost: 0.0,
    };
    let node_count: usize = 1;

    // Degenerate case: start == goal.
    const direct_dist = dist2(start, goal);
    if (direct_dist <= 1.0e-12) {
        path_out.*[0] = start;
        return RrtResult {
            found: true,
            node_count: 1,
            path_len: 1,
            total_cost: 0.0,
            iterations: 0,
        };
    }

    // Fast path: unobstructed straight line from start to goal.
    if (isSegmentCollisionFree(start, goal, circles, rects)) {
        nodes_pool.*[1] = RrtNode {
            pos: goal,
            parent: 0,
            cost: direct_dist,
        };
        path_out.*[0] = start;
        path_out.*[1] = goal;
        return RrtResult {
            found: true,
            node_count: 2,
            path_len: 2,
            total_cost: direct_dist,
            iterations: 0,
        };
    }

    let rng_state: u64 = cfg.seed;
    let best_goal_idx: u32 = RRT_NO_PARENT;
    let best_goal_cost: f64 = 1.0e308;
    let iterations: usize = 0;

    let iter: usize = 0;
    while (iter < cfg.max_iterations && node_count < nodes_pool.*.len) {
        iterations = iter + 1;

        // 1. Sample random point in bounds (with goal_bias probability of sampling goal).
        const u_bias = nextUniform01(rng_state);
        rng_state = u_bias.next_state;

        let sample = goal;
        if (u_bias.val >= cfg.goal_bias) {
            const ux = nextUniform01(rng_state);
            rng_state = ux.next_state;
            const uy = nextUniform01(rng_state);
            rng_state = uy.next_state;
            sample = Point2 {
                x: bounds.min_x + ux.val * (bounds.max_x - bounds.min_x),
                y: bounds.min_y + uy.val * (bounds.max_y - bounds.min_y),
            };
        }

        // 2. Find nearest node in tree.
        let nearest_idx: usize = 0;
        let nearest_d2: f64 = distSq2(nodes_pool.*[0].pos, sample);
        let k: usize = 1;
        while (k < node_count) {
            const d2 = distSq2(nodes_pool.*[k].pos, sample);
            if (d2 < nearest_d2) {
                nearest_d2 = d2;
                nearest_idx = k;
            }
            k += 1;
        }

        // 3. Steer from nearest node toward sample by at most step_size.
        const nearest_pos = nodes_pool.*[nearest_idx].pos;
        const d_step = @sqrt(nearest_d2);
        if (d_step <= 1.0e-12) {
            iter += 1;
            continue;
        }
        let new_pos = sample;
        if (d_step > cfg.step_size) {
            const scale = cfg.step_size / d_step;
            new_pos = Point2 {
                x: nearest_pos.x + (sample.x - nearest_pos.x) * scale,
                y: nearest_pos.y + (sample.y - nearest_pos.y) * scale,
            };
        }

        if (!isSegmentCollisionFree(nearest_pos, new_pos, circles, rects)) {
            iter += 1;
            continue;
        }

        // 4. Choose parent (nearest for RRT; min-cost collision-free neighbor for RRT*).
        let best_parent: usize = nearest_idx;
        let best_cost: f64 = nodes_pool.*[nearest_idx].cost + dist2(nearest_pos, new_pos);

        if (cfg.use_rrt_star && cfg.rewire_radius > 0.0) {
            const r2 = cfg.rewire_radius * cfg.rewire_radius;
            k = 0;
            while (k < node_count) {
                const d2 = distSq2(nodes_pool.*[k].pos, new_pos);
                if (d2 <= r2) {
                    const cand_cost = nodes_pool.*[k].cost + @sqrt(d2);
                    if (cand_cost + 1.0e-12 < best_cost) {
                        if (isSegmentCollisionFree(nodes_pool.*[k].pos, new_pos, circles, rects)) {
                            best_parent = k;
                            best_cost = cand_cost;
                        }
                    }
                }
                k += 1;
            }
        }

        // 5. Insert new node.
        const new_idx = node_count;
        nodes_pool.*[new_idx] = RrtNode {
            pos: new_pos,
            parent: best_parent as u32,
            cost: best_cost,
        };
        node_count += 1;

        // 6. RRT* rewire neighbors within rewire_radius.
        if (cfg.use_rrt_star && cfg.rewire_radius > 0.0) {
            const r2 = cfg.rewire_radius * cfg.rewire_radius;
            let any_rewired: bool = false;
            k = 0;
            while (k < new_idx) {
                if (k != best_parent) {
                    const d2 = distSq2(new_pos, nodes_pool.*[k].pos);
                    if (d2 <= r2) {
                        const via_new = best_cost + @sqrt(d2);
                        if (via_new + 1.0e-12 < nodes_pool.*[k].cost) {
                            if (isSegmentCollisionFree(new_pos, nodes_pool.*[k].pos, circles, rects)) {
                                nodes_pool.*[k].parent = new_idx as u32;
                                nodes_pool.*[k].cost = via_new;
                                any_rewired = true;
                            }
                        }
                    }
                }
                k += 1;
            }
            if (any_rewired) {
                propagateTreeCosts(nodes_pool, node_count);
                if (best_goal_idx != RRT_NO_PARENT) {
                    best_goal_cost = nodes_pool.*[best_goal_idx as usize].cost +
                        dist2(nodes_pool.*[best_goal_idx as usize].pos, goal);
                }
            }
        }

        // 7. Check goal connection.
        const d_goal = dist2(new_pos, goal);
        if (d_goal <= cfg.goal_tolerance && isSegmentCollisionFree(new_pos, goal, circles, rects)) {
            const total = nodes_pool.*[new_idx].cost + d_goal;
            if (best_goal_idx == RRT_NO_PARENT || total < best_goal_cost) {
                best_goal_idx = new_idx as u32;
                best_goal_cost = total;
            }
            if (!cfg.use_rrt_star) {
                break;
            }
        }

        iter += 1;
    }

    if (best_goal_idx == RRT_NO_PARENT) {
        return RrtResult {
            found: false,
            node_count: node_count,
            path_len: 0,
            total_cost: 0.0,
            iterations: iterations,
        };
    }

    // Reconstruct path from root to best_goal_idx (+ goal if distinct).
    let chain_len: usize = 0;
    let cur: u32 = best_goal_idx;
    while (cur != RRT_NO_PARENT) {
        chain_len += 1;
        if (chain_len > node_count) {
            return null;
        }
        cur = nodes_pool.*[cur as usize].parent;
    }

    const need_goal_append = dist2(nodes_pool.*[best_goal_idx as usize].pos, goal) > 1.0e-12;
    const total_pts = if (need_goal_append) chain_len + 1 else chain_len;
    if (total_pts > path_out.*.len) {
        return null;
    }

    if (need_goal_append) {
        path_out.*[total_pts - 1] = goal;
    }

    let write_pos: usize = chain_len;
    cur = best_goal_idx;
    while (cur != RRT_NO_PARENT) {
        write_pos -= 1;
        path_out.*[write_pos] = nodes_pool.*[cur as usize].pos;
        cur = nodes_pool.*[cur as usize].parent;
    }

    const final_cost = pathLength(path_out.*[0..total_pts]);
    return RrtResult {
        found: true,
        node_count: node_count,
        path_len: total_pts,
        total_cost: final_cost,
        iterations: iterations,
    };
}
