// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc CSR Graph, Iterative Tarjan SCC, Kahn Topological Sort, DAG Longest Path & 2-SAT Solver)

/// Zero-allocation Compressed Sparse Row (CSR) directed graph view.
/// For vertex `u in 0..num_nodes-1`, outgoing neighbors are `col_idx[row_ptr[u]..row_ptr[u + 1]]`.
pub struct CsrGraph {
    num_nodes: usize,
    row_ptr: []const usize,
    col_idx: []const usize,

    /// Validates CSR invariants: `row_ptr.len >= num_nodes + 1`, `row_ptr[0] == 0`,
    /// monotonic row offsets within `col_idx.len`, and all target vertices `< num_nodes`.
    fn validate(): bool {
        if (_.row_ptr.len < _.num_nodes + 1) {
            return false;
        }
        if (_.row_ptr[0] != 0) {
            return false;
        }
        let u: usize = 0;
        while (u < _.num_nodes) {
            const s: usize = _.row_ptr[u];
            const e: usize = _.row_ptr[u + 1];
            if (s > e || e > _.col_idx.len) {
                return false;
            }
            let ei: usize = s;
            while (ei < e) {
                if (_.col_idx[ei] >= _.num_nodes) {
                    return false;
                }
                ei += 1;
            }
            u += 1;
        }
        return true;
    }

    /// Returns the outgoing neighbor slice for vertex `u`, or an empty slice if out of bounds.
    fn neighbors(u: usize): []const usize {
        if (u >= _.num_nodes || (u + 1) >= _.row_ptr.len) {
            return _.col_idx[0..0];
        }
        const s: usize = _.row_ptr[u];
        const e: usize = _.row_ptr[u + 1];
        if (s > e || e > _.col_idx.len) {
            return _.col_idx[0..0];
        }
        return _.col_idx[s..e];
    }

    /// Returns total number of directed edges stored in the CSR graph.
    fn edgeCount(): usize {
        if (_.row_ptr.len <= _.num_nodes) {
            return 0;
        }
        return _.row_ptr[_.num_nodes];
    }
}

/// Caller-owned workspace for iterative Tarjan SCC (each slice must have length `>= num_nodes`).
pub struct TarjanWorkspace {
    disc: []i32,
    low: []i32,
    on_stack: []u8,
    tarjan_stack: []usize,
    call_node: []usize,
    call_edge: []usize,
}

/// 2-SAT disjunctive clause `(a OR b)`.
/// Positive literal `k in 1..num_vars` denotes `x_k`; negative `-k` denotes `NOT x_k`.
pub struct TwoSatClause {
    a: i32,
    b: i32,
}

fn checkTarjanWorkspace(ws: *TarjanWorkspace, n: usize): bool {
    return ws.*.disc.len >= n
        && ws.*.low.len >= n
        && ws.*.on_stack.len >= n
        && ws.*.tarjan_stack.len >= n
        && ws.*.call_node.len >= n
        && ws.*.call_edge.len >= n;
}

/// Iterative (non-recursive) Tarjan's Strongly Connected Components algorithm in `O(V + E)`.
/// Writes each vertex's SCC id in `0..num_sccs-1` into `comp_out[0..n]` in **reverse topological order**
/// (for every cross-SCC edge `u -> v`, `comp_out[u] > comp_out[v]`).
/// Returns `num_sccs`, or `null` if the CSR graph is invalid or buffers are insufficient.
pub fn tarjanScc(g: CsrGraph, comp_out: *[]usize, ws: *TarjanWorkspace): usize? {
    if (!g.validate()) {
        return null;
    }
    const n: usize = g.num_nodes;
    if (comp_out.*.len < n || !checkTarjanWorkspace(ws, n)) {
        return null;
    }
    if (n == 0) {
        return 0 as usize;
    }

    let init_i: usize = 0;
    while (init_i < n) {
        ws.*.disc[init_i] = -1;
        ws.*.low[init_i] = 0;
        ws.*.on_stack[init_i] = 0;
        init_i += 1;
    }

    let timer: i32 = 0;
    let t_top: usize = 0;
    let num_sccs: usize = 0;

    let start: usize = 0;
    while (start < n) {
        if (ws.*.disc[start] == -1) {
            ws.*.disc[start] = timer;
            ws.*.low[start] = timer;
            timer += 1;
            ws.*.tarjan_stack[t_top] = start;
            t_top += 1;
            ws.*.on_stack[start] = 1;

            ws.*.call_node[0] = start;
            ws.*.call_edge[0] = g.row_ptr[start];
            let c_top: usize = 1;

            while (c_top > 0) {
                const frame: usize = c_top - 1;
                const u: usize = ws.*.call_node[frame];
                const ei: usize = ws.*.call_edge[frame];
                const row_end: usize = g.row_ptr[u + 1];

                if (ei < row_end) {
                    ws.*.call_edge[frame] = ei + 1;
                    const v: usize = g.col_idx[ei];
                    if (ws.*.disc[v] == -1) {
                        ws.*.disc[v] = timer;
                        ws.*.low[v] = timer;
                        timer += 1;
                        ws.*.tarjan_stack[t_top] = v;
                        t_top += 1;
                        ws.*.on_stack[v] = 1;

                        ws.*.call_node[c_top] = v;
                        ws.*.call_edge[c_top] = g.row_ptr[v];
                        c_top += 1;
                    } else if (ws.*.on_stack[v] != 0) {
                        if (ws.*.disc[v] < ws.*.low[u]) {
                            ws.*.low[u] = ws.*.disc[v];
                        }
                    }
                } else {
                    if (ws.*.low[u] == ws.*.disc[u]) {
                        while (t_top > 0) {
                            t_top -= 1;
                            const w: usize = ws.*.tarjan_stack[t_top];
                            ws.*.on_stack[w] = 0;
                            comp_out.*[w] = num_sccs;
                            if (w == u) {
                                break;
                            }
                        }
                        num_sccs += 1;
                    }
                    c_top -= 1;
                    if (c_top > 0) {
                        const parent: usize = ws.*.call_node[c_top - 1];
                        if (ws.*.low[u] < ws.*.low[parent]) {
                            ws.*.low[parent] = ws.*.low[u];
                        }
                    }
                }
            }
        }
        start += 1;
    }

    return num_sccs;
}

