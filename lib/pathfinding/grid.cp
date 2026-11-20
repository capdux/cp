// Source: cp/lib/pathfinding/grid.cp (2D Grid Map, A* Search, Dijkstra Distance Field & Line-of-Sight Smoothing)
// Pure CP, zero-C-dependency 2D grid pathfinding engine with corner-cutting prevention,
// configurable terrain weights, Dijkstra flow-field distance maps, and supercover Bresenham smoothing.

const std = @import("std");

const SQRT2: f64 = 1.4142135623730950488;
const INVALID_IDX: usize = std.math.maxInt(usize);

/// Error set for 2D grid pathfinding operations.
pub err PathError {
    InvalidDimensions,
    OutOfBounds,
    BlockedEndpoint,
    NoPathFound,
    InvalidCost,
    OutOfMemory,
}

/// Admissible heuristic metrics supported by `GridMap.findPathAStar`.
pub enum HeuristicKind {
    Manhattan = 0,
    Octile = 1,
    Euclidean = 2,
}

/// Discrete 2D grid cell coordinate `(x, y)`.
pub struct GridCoord {
    x: usize = 0;
    y: usize = 0;

    /// Constructs a `GridCoord` from `(x, y)` indices.
    static fn init(x: usize, y: usize): GridCoord {
        return GridCoord { x: x, y: y };
    }

    /// Returns true if both coordinates are equal.
    fn eql(other: GridCoord): bool {
        return _.x == other.x && _.y == other.y;
    }
}

/// Owned pathfinding result returned by `GridMap.findPathAStar`.
pub struct PathResult {
    alloc: std.mem.Allocator = undefined;
    coords: []GridCoord = [];
    total_cost: f64 = 0.0;
    nodes_expanded: usize = 0;

    /// Releases the owned `coords` slice.
    fn deinit(*_) {
        if (_.coords.len > 0) {
            _.alloc.free(_.coords);
        }
        _.coords = [];
        _.total_cost = 0.0;
        _.nodes_expanded = 0;
    }
}

struct HeapNode {
    priority: f64 = 0.0;
    g: f64 = 0.0;
    idx: usize = 0;
}

fn heapLess(a: HeapNode, b: HeapNode): bool {
    if (a.priority != b.priority) return a.priority < b.priority;
    if (a.g != b.g) return a.g > b.g;
    return a.idx < b.idx;
}

fn heapPush(alloc: std.mem.Allocator, heap: *std.ArrayList(HeapNode), item: HeapNode): void ! PathError {
    heap.append(alloc, item) catch return PathError.OutOfMemory;
    let idx = heap.items.len - 1;
    while (idx > 0) {
        const parent = (idx - 1) / 2;
        if (!heapLess(heap.items[idx], heap.items[parent])) break;
        const tmp = heap.items[idx];
        heap.items[idx] = heap.items[parent];
        heap.items[parent] = tmp;
        idx = parent;
    }
}

fn heapPop(heap: *std.ArrayList(HeapNode)): HeapNode {
    const top = heap.items[0];
    const last = heap.pop().?;
    if (heap.items.len > 0) {
        heap.items[0] = last;
        let idx: usize = 0;
        const n = heap.items.len;
        while (true) {
            const left = idx * 2 + 1;
            if (left >= n) break;
            const right = left + 1;
            let best = left;
            if (right < n && heapLess(heap.items[right], heap.items[left])) {
                best = right;
            }
            if (!heapLess(heap.items[best], heap.items[idx])) break;
            const tmp = heap.items[idx];
            heap.items[idx] = heap.items[best];
            heap.items[best] = tmp;
            idx = best;
        }
    }
    return top;
}

fn absDiffF64(a: usize, b: usize): f64 {
    const diff = if (a > b) a - b else b - a;
    return f64(diff);
}

fn computeHeuristic(a: GridCoord, b: GridCoord, kind: HeuristicKind): f64 {
    const dx = absDiffF64(a.x, b.x);
    const dy = absDiffF64(a.y, b.y);
    switch (kind) {
        .Manhattan => return dx + dy,
        .Octile => {
            const lo = if (dx < dy) dx else dy;
            const hi = if (dx > dy) dx else dy;
            return hi + (SQRT2 - 1.0) * lo;
        },
        .Euclidean => return @sqrt(dx * dx + dy * dy),
    }
}

/// 2D uniform-cell navigation grid with obstacle flags and per-cell traversal weights.
pub struct GridMap {
    alloc: std.mem.Allocator = undefined;
    width: usize = 0;
    height: usize = 0;
    blocked: []bool = [];
    costs: []f64 = [];

