// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc Dinic Max-Flow / Min-Cut & Bipartite Matching Solver) | Created: 2026-11-22
// Pure CP zero-heap-allocation Dinic Maximum Flow / Minimum Cut solver with Current-Arc Optimization
// and unit-capacity Bipartite Maximum Matching solver for bandwidth allocation & Agent-Worker assignment:
//   - Inline forward-star adjacency pool (`FLOW_MAX_NODES = 64`, `FLOW_MAX_EDGES = 512`, zero dynamic heap)
//   - Paired forward/reverse residual edges with O(V^2 E) Dinic layered BFS + Current-Arc DFS augmentation
//   - Exact residual-graph source reachability bitmask (`source_cut_mask: u64`) exporting the Minimum s-t Cut
//   - Built-in deduplicated `solveBipartiteMatching` wrapper with O(1) 64-bit adjacency deduplication

// ============================================================================
// 1. Constants & Public Models
// ============================================================================

/// Maximum supported vertices in `DinicFlowNetwork` (zero dynamic heap, fits in a 64-bit cut mask).
pub const FLOW_MAX_NODES: usize = 64;

/// Maximum directed residual edges (forward + reverse combined = up to 256 user edges).
pub const FLOW_MAX_EDGES: usize = 512;

/// Sentinel edge index indicating end of adjacency list.
pub const FLOW_NO_EDGE: u16 = 0xFFFF;

const FLOW_INF: i64 = 0x7FFFFFFFFFFFFFFF;

/// Directed residual edge entry in the inline forward-star edge pool.
pub struct FlowEdge {
    to: u16 = 0;
    rev: u16 = FLOW_NO_EDGE;
    next: u16 = FLOW_NO_EDGE;
    cap: i64 = 0;
    flow: i64 = 0;
}

/// Solution returned by `DinicFlowNetwork.solve`, including exact Minimum s-t Cut bitmask.
pub struct MaxFlowResult {
    valid: bool = false;
    max_flow: i64 = 0;
    bfs_phases: u32 = 0;
    source_cut_mask: u64 = 0;

    /// Returns `true` iff `node` belongs to the source side `S` of the minimum s-t cut.
    fn isInSourceCut(node: usize): bool {
        if (node >= FLOW_MAX_NODES) return false;
        const shift: u6 = @intCast(node);
        return ((_.source_cut_mask >> shift) & 1) != 0;
    }
}

/// Matched `(left_idx, right_idx)` pair returned by `solveBipartiteMatching`.
pub struct BipartiteMatchPair {
    left_idx: u16 = 0;
    right_idx: u16 = 0;
}

// ============================================================================
// 2. Zero-Alloc Dinic Max-Flow / Min-Cut Engine
// ============================================================================

/// Inline zero-heap-allocation Dinic flow network with Current-Arc Optimization.
pub struct DinicFlowNetwork {
    node_count: usize = 0;
    edge_count: usize = 0;
    head: [FLOW_MAX_NODES]u16 = [FLOW_NO_EDGE; FLOW_MAX_NODES];
    edges: [FLOW_MAX_EDGES]FlowEdge = [FlowEdge{}; FLOW_MAX_EDGES];

    /// Initializes an empty flow network with `node_count` vertices (`1..=FLOW_MAX_NODES`).
    /// Returns `null` if `node_count == 0` or `node_count > FLOW_MAX_NODES`.
    static fn init(node_count: usize): DinicFlowNetwork? {
        if (node_count == 0 || node_count > FLOW_MAX_NODES) return null;
        return DinicFlowNetwork{
            node_count: node_count,
            edge_count: 0,
            head: [FLOW_NO_EDGE; FLOW_MAX_NODES],
            edges: [FlowEdge{}; FLOW_MAX_EDGES],
        };
    }

