// Origin: Pure-CP Stdlib (Zero-Alloc Bipartite Graph Algorithms: BFS 2-Coloring, Hopcroft-Karp O(E*sqrt(V)) Matching & Konig Min Vertex Cover) | Created: 2026-11-22

/// Sentinel indicating an unmatched vertex in `match_left` / `match_right`.
pub const UNMATCHED: i32 = -1;

const INF_DIST: u32 = 0xFFFFFFFF;

/// CSR adjacency representation of a bipartite graph from left vertices `0..num_left`
/// to right vertices `0..num_right`.
pub struct BipartiteGraph {
    num_left: usize,
    num_right: usize,
    row_ptr: []const usize,
    col_idx: []const usize,

    /// Validates CSR invariants: monotonic `row_ptr` of length `>= num_left + 1`,
    /// `row_ptr[0] == 0`, `row_ptr[num_left] <= col_idx.len`, and all right endpoints `< num_right`.
    fn validate(): bool {
        if (_.row_ptr.len < _.num_left + 1) {
            return false;
        }
        if (_.row_ptr[0] != 0) {
            return false;
        }
        let u: usize = 0;
        while (u < _.num_left) {
            if (_.row_ptr[u] > _.row_ptr[u + 1]) {
                return false;
            }
            u += 1;
        }
        const total_edges = _.row_ptr[_.num_left];
        if (total_edges > _.col_idx.len) {
            return false;
        }
        let e: usize = 0;
        while (e < total_edges) {
            if (_.col_idx[e] >= _.num_right) {
                return false;
            }
            e += 1;
        }
        return true;
    }

    /// Constructs a validated `BipartiteGraph`, returning `null` if CSR invariants fail.
    static fn init(
        num_left: usize,
        num_right: usize,
        row_ptr: []const usize,
        col_idx: []const usize,
    ): BipartiteGraph? {
        const g = BipartiteGraph {
            num_left: num_left,
            num_right: num_right,
            row_ptr: row_ptr,
            col_idx: col_idx,
        };
        if (!g.validate()) {
            return null;
        }
        return g;
    }
}

/// Tests whether an undirected CSR graph on `0..num_nodes` is bipartite via BFS 2-coloring.
/// On success, fills `color_out[0..num_nodes]` with `0` or `1` and returns `true`.
/// Returns `false` if an odd cycle / self-loop exists, CSR is invalid, or buffers are undersized.
pub fn isBipartite(
    num_nodes: usize,
    row_ptr: []const usize,
    col_idx: []const usize,
    color_out: *[]i8,
    queue_scratch: *[]usize,
): bool {
    if (row_ptr.len < num_nodes + 1 || color_out.*.len < num_nodes || queue_scratch.*.len < num_nodes) {
        return false;
    }
    if (row_ptr[0] != 0) {
        return false;
    }
    let u: usize = 0;
    while (u < num_nodes) {
        if (row_ptr[u] > row_ptr[u + 1]) {
            return false;
        }
        u += 1;
    }
    const total_edges = row_ptr[num_nodes];
    if (total_edges > col_idx.len) {
        return false;
    }
    let e: usize = 0;
    while (e < total_edges) {
        if (col_idx[e] >= num_nodes) {
            return false;
        }
        e += 1;
    }

    if (num_nodes == 0) {
        return true;
    }

    u = 0;
    while (u < num_nodes) {
        color_out.*[u] = -1;
        u += 1;
    }

    let start: usize = 0;
    while (start < num_nodes) {
        if (color_out.*[start] != -1) {
            start += 1;
            continue;
        }

        color_out.*[start] = 0;
        let q_head: usize = 0;
        let q_tail: usize = 0;
        queue_scratch.*[q_tail] = start;
        q_tail += 1;

        while (q_head < q_tail) {
            const cur = queue_scratch.*[q_head];
            q_head += 1;
            const cur_col = color_out.*[cur];
            const next_col: i8 = if (cur_col == 0) 1 else 0;

            let ei = row_ptr[cur];
            const end_e = row_ptr[cur + 1];
            while (ei < end_e) {
                const nb = col_idx[ei];
                if (nb == cur) {
                    return false;
                }
                if (color_out.*[nb] == -1) {
                    color_out.*[nb] = next_col;
                    queue_scratch.*[q_tail] = nb;
                    q_tail += 1;
                } else if (color_out.*[nb] == cur_col) {
                    return false;
                }
                ei += 1;
            }
        }

        start += 1;
    }

    return true;
}

