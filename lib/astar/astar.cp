// Origin: Pure-CP Stdlib (Zero-Alloc 2D Grid A* Shortest Pathfinding, Binary Min-Heap & Bresenham Line-of-Sight) | Created: 2026-11-22

/// Orthogonal (4-way) step cost scaled by 10 for exact integer arithmetic.
pub const COST_CARDINAL: u32 = 10;

/// Diagonal (8-way) step cost scaled by 10 (`≈ 10 * sqrt(2)`).
pub const COST_DIAGONAL: u32 = 14;

const INF_COST: u32 = 0xFFFFFFFF;

/// 2D integer grid coordinate.
pub struct GridPos {
    x: i32,
    y: i32,
}

/// Summary returned by `astarFindPath`.
pub struct AstarResult {
    found: bool,
    path_len: usize,
    total_cost: u32,
    nodes_expanded: u32,
}

/// Caller-supplied zero-alloc working buffers for A* search.
/// Every slice must have length `>= width * height`.
pub struct AstarWorkspace {
    g_score: []u32,
    parent: []i32,
    closed: []u8,
    heap_nodes: []u32,
    heap_f: []u32,
}

fn absDiffI32(a: i32, b: i32): u32 {
    const d: i64 = (a as i64) - (b as i64);
    return (if (d < 0) -d else d) as u32;
}

fn inBounds(p: GridPos, width: usize, height: usize): bool {
    return p.x >= 0 && p.y >= 0 && (p.x as usize) < width && (p.y as usize) < height;
}

/// Admissible 4-neighbor Manhattan heuristic scaled by `COST_CARDINAL (10)`.
pub fn manhattanHeuristic(a: GridPos, b: GridPos): u32 {
    return COST_CARDINAL * (absDiffI32(a.x, b.x) + absDiffI32(a.y, b.y));
}

/// Admissible 8-neighbor Octile heuristic (`14 * min(dx, dy) + 10 * |dx - dy|`).
pub fn octileHeuristic(a: GridPos, b: GridPos): u32 {
    const dx = absDiffI32(a.x, b.x);
    const dy = absDiffI32(a.y, b.y);
    const mn = if (dx < dy) dx else dy;
    const mx = if (dx > dy) dx else dy;
    return COST_DIAGONAL * mn + COST_CARDINAL * (mx - mn);
}

fn heapSiftUp(ws: *AstarWorkspace, start_idx: usize) {
    let idx = start_idx;
    while (idx > 0) {
        const parent_idx = (idx - 1) >> 1;
        if (ws.heap_f[idx] < ws.heap_f[parent_idx]) {
            const tmp_n = ws.heap_nodes[idx];
            const tmp_f = ws.heap_f[idx];
            ws.heap_nodes[idx] = ws.heap_nodes[parent_idx];
            ws.heap_f[idx] = ws.heap_f[parent_idx];
            ws.heap_nodes[parent_idx] = tmp_n;
            ws.heap_f[parent_idx] = tmp_f;
            idx = parent_idx;
        } else {
            break;
        }
    }
}

fn heapSiftDown(ws: *AstarWorkspace, heap_size: usize) {
    let idx: usize = 0;
    while (true) {
        const left = (idx << 1) + 1;
        if (left >= heap_size) {
            break;
        }
        let smallest = left;
        const right = left + 1;
        if (right < heap_size && ws.heap_f[right] < ws.heap_f[left]) {
            smallest = right;
        }
        if (ws.heap_f[smallest] < ws.heap_f[idx]) {
            const tmp_n = ws.heap_nodes[idx];
            const tmp_f = ws.heap_f[idx];
            ws.heap_nodes[idx] = ws.heap_nodes[smallest];
            ws.heap_f[idx] = ws.heap_f[smallest];
            ws.heap_nodes[smallest] = tmp_n;
            ws.heap_f[smallest] = tmp_f;
            idx = smallest;
        } else {
            break;
        }
    }
}