    /// Adds a directed edge `u -> v` with non-negative capacity `cap` alongside its paired
    /// zero-capacity reverse residual edge `v -> u`.
    /// Returns `false` on out-of-range vertices, negative capacity, or exhausted edge pool.
    fn add_edge(*_, u: usize, v: usize, cap: i64): bool {
        if (u >= _.node_count || v >= _.node_count) return false;
        if (cap < 0) return false;
        if (_.edge_count + 2 > FLOW_MAX_EDGES) return false;

        const fwd_usize: usize = _.edge_count;
        const rev_usize: usize = _.edge_count + 1;
        const fwd_idx: u16 = @intCast(fwd_usize);
        const rev_idx: u16 = @intCast(rev_usize);

        _.edges[fwd_usize] = FlowEdge{
            to: @intCast(v),
            rev: rev_idx,
            next: _.head[u],
            cap: cap,
            flow: 0,
        };
        _.head[u] = fwd_idx;

        _.edges[rev_usize] = FlowEdge{
            to: @intCast(u),
            rev: fwd_idx,
            next: _.head[v],
            cap: 0,
            flow: 0,
        };
        _.head[v] = rev_idx;

        _.edge_count += 2;
        return true;
    }

    /// Returns the committed flow on the `edge_pair_idx`-th user edge (0-based insertion order).
    fn edge_flow(edge_pair_idx: usize): i64 {
        const fwd_usize: usize = edge_pair_idx * 2;
        if (fwd_usize >= _.edge_count) return 0;
        return _.edges[fwd_usize].flow;
    }

    /// Internal bounded DFS with Current-Arc Optimization (max recursion depth <= FLOW_MAX_NODES = 64).
    fn dfsAugment(
        *_,
        u: usize,
        sink: usize,
        pushed: i64,
        level: []const i32,
        iter_head: *[]u16,
    ): i64 {
        if (pushed == 0) return 0;
        if (u == sink) return pushed;

        let e: u16 = iter_head[u];
        while (e != FLOW_NO_EDGE) {
            iter_head[u] = e;
            const e_usize: usize = e as usize;
            const to: usize = _.edges[e_usize].to as usize;
            const rem: i64 = _.edges[e_usize].cap - _.edges[e_usize].flow;
            if (rem > 0 && level[to] == level[u] + 1) {
                const next_push: i64 = if (pushed < rem) pushed else rem;
                const tr: i64 = _.dfsAugment(to, sink, next_push, level, iter_head);
                if (tr > 0) {
                    _.edges[e_usize].flow += tr;
                    const rev_usize: usize = _.edges[e_usize].rev as usize;
                    _.edges[rev_usize].flow -= tr;
                    return tr;
                }
            }
            e = _.edges[e_usize].next;
            iter_head[u] = e;
        }
        return 0;
    }

    /// Solves Maximum Flow from `source` to `sink` using Dinic's algorithm and computes
    /// the exact Minimum s-t Cut source reachability bitmask (`source_cut_mask`).
    /// Returns `valid = false` if `source` or `sink` is out of bounds or `source == sink`.
    fn solve(*_, source: usize, sink: usize): MaxFlowResult {
        if (source >= _.node_count || sink >= _.node_count || source == sink) {
            return MaxFlowResult{
                valid: false,
                max_flow: 0,
                bfs_phases: 0,
                source_cut_mask: 0,
            };
        }

        let level_buf: [FLOW_MAX_NODES]i32 = [-1; FLOW_MAX_NODES];
        let queue_buf: [FLOW_MAX_NODES]u16 = [0; FLOW_MAX_NODES];
        let iter_buf: [FLOW_MAX_NODES]u16 = [FLOW_NO_EDGE; FLOW_MAX_NODES];

        let total_flow: i64 = 0;
        let bfs_phases: u32 = 0;

        while (true) {
            // 1. BFS layered graph on positive residual capacity edges
            let i: usize = 0;
            while (i < _.node_count) : (i += 1) {
                level_buf[i] = -1;
            }
            level_buf[source] = 0;
            let q_head: usize = 0;
            let q_tail: usize = 0;
            queue_buf[q_tail] = @intCast(source);
            q_tail += 1;

            while (q_head < q_tail) {
                const u: usize = queue_buf[q_head] as usize;
                q_head += 1;
                let e: u16 = _.head[u];
                while (e != FLOW_NO_EDGE) {
                    const e_usize: usize = e as usize;
                    const v: usize = _.edges[e_usize].to as usize;
                    if (level_buf[v] < 0 && _.edges[e_usize].cap - _.edges[e_usize].flow > 0) {
                        level_buf[v] = level_buf[u] + 1;
                        queue_buf[q_tail] = @intCast(v);
                        q_tail += 1;
                    }
                    e = _.edges[e_usize].next;
                }
            }

            if (level_buf[sink] < 0) {
                // Sink unreachable: residual reachability in level_buf defines the exact Minimum s-t Cut
                break;
            }

            bfs_phases += 1;

            // 2. Blocking flow augmentation with Current-Arc Optimization
            let n: usize = 0;
            while (n < _.node_count) : (n += 1) {
                iter_buf[n] = _.head[n];
            }
            let iter_s: []u16 = iter_buf[0.._.node_count];
            while (true) {
                const pushed: i64 = _.dfsAugment(source, sink, FLOW_INF, level_buf[0.._.node_count], iter_s);
                if (pushed == 0) break;
                total_flow += pushed;
            }
        }

        // Export Minimum s-t Cut source-side bitmask from final residual BFS reachability
        let cut_mask: u64 = 0;
        let u: usize = 0;
        while (u < _.node_count) : (u += 1) {
            if (level_buf[u] >= 0) {
                const shift: u6 = @intCast(u);
                cut_mask = cut_mask | ((1 as u64) << shift);
            }
        }

        return MaxFlowResult{
            valid: true,
            max_flow: total_flow,
            bfs_phases: bfs_phases,
            source_cut_mask: cut_mask,
        };
    }
}