fn hkBfs(
    g: BipartiteGraph,
    match_left: *[]i32,
    match_right: *[]i32,
    dist: *[]u32,
    queue: *[]usize,
): bool {
    let q_head: usize = 0;
    let q_tail: usize = 0;

    let u: usize = 0;
    while (u < g.num_left) {
        if (match_left.*[u] == UNMATCHED) {
            dist.*[u] = 0;
            queue.*[q_tail] = u;
            q_tail += 1;
        } else {
            dist.*[u] = INF_DIST;
        }
        u += 1;
    }

    let found_free = false;
    while (q_head < q_tail) {
        const cur = queue.*[q_head];
        q_head += 1;
        const cur_dist = dist.*[cur];

        let ei = g.row_ptr[cur];
        const end_e = g.row_ptr[cur + 1];
        while (ei < end_e) {
            const v = g.col_idx[ei];
            const paired_u = match_right.*[v];
            if (paired_u == UNMATCHED) {
                found_free = true;
            } else {
                const pu = paired_u as usize;
                if (dist.*[pu] == INF_DIST) {
                    dist.*[pu] = cur_dist + 1;
                    queue.*[q_tail] = pu;
                    q_tail += 1;
                }
            }
            ei += 1;
        }
    }

    return found_free;
}

fn hkDfs(
    g: BipartiteGraph,
    u: usize,
    match_left: *[]i32,
    match_right: *[]i32,
    dist: *[]u32,
): bool {
    let ei = g.row_ptr[u];
    const end_e = g.row_ptr[u + 1];
    while (ei < end_e) {
        const v = g.col_idx[ei];
        const paired_u = match_right.*[v];
        if (paired_u == UNMATCHED) {
            match_left.*[u] = v as i32;
            match_right.*[v] = u as i32;
            return true;
        }
        const pu = paired_u as usize;
        if (dist.*[pu] == dist.*[u] + 1) {
            if (hkDfs(g, pu, match_left, match_right, dist)) {
                match_left.*[u] = v as i32;
                match_right.*[v] = u as i32;
                return true;
            }
        }
        ei += 1;
    }

    dist.*[u] = INF_DIST;
    return false;
}

/// Computes a Maximum Cardinality Bipartite Matching in O(E * sqrt(V)) time using Hopcroft-Karp.
/// Fills `match_left_out[0..num_left]` and `match_right_out[0..num_right]` with partner indices (or `UNMATCHED`).
/// Returns the maximum matching size, or `null` on invalid graph or undersized scratch/output slices.
pub fn hopcroftKarp(
    g: BipartiteGraph,
    match_left_out: *[]i32,
    match_right_out: *[]i32,
    dist_scratch: *[]u32,
    queue_scratch: *[]usize,
): usize? {
    if (!g.validate()) {
        return null;
    }
    if (
        match_left_out.*.len < g.num_left ||
        match_right_out.*.len < g.num_right ||
        dist_scratch.*.len < g.num_left ||
        queue_scratch.*.len < g.num_left
    ) {
        return null;
    }

    let u: usize = 0;
    while (u < g.num_left) {
        match_left_out.*[u] = UNMATCHED;
        u += 1;
    }
    let v: usize = 0;
    while (v < g.num_right) {
        match_right_out.*[v] = UNMATCHED;
        v += 1;
    }

    if (g.num_left == 0 || g.num_right == 0) {
        return 0;
    }

    let matching: usize = 0;
    while (hkBfs(g, match_left_out, match_right_out, dist_scratch, queue_scratch)) {
        u = 0;
        while (u < g.num_left) {
            if (match_left_out.*[u] == UNMATCHED) {
                if (hkDfs(g, u, match_left_out, match_right_out, dist_scratch)) {
                    matching += 1;
                }
            }
            u += 1;
        }
    }

    return matching;
}