    /// Allocates a `width x height` grid with all cells unblocked and cost `1.0`.
    static fn init(alloc: std.mem.Allocator, width: usize, height: usize): GridMap ! PathError {
        if (width == 0 || height == 0) {
            return PathError.InvalidDimensions;
        }
        const total = std.math.mul(usize, width, height) catch return PathError.InvalidDimensions;

        const blocked = alloc.alloc(bool, total) catch return PathError.OutOfMemory;
        errdefer alloc.free(blocked);
        @memset(blocked, false);

        const costs = alloc.alloc(f64, total) catch return PathError.OutOfMemory;
        @memset(costs, 1.0);

        return GridMap {
            alloc: alloc,
            width: width,
            height: height,
            blocked: blocked,
            costs: costs,
        };
    }

    /// Frees the backing obstacle and terrain cost buffers.
    fn deinit(*_) {
        if (_.blocked.len > 0) {
            _.alloc.free(_.blocked);
        }
        if (_.costs.len > 0) {
            _.alloc.free(_.costs);
        }
        _.blocked = [];
        _.costs = [];
        _.width = 0;
        _.height = 0;
    }

    /// Returns true if `(x, y)` lies within `[0, width) x [0, height)`.
    fn inBounds(x: usize, y: usize): bool {
        return x < _.width && y < _.height;
    }

    /// Marks cell `(x, y)` as blocked (`true`) or passable (`false`).
    fn setBlocked(*_, x: usize, y: usize, is_blocked: bool): void ! PathError {
        if (!_.inBounds(x, y)) return PathError.OutOfBounds;
        _.blocked[y * _.width + x] = is_blocked;
    }

    /// Returns whether cell `(x, y)` is blocked.
    fn isBlocked(x: usize, y: usize): bool ! PathError {
        if (!_.inBounds(x, y)) return PathError.OutOfBounds;
        return _.blocked[y * _.width + x];
    }

    /// Sets the traversal cost multiplier (`weight >= 1.0`, finite) for cell `(x, y)`.
    fn setCost(*_, x: usize, y: usize, weight: f64): void ! PathError {
        if (!_.inBounds(x, y)) return PathError.OutOfBounds;
        if (!(weight >= 1.0) || std.math.isInf(weight)) {
            return PathError.InvalidCost;
        }
        _.costs[y * _.width + x] = weight;
    }

    /// Returns the traversal cost multiplier of cell `(x, y)`.
    fn getCost(x: usize, y: usize): f64 ! PathError {
        if (!_.inBounds(x, y)) return PathError.OutOfBounds;
        return _.costs[y * _.width + x];
    }

    /// Supercover Bresenham / exact integer DDA line-of-sight check between `a` and `b`.
    /// Prevents corner-cutting by checking both orthogonal neighbors whenever the ray
    /// passes through a grid corner.
    fn hasLineOfSight(a: GridCoord, b: GridCoord): bool {
        if (!_.inBounds(a.x, a.y) || !_.inBounds(b.x, b.y)) return false;
        if (_.blocked[a.y * _.width + a.x] || _.blocked[b.y * _.width + b.x]) return false;
        if (a.x == b.x && a.y == b.y) return true;

        const dx: usize = if (b.x > a.x) b.x - a.x else a.x - b.x;
        const dy: usize = if (b.y > a.y) b.y - a.y else a.y - b.y;
        const sx: isize = if (b.x > a.x) 1 else -1;
        const sy: isize = if (b.y > a.y) 1 else -1;

        let cx: isize = isize(a.x);
        let cy: isize = isize(a.y);
        const gx: isize = isize(b.x);
        const gy: isize = isize(b.y);

        if (dx == 0) {
            while (cy != gy) {
                cy += sy;
                if (_.blocked[usize(cy) * _.width + usize(cx)]) return false;
            }
            return true;
        }
        if (dy == 0) {
            while (cx != gx) {
                cx += sx;
                if (_.blocked[usize(cy) * _.width + usize(cx)]) return false;
            }
            return true;
        }

        let t_x: usize = dy;
        let t_y: usize = dx;
        const step_x: usize = dy * usize(2);
        const step_y: usize = dx * usize(2);

        while (cx != gx || cy != gy) {
            if (t_x < t_y) {
                cx += sx;
                t_x += step_x;
                if (_.blocked[usize(cy) * _.width + usize(cx)]) return false;
            } else if (t_y < t_x) {
                cy += sy;
                t_y += step_y;
                if (_.blocked[usize(cy) * _.width + usize(cx)]) return false;
            } else {
                // Ray passes directly through a grid vertex: both orthogonal cells must be clear.
                const nx = usize(cx + sx);
                const ny = usize(cy + sy);
                if (_.blocked[usize(cy) * _.width + nx]) return false;
                if (_.blocked[ny * _.width + usize(cx)]) return false;
                cx += sx;
                cy += sy;
                t_x += step_x;
                t_y += step_y;
                if (_.blocked[usize(cy) * _.width + usize(cx)]) return false;
            }
        }
        return true;
    }

