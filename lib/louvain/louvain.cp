// Origin: Pure-CP Stdlib (Zero-Alloc Louvain Modularity Community Detection & Label Propagation) | Created: 2026-11-22

fn isFiniteF64(v: f64): bool {
    return (v == v) && ((v - v) == 0.0);
}

/// Weighted undirected graph in Compressed Sparse Row (CSR) format.
/// For undirected graphs, each edge `{u, v}` with weight `w` appears in both `u`'s and `v`'s adjacency lists,
/// so `totalWeight()` equals `2m = sum_e w_e`.
pub struct WeightedGraph {
    num_nodes: usize,
    row_ptr: []const usize,
    col_idx: []const usize,
    weights: []const f64,

    /// Validates CSR structure, column bounds, and non-negative finite edge weights.
    fn validate(): bool {
        if (_.row_ptr.len != _.num_nodes + 1) {
            return false;
        }
        if (_.row_ptr[0] != 0) {
            return false;
        }
        let i: usize = 0;
        while (i < _.num_nodes) {
            if (_.row_ptr[i] > _.row_ptr[i + 1]) {
                return false;
            }
            i += 1;
        }
        const nnz = _.row_ptr[_.num_nodes];
        if (_.col_idx.len != nnz || _.weights.len != nnz) {
            return false;
        }
        let e: usize = 0;
        while (e < nnz) {
            if (_.col_idx[e] >= _.num_nodes || !isFiniteF64(_.weights[e]) || _.weights[e] < 0.0) {
                return false;
            }
            e += 1;
        }
        return true;
    }

    /// Computes total directed edge weight sum `2m = sum(weights)`.
    fn totalWeight(): f64 {
        let sum: f64 = 0.0;
        let e: usize = 0;
        while (e < _.weights.len) {
            sum += _.weights[e];
            e += 1;
        }
        return sum;
    }
}

/// Hyperparameters for `louvainDetect`.
pub struct LouvainConfig {
    resolution: f64,
    min_gain: f64,
    max_passes: usize,

    /// Standard Newman-Girvan Louvain defaults (`resolution = 1.0`, `min_gain = 1e-9`, `max_passes = 50`).
    fn default(): LouvainConfig {
        return LouvainConfig {
            resolution: 1.0,
            min_gain: 1.0e-9,
            max_passes: 50,
        };
    }
}

/// Summary returned by `louvainDetect`.
pub struct LouvainResult {
    num_communities: usize,
    modularity: f64,
    passes: usize,
    moves: usize,
}

/// Caller-owned zero-alloc workspace for `louvainDetect`.
/// All 5 slices must have length `>= g.num_nodes`.
pub struct LouvainWorkspace {
    node_deg: []f64,
    comm_tot: []f64,
    comm_weights: []f64,
    touched_comms: []usize,
    relabel_map: []i32,
}

/// Computes exact Newman-Girvan modularity with resolution parameter `gamma = resolution`:
/// `Q = sum_c [ in_c / (2m) - resolution * (tot_c / (2m))^2 ]`.
/// Returns `null` on invalid graph, invalid resolution, or insufficient buffers.
pub fn computeModularity(
    g: WeightedGraph,
    comm: []const usize,
    resolution: f64,
    comm_in_s: *[]f64,
    comm_tot_s: *[]f64,
): f64? {
    if (!g.validate() || !isFiniteF64(resolution) || resolution < 0.0) {
        return null;
    }
    const n = g.num_nodes;
    if (comm.len < n) {
        return null;
    }
    if (n == 0) {
        return 0.0;
    }

    // Determine maximum community index and verify scratch buffer lengths.
    let max_c: usize = 0;
    let i: usize = 0;
    while (i < n) {
        if (comm[i] > max_c) {
            max_c = comm[i];
        }
        i += 1;
    }
    const num_slots = max_c + 1;
    if (comm_in_s.*.len < num_slots || comm_tot_s.*.len < num_slots) {
        return null;
    }

    const two_m = g.totalWeight();
    if (!isFiniteF64(two_m) || two_m <= 0.0) {
        return 0.0;
    }

    let c: usize = 0;
    while (c < num_slots) {
        comm_in_s.*[c] = 0.0;
        comm_tot_s.*[c] = 0.0;
        c += 1;
    }

    i = 0;
    while (i < n) {
        const ci = comm[i];
        const start = g.row_ptr[i];
        const end = g.row_ptr[i + 1];
        let deg_i: f64 = 0.0;
        let e: usize = start;
        while (e < end) {
            const w = g.weights[e];
            deg_i += w;
            if (comm[g.col_idx[e]] == ci) {
                comm_in_s.*[ci] += w;
            }
            e += 1;
        }
        comm_tot_s.*[ci] += deg_i;
        i += 1;
    }

    let q: f64 = 0.0;
    const inv_2m = 1.0 / two_m;
    c = 0;
    while (c < num_slots) {
        if (comm_tot_s.*[c] > 0.0 || comm_in_s.*[c] > 0.0) {
            const frac_tot = comm_tot_s.*[c] * inv_2m;
            q += comm_in_s.*[c] * inv_2m - resolution * frac_tot * frac_tot;
        }
        c += 1;
    }
    return q;
}