/// Extracts a Minimum Vertex Cover via Konig's theorem from a maximum matching (`match_left`, `match_right`).
/// Marks vertices in the minimum vertex cover as `1` in `cover_left_out` / `cover_right_out` (`0` = Maximum Independent Set).
/// Returns total cover size (`== maximum matching size`), or `null` on invalid inputs or undersized buffers.
pub fn konigMinVertexCover(
    g: BipartiteGraph,
    match_left: []const i32,
    match_right: []const i32,
    cover_left_out: *[]u8,
    cover_right_out: *[]u8,
    vis_left_scratch: *[]u8,
    vis_right_scratch: *[]u8,
    queue_scratch: *[]usize,
): usize? {
    if (!g.validate()) {
        return null;
    }
    if (
        match_left.len < g.num_left ||
        match_right.len < g.num_right ||
        cover_left_out.*.len < g.num_left ||
        cover_right_out.*.len < g.num_right ||
        vis_left_scratch.*.len < g.num_left ||
        vis_right_scratch.*.len < g.num_right ||
        queue_scratch.*.len < g.num_left
    ) {
        return null;
    }

    let u: usize = 0;
    while (u < g.num_left) {
        vis_left_scratch.*[u] = 0;
        u += 1;
    }
    let v: usize = 0;
    while (v < g.num_right) {
        vis_right_scratch.*[v] = 0;
        v += 1;
    }

    let q_head: usize = 0;
    let q_tail: usize = 0;

    // Seed alternating BFS from all unmatched left vertices
    let u_seed: usize = 0;
    while (u_seed < g.num_left) {
        if (match_left[u_seed] == UNMATCHED) {
            vis_left_scratch.*[u_seed] = 1;
            queue_scratch.*[q_tail] = u_seed;
            q_tail += 1;
        }
        u_seed += 1;
    }

    while (q_head < q_tail) {
        const cur = queue_scratch.*[q_head];
        q_head += 1;

        let ei = g.row_ptr[cur];
        const end_e = g.row_ptr[cur + 1];
        while (ei < end_e) {
            const nb_r = g.col_idx[ei];
            // Traverse non-matching edge left -> right
            if (match_left[cur] != (nb_r as i32) && vis_right_scratch.*[nb_r] == 0) {
                vis_right_scratch.*[nb_r] = 1;
                const paired_l = match_right[nb_r];
                if (paired_l != UNMATCHED) {
                    if (paired_l < 0 || (paired_l as usize) >= g.num_left) {
                        return null;
                    }
                    const pl = paired_l as usize;
                    if (vis_left_scratch.*[pl] == 0) {
                        vis_left_scratch.*[pl] = 1;
                        queue_scratch.*[q_tail] = pl;
                        q_tail += 1;
                    }
                }
            }
            ei += 1;
        }
    }

    // Konig's theorem: MVC = (L \ Visited_L) U (R ∩ Visited_R)
    let cover_size: usize = 0;
    u = 0;
    while (u < g.num_left) {
        if (vis_left_scratch.*[u] == 0) {
            cover_left_out.*[u] = 1;
            cover_size += 1;
        } else {
            cover_left_out.*[u] = 0;
        }
        u += 1;
    }

    v = 0;
    while (v < g.num_right) {
        if (vis_right_scratch.*[v] != 0) {
            cover_right_out.*[v] = 1;
            cover_size += 1;
        } else {
            cover_right_out.*[v] = 0;
        }
        v += 1;
    }

    return cover_size;
}
