// @cap/astar — Zero-heap 2D grid A* heuristic pathfinding engine
// MIT License (clean-room CP implementation)

pub const MAX_W: usize = 32;
pub const MAX_H: usize = 32;
pub const MAX_CELLS: usize = 1024;
pub const COST_STRAIGHT: i64 = 10;
pub const COST_DIAGONAL: i64 = 14;

const INF_COST: i64 = 1000000000;
const NONE_IDX: u16 = 0xFFFF;

pub struct GridPos {
    pub x: u16,
    pub y: u16,
}

pub struct AStarResult {
    pub found: bool,
    pub cost: i64,
    pub path_len: usize,
}

fn absDiffUsize(a: usize, b: usize) -> i64 {
    if (a >= b) {
        return (a - b) as i64;
    }
    return (b - a) as i64;
}

fn heuristic(x1: usize, y1: usize, x2: usize, y2: usize, allow_diagonal: bool) -> i64 {
    const dx: i64 = absDiffUsize(x1, x2);
    const dy: i64 = absDiffUsize(y1, y2);
    if (!allow_diagonal) {
        return (dx + dy) * COST_STRAIGHT;
    }
    let mn: i64 = dx;
    let mx: i64 = dy;
    if (dy < dx) {
        mn = dy;
        mx = dx;
    }
    return mx * COST_STRAIGHT + mn * (COST_DIAGONAL - COST_STRAIGHT);
}

pub struct GridMap {
    pub width: usize,
    pub height: usize,
    pub blocked: [1024]bool,

    pub static fn init(width: usize, height: usize) -> GridMap {
        let w: usize = width;
        if (w < 1) {
            w = 1;
        }
        if (w > MAX_W) {
            w = MAX_W;
        }
        let h: usize = height;
        if (h < 1) {
            h = 1;
        }
        if (h > MAX_H) {
            h = MAX_H;
        }
        let res = GridMap {
            width: w,
            height: h,
            blocked: undefined,
        };
        let i: usize = 0;
        while (i < MAX_CELLS) {
            res.blocked[i] = false;
            i += 1;
        }
        return res;
    }

    pub fn setBlocked(*_, x: usize, y: usize, is_blocked: bool) {
        if (x >= _.width || y >= _.height) {
            return;
        }
        _.blocked[y * _.width + x] = is_blocked;
    }

    pub fn isBlocked(x: usize, y: usize) -> bool {
        if (x >= _.width || y >= _.height) {
            return true;
        }
        return _.blocked[y * _.width + x];
    }