/// Louvain local-moving community detection with canonical community relabeling (`0..num_communities-1`).
/// Returns `null` on invalid graph, invalid config, or insufficient buffers.
pub fn louvainDetect(
    g: WeightedGraph,
    cfg: LouvainConfig,
    comm_out: *[]usize,
    ws: *LouvainWorkspace,
): LouvainResult? {
    if (
        !g.validate() ||
        !isFiniteF64(cfg.resolution) ||
        cfg.resolution < 0.0 ||
        !isFiniteF64(cfg.min_gain) ||
        cfg.min_gain < 0.0
    ) {
        return null;
    }
    const n = g.num_nodes;
    if (
        comm_out.*.len < n ||
        ws.node_deg.len < n ||
        ws.comm_tot.len < n ||
        ws.comm_weights.len < n ||
        ws.touched_comms.len < n ||
        ws.relabel_map.len < n
    ) {
        return null;
    }

    if (n == 0) {
        return LouvainResult {
            num_communities: 0,
            modularity: 0.0,
            passes: 0,
            moves: 0,
        };
    }

    // Initialize each node in its own singleton community and precompute weighted degrees.
    let i: usize = 0;
    while (i < n) {
        comm_out.*[i] = i;
        let deg: f64 = 0.0;
        let e = g.row_ptr[i];
        const end = g.row_ptr[i + 1];
        while (e < end) {
            deg += g.weights[e];
            e += 1;
        }
        ws.node_deg[i] = deg;
        ws.comm_tot[i] = deg;
        ws.comm_weights[i] = 0.0;
        i += 1;
    }

    const two_m = g.totalWeight();
    if (!isFiniteF64(two_m) || two_m <= 0.0) {
        return LouvainResult {
            num_communities: n,
            modularity: 0.0,
            passes: 0,
            moves: 0,
        };
    }

    const inv_2m = 1.0 / two_m;
    const scale_dq = 2.0 * inv_2m;
    let total_moves: usize = 0;
    let passes: usize = 0;

    let pass: usize = 0;
    while (pass < cfg.max_passes) {
        let moved_in_pass: bool = false;
        passes = pass + 1;

        i = 0;
        while (i < n) {
            const ki = ws.node_deg[i];
            if (ki <= 0.0) {
                i += 1;
                continue;
            }
            const old_c = comm_out.*[i];

            // Accumulate edge weights from node i to neighboring communities (excluding self-loops).
            let touched_cnt: usize = 0;
            let e = g.row_ptr[i];
            const end = g.row_ptr[i + 1];
            while (e < end) {
                const nb = g.col_idx[e];
                if (nb != i) {
                    const nc = comm_out.*[nb];
                    if (ws.comm_weights[nc] == 0.0) {
                        ws.touched_comms[touched_cnt] = nc;
                        touched_cnt += 1;
                    }
                    ws.comm_weights[nc] += g.weights[e];
                }
                e += 1;
            }

            // Temporarily remove node i from old_c.
            ws.comm_tot[old_c] -= ki;
            if (ws.comm_tot[old_c] < 0.0) {
                ws.comm_tot[old_c] = 0.0;
            }

            const old_score = ws.comm_weights[old_c] - cfg.resolution * ki * ws.comm_tot[old_c] * inv_2m;
            let best_c: usize = old_c;
            let best_score: f64 = old_score;
            let best_is_empty: bool = false;

            let t: usize = 0;
            while (t < touched_cnt) {
                const cand_c = ws.touched_comms[t];
                const score = ws.comm_weights[cand_c] - cfg.resolution * ki * ws.comm_tot[cand_c] * inv_2m;
                if (score > best_score) {
                    best_score = score;
                    best_c = cand_c;
                    best_is_empty = false;
                }
                t += 1;
            }

            // Consider splitting into a fresh empty community (score = 0) when resolution is high.
            if (ws.comm_tot[old_c] > 0.0 && 0.0 > best_score) {
                best_score = 0.0;
                best_is_empty = true;
            }

            const delta_q = scale_dq * (best_score - old_score);
            if (delta_q > cfg.min_gain) {
                if (best_is_empty) {
                    let empty_c: usize = 0;
                    while (empty_c < n && ws.comm_tot[empty_c] > 0.0) {
                        empty_c += 1;
                    }
                    if (empty_c < n) {
                        best_c = empty_c;
                    } else {
                        best_c = old_c;
                    }
                }
                if (best_c != old_c) {
                    comm_out.*[i] = best_c;
                    ws.comm_tot[best_c] += ki;
                    moved_in_pass = true;
                    total_moves += 1;
                } else {
                    ws.comm_tot[old_c] += ki;
                }
            } else {
                ws.comm_tot[old_c] += ki;
            }

            // Sparse cleanup of comm_weights.
            t = 0;
            while (t < touched_cnt) {
                ws.comm_weights[ws.touched_comms[t]] = 0.0;
                t += 1;
            }

            i += 1;
        }

        if (!moved_in_pass) {
            break;
        }
        pass += 1;
    }

    // Canonical relabeling to 0..num_communities-1.
    i = 0;
    while (i < n) {
        ws.relabel_map[i] = -1;
        i += 1;
    }
    let next_id: i32 = 0;
    i = 0;
    while (i < n) {
        const old_c = comm_out.*[i];
        if (ws.relabel_map[old_c] < 0) {
            ws.relabel_map[old_c] = next_id;
            next_id += 1;
        }
        comm_out.*[i] = ws.relabel_map[old_c] as usize;
        i += 1;
    }
    const num_comms = next_id as usize;

    // Compute exact modularity using workspace scratch slices.
    let in_s: []f64 = ws.comm_weights[0..num_comms];
    let tot_s: []f64 = ws.comm_tot[0..num_comms];
    const mod_q = computeModularity(g, comm_out.*[0..n], cfg.resolution, &in_s, &tot_s) ?? 0.0;

    return LouvainResult {
        num_communities: num_comms,
        modularity: mod_q,
        passes: passes,
        moves: total_moves,
    };
}