// ============================================================================
// 3. Zero-Alloc Bipartite Maximum Matching Solver (via Unit-Capacity Dinic)
// ============================================================================

/// Solves Maximum Cardinality Bipartite Matching between `left_count` left vertices
/// and `right_count` right vertices using unit-capacity `DinicFlowNetwork`.
/// - Requires `left_count + right_count + 2 <= FLOW_MAX_NODES` (`64`).
/// - Automatically ignores out-of-range vertex indices and deduplicates parallel edges.
/// - Writes matched `(left_idx, right_idx)` pairs into `out_pairs` and returns the count written.
pub fn solveBipartiteMatching(
    left_count: usize,
    right_count: usize,
    edges_left: []const u16,
    edges_right: []const u16,
    out_pairs: *[]BipartiteMatchPair,
): usize {
    if (left_count == 0 || right_count == 0) return 0;
    const total_nodes: usize = left_count + right_count + 2;
    if (total_nodes > FLOW_MAX_NODES) return 0;

    let net = DinicFlowNetwork.init(total_nodes) ?? return 0;
    const source: usize = 0;
    const right_offset: usize = left_count + 1;
    const sink: usize = right_offset + right_count;

    // Source -> Left (cap = 1)
    let l: usize = 0;
    while (l < left_count) : (l += 1) {
        if (!net.add_edge(source, l + 1, 1)) return 0;
    }

    // Right -> Sink (cap = 1)
    let r: usize = 0;
    while (r < right_count) : (r += 1) {
        if (!net.add_edge(right_offset + r, sink, 1)) return 0;
    }

    // Deduplicated Left -> Right edges (cap = 1)
    let seen_mask: [FLOW_MAX_NODES]u64 = [0; FLOW_MAX_NODES];
    const m: usize = if (edges_left.len < edges_right.len) edges_left.len else edges_right.len;
    let i: usize = 0;
    while (i < m) : (i += 1) {
        const lu: usize = edges_left[i] as usize;
        const rv: usize = edges_right[i] as usize;
        if (lu >= left_count || rv >= right_count) continue;
        const r_shift: u6 = @intCast(rv);
        if (((seen_mask[lu] >> r_shift) & 1) != 0) continue;
        seen_mask[lu] = seen_mask[lu] | ((1 as u64) << r_shift);
        if (!net.add_edge(lu + 1, right_offset + rv, 1)) break;
    }

    const res: MaxFlowResult = net.solve(source, sink);
    if (!res.valid || res.max_flow <= 0) return 0;

    // Extract matched (left_idx, right_idx) pairs in deterministic ascending left_idx order
    let written: usize = 0;
    let li: usize = 0;
    while (li < left_count) : (li += 1) {
        let e: u16 = net.head[li + 1];
        while (e != FLOW_NO_EDGE) {
            const e_usize: usize = e as usize;
            const to: usize = net.edges[e_usize].to as usize;
            if (to >= right_offset && to < sink && net.edges[e_usize].flow == 1) {
                if (written < out_pairs.len) {
                    out_pairs[written] = BipartiteMatchPair{
                        left_idx: @intCast(li),
                        right_idx: @intCast(to - right_offset),
                    };
                    written += 1;
                }
                break;
            }
            e = net.edges[e_usize].next;
        }
    }
    return written;
}