    pub fn findPath(
        start: GridPos,
        goal: GridPos,
        allow_diagonal: bool,
        out_path: []GridPos,
    ) -> AStarResult {
        const fail = AStarResult { found: false, cost: 0, path_len: 0 };
        const sx: usize = start.x as usize;
        const sy: usize = start.y as usize;
        const gx: usize = goal.x as usize;
        const gy: usize = goal.y as usize;

        if (_.isBlocked(sx, sy) || _.isBlocked(gx, gy)) {
            return fail;
        }
        if (sx == gx && sy == gy) {
            if (out_path.len >= 1) {
                out_path[0] = start;
            }
            return AStarResult { found: true, cost: 0, path_len: 1 };
        }

        const total: usize = _.width * _.height;
        const start_idx: usize = sy * _.width + sx;
        const goal_idx: usize = gy * _.width + gx;

        let g_score: [1024]i64 = undefined;
        let came_from: [1024]u16 = undefined;
        let closed: [1024]bool = undefined;
        let heap_node: [1024]u16 = undefined;
        let heap_f: [1024]i64 = undefined;
        let heap_pos: [1024]u16 = undefined;

        let i: usize = 0;
        while (i < total) {
            g_score[i] = INF_COST;
            came_from[i] = NONE_IDX;
            closed[i] = false;
            heap_pos[i] = NONE_IDX;
            i += 1;
        }

        g_score[start_idx] = 0;
        heap_node[0] = start_idx as u16;
        heap_f[0] = heuristic(sx, sy, gx, gy, allow_diagonal);
        heap_pos[start_idx] = 0;
        let heap_len: usize = 1;

        const dxs: [8]i32 = [1, -1, 0, 0, 1, 1, -1, -1];
        const dys: [8]i32 = [0, 0, 1, -1, 1, -1, 1, -1];
        let dir_count: usize = 4;
        if (allow_diagonal) {
            dir_count = 8;
        }

        while (heap_len > 0) {
            // Pop minimum f-score node from binary min-heap
            const cur_idx: usize = heap_node[0] as usize;
            heap_pos[cur_idx] = NONE_IDX;
            heap_len -= 1;
            if (heap_len > 0) {
                const last_node: u16 = heap_node[heap_len];
                const last_f: i64 = heap_f[heap_len];
                heap_node[0] = last_node;
                heap_f[0] = last_f;
                heap_pos[last_node as usize] = 0;

                // Sift down
                let p: usize = 0;
                while (true) {
                    const left: usize = p * 2 + 1;
                    const right: usize = p * 2 + 2;
                    let smallest: usize = p;
                    if (left < heap_len && heap_f[left] < heap_f[smallest]) {
                        smallest = left;
                    }
                    if (right < heap_len && heap_f[right] < heap_f[smallest]) {
                        smallest = right;
                    }
                    if (smallest == p) {
                        break;
                    }
                    const tmp_n: u16 = heap_node[p];
                    const tmp_f: i64 = heap_f[p];
                    heap_node[p] = heap_node[smallest];
                    heap_f[p] = heap_f[smallest];
                    heap_node[smallest] = tmp_n;
                    heap_f[smallest] = tmp_f;
                    heap_pos[heap_node[p] as usize] = p as u16;
                    heap_pos[heap_node[smallest] as usize] = smallest as u16;
                    p = smallest;
                }
            }

            if (cur_idx == goal_idx) {
                // Reconstruct path in reverse, then write forward into out_path
                let rev: [1024]u16 = undefined;
                let rev_len: usize = 0;
                let trace: usize = goal_idx;
                while (true) {
                    rev[rev_len] = trace as u16;
                    rev_len += 1;
                    if (trace == start_idx) {
                        break;
                    }
                    trace = came_from[trace] as usize;
                }
                let out_i: usize = 0;
                while (out_i < rev_len && out_i < out_path.len) {
                    const cell: usize = rev[rev_len - 1 - out_i] as usize;
                    out_path[out_i] = GridPos {
                        x: (cell % _.width) as u16,
                        y: (cell / _.width) as u16,
                    };
                    out_i += 1;
                }
                return AStarResult {
                    found: true,
                    cost: g_score[goal_idx],
                    path_len: rev_len,
                };
            }

            closed[cur_idx] = true;
            const cx: i32 = (cur_idx % _.width) as i32;
            const cy: i32 = (cur_idx / _.width) as i32;

            let d: usize = 0;
            while (d < dir_count) {
                const nx_i: i32 = cx + dxs[d];
                const ny_i: i32 = cy + dys[d];
                if (nx_i < 0 || ny_i < 0) {
                    d += 1;
                    continue;
                }
                const nx: usize = nx_i as usize;
                const ny: usize = ny_i as usize;
                if (nx >= _.width || ny >= _.height) {
                    d += 1;
                    continue;
                }
                const n_idx: usize = ny * _.width + nx;
                if (_.blocked[n_idx] || closed[n_idx]) {
                    d += 1;
                    continue;
                }

                // Prevent diagonal corner cutting through blocked orthogonal neighbors
                let step_cost: i64 = COST_STRAIGHT;
                if (d >= 4) {
                    if (_.blocked[(cy as usize) * _.width + nx] || _.blocked[ny * _.width + (cx as usize)]) {
                        d += 1;
                        continue;
                    }
                    step_cost = COST_DIAGONAL;
                }

                const tentative_g: i64 = g_score[cur_idx] + step_cost;
                if (tentative_g < g_score[n_idx]) {
                    came_from[n_idx] = cur_idx as u16;
                    g_score[n_idx] = tentative_g;
                    const f_val: i64 = tentative_g + heuristic(nx, ny, gx, gy, allow_diagonal);

                    let pos: usize = 0;
                    if (heap_pos[n_idx] == NONE_IDX) {
                        pos = heap_len;
                        heap_node[pos] = n_idx as u16;
                        heap_f[pos] = f_val;
                        heap_pos[n_idx] = pos as u16;
                        heap_len += 1;
                    } else {
                        pos = heap_pos[n_idx] as usize;
                        heap_f[pos] = f_val;
                    }

                    // Sift up
                    while (pos > 0) {
                        const parent: usize = (pos - 1) / 2;
                        if (heap_f[pos] >= heap_f[parent]) {
                            break;
                        }
                        const tmp_n: u16 = heap_node[pos];
                        const tmp_f: i64 = heap_f[pos];
                        heap_node[pos] = heap_node[parent];
                        heap_f[pos] = heap_f[parent];
                        heap_node[parent] = tmp_n;
                        heap_f[parent] = tmp_f;
                        heap_pos[heap_node[pos] as usize] = pos as u16;
                        heap_pos[heap_node[parent] as usize] = parent as u16;
                        pos = parent;
                    }
                }
                d += 1;
            }
        }

        return fail;
    }
}