/// Zero-alloc asynchronous Label Propagation Algorithm (LPA) for weighted graphs.
/// Writes community labels into `labels_out[0..g.num_nodes]` and returns the number of distinct labels.
pub fn labelPropagation(
    g: WeightedGraph,
    max_iter: usize,
    labels_out: *[]usize,
    ws_weights: *[]f64,
    ws_touched: *[]usize,
): usize? {
    if (!g.validate()) {
        return null;
    }
    const n = g.num_nodes;
    if (labels_out.*.len < n || ws_weights.*.len < n || ws_touched.*.len < n) {
        return null;
    }
    if (n == 0) {
        return 0;
    }

    let i: usize = 0;
    while (i < n) {
        labels_out.*[i] = i;
        ws_weights.*[i] = 0.0;
        i += 1;
    }

    let iter: usize = 0;
    while (iter < max_iter) {
        let changed: bool = false;
        i = 0;
        while (i < n) {
            const cur_label = labels_out.*[i];
            let touched_cnt: usize = 0;
            let e = g.row_ptr[i];
            const end = g.row_ptr[i + 1];
            while (e < end) {
                const nb = g.col_idx[e];
                const w = g.weights[e];
                if (nb != i && w > 0.0) {
                    const lbl = labels_out.*[nb];
                    if (ws_weights.*[lbl] == 0.0) {
                        ws_touched.*[touched_cnt] = lbl;
                        touched_cnt += 1;
                    }
                    ws_weights.*[lbl] += w;
                }
                e += 1;
            }

            if (touched_cnt > 0) {
                let best_lbl: usize = cur_label;
                let best_w: f64 = ws_weights.*[cur_label];
                let t: usize = 0;
                while (t < touched_cnt) {
                    const lbl = ws_touched.*[t];
                    const w = ws_weights.*[lbl];
                    if (w > best_w || (w == best_w && lbl != cur_label && best_lbl != cur_label && lbl < best_lbl)) {
                        best_w = w;
                        best_lbl = lbl;
                    }
                    t += 1;
                }
                if (best_lbl != cur_label) {
                    labels_out.*[i] = best_lbl;
                    changed = true;
                }

                t = 0;
                while (t < touched_cnt) {
                    ws_weights.*[ws_touched.*[t]] = 0.0;
                    t += 1;
                }
            }

            i += 1;
        }
        if (!changed) {
            break;
        }
        iter += 1;
    }

    // Count distinct labels using ws_weights as a zero-alloc marker array.
    let distinct: usize = 0;
    i = 0;
    while (i < n) {
        const lbl = labels_out.*[i];
        if (ws_weights.*[lbl] == 0.0) {
            ws_weights.*[lbl] = 1.0;
            distinct += 1;
        }
        i += 1;
    }
    return distinct;
}