/// Kahn's `O(V + E)` topological sort and cycle detection.
/// If `g` is a DAG, fills `order_out[0..n]` with a valid topological ordering and returns `true`.
/// Returns `false` if `g` contains a directed cycle, is invalid, or scratch buffers are too small.
pub fn topoSort(
    g: CsrGraph,
    order_out: *[]usize,
    indeg_scratch: *[]usize,
    queue_scratch: *[]usize,
): bool {
    if (!g.validate()) {
        return false;
    }
    const n: usize = g.num_nodes;
    if (order_out.*.len < n || indeg_scratch.*.len < n || queue_scratch.*.len < n) {
        return false;
    }
    if (n == 0) {
        return true;
    }

    let i: usize = 0;
    while (i < n) {
        indeg_scratch.*[i] = 0;
        i += 1;
    }

    let u: usize = 0;
    while (u < n) {
        const s: usize = g.row_ptr[u];
        const e: usize = g.row_ptr[u + 1];
        let ei: usize = s;
        while (ei < e) {
            const v: usize = g.col_idx[ei];
            indeg_scratch.*[v] += 1;
            ei += 1;
        }
        u += 1;
    }

    let q_head: usize = 0;
    let q_tail: usize = 0;
    let v0: usize = 0;
    while (v0 < n) {
        if (indeg_scratch.*[v0] == 0) {
            queue_scratch.*[q_tail] = v0;
            q_tail += 1;
        }
        v0 += 1;
    }

    let written: usize = 0;
    while (q_head < q_tail) {
        const cur: usize = queue_scratch.*[q_head];
        q_head += 1;
        order_out.*[written] = cur;
        written += 1;

        const s: usize = g.row_ptr[cur];
        const e: usize = g.row_ptr[cur + 1];
        let ei: usize = s;
        while (ei < e) {
            const nxt: usize = g.col_idx[ei];
            indeg_scratch.*[nxt] -= 1;
            if (indeg_scratch.*[nxt] == 0) {
                queue_scratch.*[q_tail] = nxt;
                q_tail += 1;
            }
            ei += 1;
        }
    }

    return written == n;
}

/// Returns `true` if `g` is a valid Directed Acyclic Graph (DAG), `false` otherwise.
pub fn isDag(
    g: CsrGraph,
    indeg_scratch: *[]usize,
    queue_scratch: *[]usize,
    order_scratch: *[]usize,
): bool {
    return topoSort(g, order_scratch, indeg_scratch, queue_scratch);
}

/// Computes longest path length (in edges) in a DAG via topological DP in `O(V + E)`.
/// Writes the longest path ending at each vertex `v` into `dist_out[v]`.
/// Returns the global maximum path length, or `null` if `g` has a cycle or buffers are insufficient.
pub fn dagLongestPath(
    g: CsrGraph,
    dist_out: *[]usize,
    order_scratch: *[]usize,
    indeg_scratch: *[]usize,
    queue_scratch: *[]usize,
): usize? {
    const n: usize = g.num_nodes;
    if (dist_out.*.len < n) {
        return null;
    }
    if (!topoSort(g, order_scratch, indeg_scratch, queue_scratch)) {
        return null;
    }
    if (n == 0) {
        return 0 as usize;
    }

    let i: usize = 0;
    while (i < n) {
        dist_out.*[i] = 0;
        i += 1;
    }

    let max_len: usize = 0;
    let idx: usize = 0;
    while (idx < n) {
        const u: usize = order_scratch.*[idx];
        const du: usize = dist_out.*[u];
        if (du > max_len) {
            max_len = du;
        }
        const cand: usize = du + 1;
        const s: usize = g.row_ptr[u];
        const e: usize = g.row_ptr[u + 1];
        let ei: usize = s;
        while (ei < e) {
            const v: usize = g.col_idx[ei];
            if (cand > dist_out.*[v]) {
                dist_out.*[v] = cand;
                if (cand > max_len) {
                    max_len = cand;
                }
            }
            ei += 1;
        }
        idx += 1;
    }

    return max_len;
}