fn heapPushOrDecrease(ws: *AstarWorkspace, heap_size: usize, node: u32, f_score: u32, already_open: bool): usize {
    if (already_open) {
        let i: usize = 0;
        while (i < heap_size) {
            if (ws.heap_nodes[i] == node) {
                if (f_score < ws.heap_f[i]) {
                    ws.heap_f[i] = f_score;
                    heapSiftUp(ws, i);
                }
                return heap_size;
            }
            i += 1;
        }
    }
    ws.heap_nodes[heap_size] = node;
    ws.heap_f[heap_size] = f_score;
    heapSiftUp(ws, heap_size);
    return heap_size + 1;
}

/// Finds a shortest path on a 2D occupancy grid (`0` = passable, non-zero = obstacle)
/// using zero-alloc A* search with a binary min-heap open set.
/// - Supports 4-neighbor (Manhattan) and 8-neighbor (Octile with strict no-corner-cutting).
/// - Returns `null` on invalid dimensions, out-of-bounds `start`/`goal`, undersized `ws`,
///   or when `out_path` cannot hold the discovered path.
/// - Returns `AstarResult { found: false, ... }` when `start`/`goal` is blocked or unreachable.
pub fn astarFindPath(
    grid: []const u8,
    width: usize,
    height: usize,
    start: GridPos,
    goal: GridPos,
    allow_diagonal: bool,
    ws: *AstarWorkspace,
    out_path: []GridPos,
): AstarResult? {
    if (width == 0 || height == 0) {
        return null;
    }
    const n_cells = width * height;
    if (
        grid.len < n_cells ||
        ws.g_score.len < n_cells ||
        ws.parent.len < n_cells ||
        ws.closed.len < n_cells ||
        ws.heap_nodes.len < n_cells ||
        ws.heap_f.len < n_cells
    ) {
        return null;
    }
    if (!inBounds(start, width, height) || !inBounds(goal, width, height)) {
        return null;
    }

    const start_idx: usize = (start.y as usize) * width + (start.x as usize);
    const goal_idx: usize = (goal.y as usize) * width + (goal.x as usize);

    // Blocked start or goal -> unreachable
    if (grid[start_idx] != 0 || grid[goal_idx] != 0) {
        return AstarResult { found: false, path_len: 0, total_cost: 0, nodes_expanded: 0 };
    }

    // Trivial start == goal
    if (start_idx == goal_idx) {
        if (out_path.len < 1) {
            return null;
        }
        out_path[0] = start;
        return AstarResult { found: true, path_len: 1, total_cost: 0, nodes_expanded: 1 };
    }

    // Initialize workspace
    let i: usize = 0;
    while (i < n_cells) {
        ws.g_score[i] = INF_COST;
        ws.parent[i] = -1;
        ws.closed[i] = 0;
        i += 1;
    }

    const h0 = if (allow_diagonal) octileHeuristic(start, goal) else manhattanHeuristic(start, goal);
    ws.g_score[start_idx] = 0;
    let heap_size: usize = 0;
    heap_size = heapPushOrDecrease(ws, heap_size, start_idx as u32, h0, false);

    // 4 cardinal directions followed by 4 diagonal directions
    const dir_x: [8]i32 = [1, -1, 0, 0, 1, 1, -1, -1];
    const dir_y: [8]i32 = [0, 0, 1, -1, 1, -1, 1, -1];
    const dir_count: usize = if (allow_diagonal) 8 else 4;

    let expanded: u32 = 0;
    let reached = false;

    while (heap_size > 0) {
        // Pop minimum-f node from binary min-heap
        const cur_idx: usize = ws.heap_nodes[0] as usize;
        heap_size -= 1;
        if (heap_size > 0) {
            ws.heap_nodes[0] = ws.heap_nodes[heap_size];
            ws.heap_f[0] = ws.heap_f[heap_size];
            heapSiftDown(ws, heap_size);
        }

        if (ws.closed[cur_idx] != 0) {
            continue;
        }
        ws.closed[cur_idx] = 1;
        expanded += 1;

        if (cur_idx == goal_idx) {
            reached = true;
            break;
        }

        const cx: i32 = (cur_idx % width) as i32;
        const cy: i32 = (cur_idx / width) as i32;
        const cur_g = ws.g_score[cur_idx];

        let d: usize = 0;
        while (d < dir_count) {
            const dx = dir_x[d];
            const dy = dir_y[d];
            const nx = cx + dx;
            const ny = cy + dy;
            d += 1;

            if (nx < 0 || ny < 0 || (nx as usize) >= width || (ny as usize) >= height) {
                continue;
            }
            const next_idx: usize = (ny as usize) * width + (nx as usize);
            if (grid[next_idx] != 0 || ws.closed[next_idx] != 0) {
                continue;
            }

            // Strict no-corner-cutting for diagonal steps: both adjacent orthogonal cells must be free
            const is_diag = dx != 0 && dy != 0;
            if (is_diag) {
                const ortho1: usize = (cy as usize) * width + (nx as usize);
                const ortho2: usize = (ny as usize) * width + (cx as usize);
                if (grid[ortho1] != 0 || grid[ortho2] != 0) {
                    continue;
                }
            }

            const step_cost = if (is_diag) COST_DIAGONAL else COST_CARDINAL;
            const cand_g = cur_g + step_cost;
            if (cand_g < ws.g_score[next_idx]) {
                const already_open = ws.g_score[next_idx] != INF_COST;
                ws.g_score[next_idx] = cand_g;
                ws.parent[next_idx] = cur_idx as i32;
                const npos = GridPos { x: nx, y: ny };
                const h = if (allow_diagonal) octileHeuristic(npos, goal) else manhattanHeuristic(npos, goal);
                heap_size = heapPushOrDecrease(ws, heap_size, next_idx as u32, cand_g + h, already_open);
            }
        }
    }

    if (!reached) {
        return AstarResult { found: false, path_len: 0, total_cost: 0, nodes_expanded: expanded };
    }

    // Count path length first
    let plen: usize = 0;
    let trace: i32 = goal_idx as i32;
    while (trace >= 0) {
        plen += 1;
        if ((trace as usize) == start_idx) {
            break;
        }
        trace = ws.parent[trace as usize];
    }

    if (out_path.len < plen) {
        return null;
    }

    // Write path in forward order (start -> goal) directly into out_path
    let write_pos: usize = plen;
    trace = goal_idx as i32;
    while (write_pos > 0 && trace >= 0) {
        write_pos -= 1;
        const u_trace = trace as usize;
        out_path[write_pos] = GridPos {
            x: (u_trace % width) as i32,
            y: (u_trace / width) as i32,
        };
        if (u_trace == start_idx) {
            break;
        }
        trace = ws.parent[u_trace];
    }

    return AstarResult {
        found: true,
        path_len: plen,
        total_cost: ws.g_score[goal_idx],
        nodes_expanded: expanded,
    };
}

/// Checks unobstructed integer Bresenham line-of-sight between `a` and `b` on `grid`.
/// Returns `false` if dimensions/coordinates are invalid or any traversed cell is an obstacle.
pub fn hasLineOfSight(grid: []const u8, width: usize, height: usize, a: GridPos, b: GridPos): bool {
    if (width == 0 || height == 0 || grid.len < width * height) {
        return false;
    }
    if (!inBounds(a, width, height) || !inBounds(b, width, height)) {
        return false;
    }

    let x0 = a.x;
    let y0 = a.y;
    const x1 = b.x;
    const y1 = b.y;

    const dx: i32 = if (x1 >= x0) x1 - x0 else x0 - x1;
    const dy: i32 = if (y1 >= y0) y1 - y0 else y0 - y1;
    const sx: i32 = if (x0 < x1) 1 else -1;
    const sy: i32 = if (y0 < y1) 1 else -1;
    let err: i32 = dx - dy;

    while (true) {
        const idx: usize = (y0 as usize) * width + (x0 as usize);
        if (grid[idx] != 0) {
            return false;
        }
        if (x0 == x1 && y0 == y1) {
            return true;
        }
        const e2 = err * 2;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}