    /// Finds the minimum-cost path from `start` to `goal` using A* with a binary min-heap.
    /// When `allow_diagonal` is true, diagonal moves are only permitted if both adjacent
    /// orthogonal cells are unblocked (strict corner-cutting prevention).
    fn findPathAStar(
        alloc: std.mem.Allocator,
        start: GridCoord,
        goal: GridCoord,
        allow_diagonal: bool,
        heuristic: HeuristicKind,
    ): PathResult ! PathError {
        if (!_.inBounds(start.x, start.y) || !_.inBounds(goal.x, goal.y)) {
            return PathError.OutOfBounds;
        }
        const start_idx = start.y * _.width + start.x;
        const goal_idx = goal.y * _.width + goal.x;
        if (_.blocked[start_idx] || _.blocked[goal_idx]) {
            return PathError.BlockedEndpoint;
        }

        if (start_idx == goal_idx) {
            let coords = alloc.alloc(GridCoord, 1) catch return PathError.OutOfMemory;
            coords[0] = start;
            return PathResult {
                alloc: alloc,
                coords: coords,
                total_cost: 0.0,
                nodes_expanded: 1,
            };
        }

        const total = _.width * _.height;
        let g_score = alloc.alloc(f64, total) catch return PathError.OutOfMemory;
        defer alloc.free(g_score);
        @memset(g_score, std.math.inf(f64));

        let came_from = alloc.alloc(usize, total) catch return PathError.OutOfMemory;
        defer alloc.free(came_from);
        @memset(came_from, INVALID_IDX);

        let closed = alloc.alloc(bool, total) catch return PathError.OutOfMemory;
        defer alloc.free(closed);
        @memset(closed, false);

        let open: std.ArrayList(HeapNode) = .empty;
        defer open.deinit(alloc);

        g_score[start_idx] = 0.0;
        const h0 = computeHeuristic(start, goal, heuristic);
        heapPush(alloc, *open, HeapNode { priority: h0, g: 0.0, idx: start_idx })?;

        const dirs_x: [8]isize = [1, -1, 0, 0, 1, 1, -1, -1];
        const dirs_y: [8]isize = [0, 0, 1, -1, 1, -1, 1, -1];
        const dir_count: usize = if (allow_diagonal) 8 else 4;

        let nodes_expanded: usize = 0;
        let found = false;

        while (open.items.len > 0) {
            const cur = heapPop(*open);
            if (closed[cur.idx]) continue;
            closed[cur.idx] = true;
            nodes_expanded += 1;

            if (cur.idx == goal_idx) {
                found = true;
                break;
            }

            const cx = cur.idx % _.width;
            const cy = cur.idx / _.width;

            let d: usize = 0;
            while (d < dir_count) : (d += 1) {
                const dx = dirs_x[d];
                const dy = dirs_y[d];
                const nx_s = isize(cx) + dx;
                const ny_s = isize(cy) + dy;
                if (nx_s < 0 || ny_s < 0) continue;
                const nx = usize(nx_s);
                const ny = usize(ny_s);
                if (!_.inBounds(nx, ny)) continue;

                const n_idx = ny * _.width + nx;
                if (closed[n_idx] || _.blocked[n_idx]) continue;

                const is_diag = dx != 0 && dy != 0;
                if (is_diag) {
                    // Strict corner-cutting prevention: both orthogonal neighbors must be passable.
                    const adj1_idx = cy * _.width + nx;
                    const adj2_idx = ny * _.width + cx;
                    if (_.blocked[adj1_idx] || _.blocked[adj2_idx]) continue;
                }

                const step_len: f64 = if (is_diag) SQRT2 else 1.0;
                const tentative_g = cur.g + step_len * _.costs[n_idx];

                if (tentative_g < g_score[n_idx]) {
                    g_score[n_idx] = tentative_g;
                    came_from[n_idx] = cur.idx;
                    const h = computeHeuristic(GridCoord.init(nx, ny), goal, heuristic);
                    heapPush(alloc, *open, HeapNode {
                        priority: tentative_g + h,
                        g: tentative_g,
                        idx: n_idx,
                    })?;
                }
            }
        }

        if (!found) {
            return PathError.NoPathFound;
        }

        let rev: std.ArrayList(GridCoord) = .empty;
        defer rev.deinit(alloc);

        let trace = goal_idx;
        while (trace != INVALID_IDX) {
            const tx = trace % _.width;
            const ty = trace / _.width;
            rev.append(alloc, GridCoord.init(tx, ty)) catch return PathError.OutOfMemory;
            if (trace == start_idx) break;
            trace = came_from[trace];
        }

        const path_len = rev.items.len;
        let coords = alloc.alloc(GridCoord, path_len) catch return PathError.OutOfMemory;
        let i: usize = 0;
        while (i < path_len) : (i += 1) {
            coords[i] = rev.items[path_len - 1 - i];
        }

        return PathResult {
            alloc: alloc,
            coords: coords,
            total_cost: g_score[goal_idx],
            nodes_expanded: nodes_expanded,
        };
    }