fn literalToNode(lit: i32, num_vars: usize): usize? {
    if (lit == 0) {
        return null;
    }
    if (lit > 0) {
        const v: usize = (lit - 1) as usize;
        if (v >= num_vars) {
            return null;
        }
        return (2 as usize) * v;
    }
    // Negative literal: careful with i32 minimum overflow
    if (lit == -2147483648) {
        return null;
    }
    const pos: i32 = 0 - lit;
    const v: usize = (pos - 1) as usize;
    if (v >= num_vars) {
        return null;
    }
    return (2 as usize) * v + 1;
}

/// Zero-allocation 2-SAT solver backed by iterative Tarjan SCC.
/// - `assignment_out`: length `>= num_vars` (receives `0` or `1` per variable `x_1..x_n`)
/// - `row_ptr_s`: length `>= 2 * num_vars + 1`
/// - `col_idx_s`: length `>= 2 * clauses.len`
/// - `comp_s`: length `>= 2 * num_vars`
/// - `ws`: workspace with all 6 slices of length `>= 2 * num_vars`
/// Returns `true` if satisfiable (filling `assignment_out`), `false` if unsatisfiable or invalid.
pub fn solve2Sat(
    num_vars: usize,
    clauses: []const TwoSatClause,
    assignment_out: *[]u8,
    row_ptr_s: *[]usize,
    col_idx_s: *[]usize,
    comp_s: *[]usize,
    ws: *TarjanWorkspace,
): bool {
    if (num_vars == 0) {
        return clauses.len == 0;
    }
    const total_nodes: usize = (2 as usize) * num_vars;
    const total_edges: usize = (2 as usize) * clauses.len;

    if (assignment_out.*.len < num_vars) {
        return false;
    }
    if (row_ptr_s.*.len < total_nodes + 1 || col_idx_s.*.len < total_edges || comp_s.*.len < total_nodes) {
        return false;
    }
    if (!checkTarjanWorkspace(ws, total_nodes)) {
        return false;
    }

    let z: usize = 0;
    while (z <= total_nodes) {
        row_ptr_s.*[z] = 0;
        z += 1;
    }

    // Pass 1: validate literals and count out-degrees for (~a -> b) and (~b -> a)
    let ci: usize = 0;
    while (ci < clauses.len) {
        const cl: TwoSatClause = clauses[ci];
        const a_node: usize = literalToNode(cl.a, num_vars) ?? return false;
        const b_node: usize = literalToNode(cl.b, num_vars) ?? return false;
        const not_a: usize = a_node ^ (1 as usize);
        const not_b: usize = b_node ^ (1 as usize);
        row_ptr_s.*[not_a + 1] += 1;
        row_ptr_s.*[not_b + 1] += 1;
        ci += 1;
    }

    // Prefix sum to build CSR row_ptr; use comp_s as temporary write cursors
    let u: usize = 0;
    while (u < total_nodes) {
        row_ptr_s.*[u + 1] += row_ptr_s.*[u];
        comp_s.*[u] = row_ptr_s.*[u];
        u += 1;
    }

    // Pass 2: scatter implication edges into col_idx_s
    let ci2: usize = 0;
    while (ci2 < clauses.len) {
        const cl: TwoSatClause = clauses[ci2];
        const a_node: usize = literalToNode(cl.a, num_vars) ?? return false;
        const b_node: usize = literalToNode(cl.b, num_vars) ?? return false;
        const not_a: usize = a_node ^ (1 as usize);
        const not_b: usize = b_node ^ (1 as usize);

        const pos1: usize = comp_s.*[not_a];
        col_idx_s.*[pos1] = b_node;
        comp_s.*[not_a] = pos1 + 1;

        const pos2: usize = comp_s.*[not_b];
        col_idx_s.*[pos2] = a_node;
        comp_s.*[not_b] = pos2 + 1;
        ci2 += 1;
    }

    const g: CsrGraph = CsrGraph {
        num_nodes: total_nodes,
        row_ptr: row_ptr_s.*[0..(total_nodes + 1)],
        col_idx: col_idx_s.*[0..total_edges],
    };

    const scc_opt = tarjanScc(g, comp_s, ws);
    if (scc_opt == null) {
        return false;
    }

    let v: usize = 0;
    while (v < num_vars) {
        const pos_comp: usize = comp_s.*[(2 as usize) * v];
        const neg_comp: usize = comp_s.*[(2 as usize) * v + 1];
        if (pos_comp == neg_comp) {
            return false;
        }
        assignment_out.*[v] = if (pos_comp < neg_comp) (1 as u8) else (0 as u8);
        v += 1;
    }

    return true;
}