    /// Computes a full-grid Dijkstra distance field from `goal` to every cell.
    /// Unreachable or blocked cells have distance `+inf`.
    /// Each cell `c` stores the exact minimum cost to travel from `c` to `goal`.
    fn computeDistanceField(
        alloc: std.mem.Allocator,
        goal: GridCoord,
        allow_diagonal: bool,
    ): ([]f64) ! PathError {
        if (!_.inBounds(goal.x, goal.y)) {
            return PathError.OutOfBounds;
        }
        const goal_idx = goal.y * _.width + goal.x;
        if (_.blocked[goal_idx]) {
            return PathError.BlockedEndpoint;
        }

        const total = _.width * _.height;
        let dist = alloc.alloc(f64, total) catch return PathError.OutOfMemory;
        errdefer alloc.free(dist);
        @memset(dist, std.math.inf(f64));

        let closed = alloc.alloc(bool, total) catch return PathError.OutOfMemory;
        defer alloc.free(closed);
        @memset(closed, false);

        let open: std.ArrayList(HeapNode) = .empty;
        defer open.deinit(alloc);

        dist[goal_idx] = 0.0;
        heapPush(alloc, *open, HeapNode { priority: 0.0, g: 0.0, idx: goal_idx })?;

        const dirs_x: [8]isize = [1, -1, 0, 0, 1, 1, -1, -1];
        const dirs_y: [8]isize = [0, 0, 1, -1, 1, -1, 1, -1];
        const dir_count: usize = if (allow_diagonal) 8 else 4;

        while (open.items.len > 0) {
            const cur = heapPop(*open);
            if (closed[cur.idx]) continue;
            closed[cur.idx] = true;

            const cx = cur.idx % _.width;
            const cy = cur.idx / _.width;

            let d: usize = 0;
            while (d < dir_count) : (d += 1) {
                const dx = dirs_x[d];
                const dy = dirs_y[d];
                const nx_s = isize(cx) + dx;
                const ny_s = isize(cy) + dy;
                if (nx_s < 0 || ny_s < 0) continue;
                const nx = usize(nx_s);
                const ny = usize(ny_s);
                if (!_.inBounds(nx, ny)) continue;

                const n_idx = ny * _.width + nx;
                if (closed[n_idx] || _.blocked[n_idx]) continue;

                const is_diag = dx != 0 && dy != 0;
                if (is_diag) {
                    const adj1_idx = cy * _.width + nx;
                    const adj2_idx = ny * _.width + cx;
                    if (_.blocked[adj1_idx] || _.blocked[adj2_idx]) continue;
                }

                // Forward movement from neighbor `n_idx` into `cur.idx` pays `_.costs[cur.idx]`.
                const step_len: f64 = if (is_diag) SQRT2 else 1.0;
                const alt = cur.g + step_len * _.costs[cur.idx];
                if (alt < dist[n_idx]) {
                    dist[n_idx] = alt;
                    heapPush(alloc, *open, HeapNode { priority: alt, g: alt, idx: n_idx })?;
                }
            }
        }

        return dist;
    }

    /// Greedily removes collinear and redundant intermediate waypoints from `path`
    /// using `hasLineOfSight`.
    fn smoothPath(alloc: std.mem.Allocator, path: []const GridCoord): ([]GridCoord) ! PathError {
        for (pt of path) {
            if (!_.inBounds(pt.x, pt.y)) return PathError.OutOfBounds;
        }

        if (path.len == 0) {
            return alloc.alloc(GridCoord, 0) catch return PathError.OutOfMemory;
        }
        if (path.len <= 2) {
            let out = alloc.alloc(GridCoord, path.len) catch return PathError.OutOfMemory;
            @memcpy(out, path);
            return out;
        }

        let smoothed: std.ArrayList(GridCoord) = .empty;
        defer smoothed.deinit(alloc);

        smoothed.append(alloc, path[0]) catch return PathError.OutOfMemory;

        let anchor: usize = 0;
        while (anchor < path.len - 1) {
            let farthest = anchor + 1;
            let probe = path.len - 1;
            while (probe > anchor + 1) : (probe -= 1) {
                if (_.hasLineOfSight(path[anchor], path[probe])) {
                    farthest = probe;
                    break;
                }
            }
            smoothed.append(alloc, path[farthest]) catch return PathError.OutOfMemory;
            anchor = farthest;
        }

        return smoothed.toOwnedSlice(alloc) catch return PathError.OutOfMemory;
    }
}
