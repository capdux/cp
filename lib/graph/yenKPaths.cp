// Origin: Pure-CP Stdlib (Zero-Alloc Yen's K-Shortest Loopless Paths Solver for Directed Weighted Graphs) | Created: 2026-11-22
// 100% pure CP zero-heap-allocation Yen's K-Shortest Simple Paths engine:
//   - Fixed-capacity directed non-negative weighted graph (up to 32 nodes, 256 edges, u64 weights).
//   - Built-in pure iterative masked Dijkstra solver (supports temporary node & edge bans with zero heap).
//   - Classic Yen's spur-node deviation algorithm with candidate pool deduplication and deterministic
//     `(total_cost ascending, lexicographical node sequence ascending)` ordering.
//   - Full defensive guards: `k == 0`, `src == dst`, unreachable targets, negative weight / self-loop /
//     out-of-bounds node rejection, fewer-than-k available paths, and full-graph capacity limits.

// ============================================================================
// Constants
// ============================================================================

pub const YEN_MAX_NODES: usize = 32;
pub const YEN_MAX_EDGES: usize = 256;
pub const YEN_MAX_CANDIDATES: usize = 128;
pub const YEN_NO_EDGE: u16 = 0xFFFF;
pub const YEN_INF_COST: u64 = 0xFFFFFFFFFFFFFFFF;

const YEN_NO_NODE: u8 = 0xFF;

// ============================================================================
// Internal Helpers
// ============================================================================

fn safeAddCost(a: u64, b: u64): u64? {
    if (a == YEN_INF_COST || b == YEN_INF_COST) {
        return null;
    }
    if (a > (YEN_INF_COST - 1) - b) {
        return null;
    }
    return a + b;
}

// ============================================================================
// 1. Data Models
// ============================================================================

/// Directed weighted edge stored in `YenGraph`'s intrusive adjacency list.
pub struct YenEdge {
    u: u8 = 0;
    to: u8 = 0;
    weight: u64 = 0;
    next_edge: u16 = YEN_NO_EDGE;
}

/// Simple (loopless) directed path represented as a fixed-capacity node sequence.
pub struct YenPath {
    nodes: [32]u8 = [0; 32];
    len: usize = 0;
    total_cost: u64 = 0;

    /// Returns the active node sequence slice `nodes[0..len]`.
    fn nodeSlice(): []const u8 {
        return _.nodes[0.._.len];
    }

    /// Returns `true` iff two paths have identical node sequences and lengths.
    fn equals(other: YenPath): bool {
        if (_.len != other.len) {
            return false;
        }
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (_.nodes[i] != other.nodes[i]) {
                return false;
            }
        }
        return true;
    }

    /// Deterministic comparator: strictly prefers lower `total_cost`, then lexicographically smaller
    /// node sequence (and shorter length on shared prefix).
    fn isBetterThan(other: YenPath): bool {
        if (_.total_cost != other.total_cost) {
            return _.total_cost < other.total_cost;
        }
        let min_len = _.len;
        if (other.len < min_len) {
            min_len = other.len;
        }
        let i: usize = 0;
        while (i < min_len) : (i += 1) {
            if (_.nodes[i] != other.nodes[i]) {
                return _.nodes[i] < other.nodes[i];
            }
        }
        return _.len < other.len;
    }

    /// Returns `true` iff this path has no repeated nodes.
    fn isLoopless(): bool {
        let seen: [32]bool = [false; 32];
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            const n = _.nodes[i] as usize;
            if (n >= YEN_MAX_NODES || seen[n]) {
                return false;
            }
            seen[n] = true;
        }
        return true;
    }
}

// ============================================================================
// 2. Fixed-Capacity Directed Weighted Graph & Yen's K-Shortest Paths Solver
// ============================================================================

/// Zero-heap-allocation directed weighted graph supporting Yen's K-shortest loopless paths.
pub struct YenGraph {
    node_count: usize = 0;
    edge_count: usize = 0;
    head: [32]u16 = [YEN_NO_EDGE; 32];
    edges: [256]YenEdge = [YenEdge{}; 256];

    /// Initializes a directed graph with `node_count` vertices (`1..=32`).
    /// Returns `null` if `node_count == 0` or `node_count > YEN_MAX_NODES`.
    static fn init(node_count: usize): YenGraph? {
        if (node_count == 0 || node_count > YEN_MAX_NODES) {
            return null;
        }
        return YenGraph{
            node_count: node_count,
        };
    }

    /// Adds a directed edge `u -> v` with non-negative `u64` weight.
    /// Rejects out-of-bounds nodes, self-loops (`u == v`), sentinel `YEN_INF_COST`, or full edge table.
    fn addEdge(*_, u: usize, v: usize, weight: u64): bool {
        if (u >= _.node_count || v >= _.node_count) {
            return false;
        }
        if (u == v || weight == YEN_INF_COST || _.edge_count >= YEN_MAX_EDGES) {
            return false;
        }
        const e_idx = _.edge_count;
        _.edges[e_idx] = YenEdge{
            u: u as u8,
            to: v as u8,
            weight: weight,
            next_edge: _.head[u],
        };
        _.head[u] = e_idx as u16;
        _.edge_count += 1;
        return true;
    }

    /// Signed-weight helper that explicitly rejects negative weights (`weight < 0`).
    fn addSignedEdge(*_, u: usize, v: usize, weight: i64): bool {
        if (weight < 0) {
            return false;
        }
        return _.addEdge(u, v, weight as u64);
    }

    fn nodeCount(): usize {
        return _.node_count;
    }

    fn edgeCount(): usize {
        return _.edge_count;
    }

    /// Computes a single shortest path from `src` to `dst` via iterative Dijkstra.
    fn shortestPath(src: usize, dst: usize): YenPath? {
        if (src >= _.node_count || dst >= _.node_count) {
            return null;
        }
        const no_nodes: [32]bool = [false; 32];
        const no_edges: [256]bool = [false; 256];
        return _.dijkstraMasked(src, dst, no_nodes[0..], no_edges[0..]);
    }

    /// Finds up to `k` shortest loopless paths from `src` to `dst` using Yen's algorithm,
    /// writing them into `out_paths[0..found]` in deterministic `(total_cost asc, lex asc)` order.
    ///
    /// - Returns `null` if `src`/`dst` is out of bounds or `out_paths.len < k`.
    /// - Returns `0` if `k == 0` or `dst` is unreachable from `src`.
    /// - If `src == dst` and `k >= 1`, writes the single trivial 0-cost path `[src]` and returns `1`.
    fn findKShortestPaths(
        src: usize,
        dst: usize,
        k: usize,
        out_paths: *[]YenPath,
    ): usize? {
        if (src >= _.node_count || dst >= _.node_count) {
            return null;
        }
        if (k == 0) {
            return 0;
        }
        if (out_paths.len < k) {
            return null;
        }

        if (src == dst) {
            let p = YenPath{ len: 1, total_cost: 0 };
            p.nodes[0] = src as u8;
            out_paths[0] = p;
            return 1;
        }

        let banned_nodes: [32]bool = [false; 32];
        let banned_edges: [256]bool = [false; 256];

        const first_opt = _.dijkstraMasked(src, dst, banned_nodes[0..], banned_edges[0..]);
        if (first_path := first_opt) {
            out_paths[0] = first_path;
        } else {
            return 0;
        }

        let found: usize = 1;
        let cand_buf: [128]YenPath = [YenPath{}; 128];
        let cand_count: usize = 0;

        while (found < k) {
            const prev = out_paths[found - 1];
            if (prev.len >= 2) {
                let spur_idx: usize = 0;
                while (spur_idx + 1 < prev.len) : (spur_idx += 1) {
                    const spur_node = prev.nodes[spur_idx] as usize;

                    // Compute exact cost of root prefix prev.nodes[0..=spur_idx]
                    let root_cost: u64 = 0;
                    let valid_root: bool = true;
                    let rj: usize = 0;
                    while (rj < spur_idx) : (rj += 1) {
                        const u = prev.nodes[rj] as usize;
                        const v = prev.nodes[rj + 1] as usize;
                        if (ew := _.minEdgeWeight(u, v)) {
                            if (next_c := safeAddCost(root_cost, ew)) {
                                root_cost = next_c;
                            } else {
                                valid_root = false;
                                break;
                            }
                        } else {
                            valid_root = false;
                            break;
                        }
                    }
                    if (!valid_root) {
                        continue;
                    }

                    // Clear temporary node & edge masks
                    let bi: usize = 0;
                    while (bi < _.node_count) : (bi += 1) {
                        banned_nodes[bi] = false;
                    }
                    bi = 0;
                    while (bi < _.edge_count) : (bi += 1) {
                        banned_edges[bi] = false;
                    }

                    // Ban outgoing edges from spur_node used by accepted paths sharing the same root prefix
                    let pi: usize = 0;
                    while (pi < found) : (pi += 1) {
                        if (sharesPrefix(out_paths[pi], prev, spur_idx + 1) && out_paths[pi].len > spur_idx + 1) {
                            const next_v = out_paths[pi].nodes[spur_idx + 1] as usize;
                            let ei = _.head[spur_node];
                            while (ei != YEN_NO_EDGE) {
                                const edge_u = ei as usize;
                                if ((_.edges[edge_u].to as usize) == next_v) {
                                    banned_edges[edge_u] = true;
                                }
                                ei = _.edges[edge_u].next_edge;
                            }
                        }
                    }

                    // Ban root-path nodes strictly before spur_node to guarantee looplessness
                    rj = 0;
                    while (rj < spur_idx) : (rj += 1) {
                        banned_nodes[prev.nodes[rj] as usize] = true;
                    }

                    if (spur_path := _.dijkstraMasked(spur_node, dst, banned_nodes[0..], banned_edges[0..])) {
                        if (combined_cost := safeAddCost(root_cost, spur_path.total_cost)) {
                            const total_len = spur_idx + spur_path.len;
                            if (total_len <= YEN_MAX_NODES) {
                                let cand = YenPath{
                                    len: total_len,
                                    total_cost: combined_cost,
                                };
                                rj = 0;
                                while (rj < spur_idx) : (rj += 1) {
                                    cand.nodes[rj] = prev.nodes[rj];
                                }
                                let sj: usize = 0;
                                while (sj < spur_path.len) : (sj += 1) {
                                    cand.nodes[spur_idx + sj] = spur_path.nodes[sj];
                                }

                                if (!pathInSlice(cand, out_paths[0..found]) && !pathInSlice(cand, cand_buf[0..cand_count])) {
                                    if (cand_count < YEN_MAX_CANDIDATES) {
                                        cand_buf[cand_count] = cand;
                                        cand_count += 1;
                                    } else {
                                        // Pool full: replace worst candidate iff `cand` is strictly better
                                        let worst_idx: usize = 0;
                                        let ci: usize = 1;
                                        while (ci < cand_count) : (ci += 1) {
                                            if (cand_buf[worst_idx].isBetterThan(cand_buf[ci])) {
                                                worst_idx = ci;
                                            }
                                        }
                                        if (cand.isBetterThan(cand_buf[worst_idx])) {
                                            cand_buf[worst_idx] = cand;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (cand_count == 0) {
                break;
            }

            // Pick best candidate from `cand_buf[0..cand_count]`
            let best_idx: usize = 0;
            let ci: usize = 1;
            while (ci < cand_count) : (ci += 1) {
                if (cand_buf[ci].isBetterThan(cand_buf[best_idx])) {
                    best_idx = ci;
                }
            }

            out_paths[found] = cand_buf[best_idx];
            found += 1;
            cand_count -= 1;
            if (best_idx < cand_count) {
                cand_buf[best_idx] = cand_buf[cand_count];
            }
        }

        return found;
    }

    // ========================================================================
    // Internal Pure Iterative Masked Dijkstra & Helpers
    // ========================================================================

    fn minEdgeWeight(u: usize, v: usize): u64? {
        let found_any: bool = false;
        let best_w: u64 = YEN_INF_COST;
        let ei = _.head[u];
        while (ei != YEN_NO_EDGE) {
            const e = _.edges[ei as usize];
            if ((e.to as usize) == v) {
                if (!found_any || e.weight < best_w) {
                    found_any = true;
                    best_w = e.weight;
                }
            }
            ei = e.next_edge;
        }
        if (!found_any) {
            return null;
        }
        return best_w;
    }

    fn dijkstraMasked(
        src: usize,
        dst: usize,
        banned_nodes: []const bool,
        banned_edges: []const bool,
    ): YenPath? {
        if (banned_nodes[src] || banned_nodes[dst]) {
            return null;
        }

        let dist: [32]u64 = [YEN_INF_COST; 32];
        let prev: [32]u8 = [YEN_NO_NODE; 32];
        let visited: [32]bool = [false; 32];

        dist[src] = 0;

        let step: usize = 0;
        while (step < _.node_count) : (step += 1) {
            // Select unvisited, non-banned vertex with minimum dist (tie-break by smaller node index)
            let u: usize = YEN_MAX_NODES;
            let best_d: u64 = YEN_INF_COST;
            let v: usize = 0;
            while (v < _.node_count) : (v += 1) {
                if (!visited[v] && !banned_nodes[v] && dist[v] < best_d) {
                    best_d = dist[v];
                    u = v;
                }
            }
            if (u >= _.node_count || best_d == YEN_INF_COST) {
                break;
            }
            visited[u] = true;
            if (u == dst) {
                break;
            }

            let ei = _.head[u];
            while (ei != YEN_NO_EDGE) {
                const edge_idx = ei as usize;
                const e = _.edges[edge_idx];
                ei = e.next_edge;
                if (banned_edges[edge_idx]) {
                    continue;
                }
                const nxt = e.to as usize;
                if (visited[nxt] || banned_nodes[nxt]) {
                    continue;
                }
                if (cand_d := safeAddCost(best_d, e.weight)) {
                    if (cand_d < dist[nxt] || (cand_d == dist[nxt] && (u as u8) < prev[nxt])) {
                        dist[nxt] = cand_d;
                        prev[nxt] = u as u8;
                    }
                }
            }
        }

        if (dist[dst] == YEN_INF_COST) {
            return null;
        }

        // Reconstruct path from dst back to src into reverse buffer
        let rev: [32]u8 = [0; 32];
        let rlen: usize = 0;
        let cur: usize = dst;
        while (true) {
            if (rlen >= YEN_MAX_NODES) {
                return null;
            }
            rev[rlen] = cur as u8;
            rlen += 1;
            if (cur == src) {
                break;
            }
            const p = prev[cur];
            if (p == YEN_NO_NODE) {
                return null;
            }
            cur = p as usize;
        }

        let path = YenPath{
            len: rlen,
            total_cost: dist[dst],
        };
        let i: usize = 0;
        while (i < rlen) : (i += 1) {
            path.nodes[i] = rev[rlen - 1 - i];
        }
        return path;
    }
}

fn sharesPrefix(a: YenPath, b: YenPath, prefix_len: usize): bool {
    if (a.len < prefix_len || b.len < prefix_len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix_len) : (i += 1) {
        if (a.nodes[i] != b.nodes[i]) {
            return false;
        }
    }
    return true;
}

fn pathInSlice(target: YenPath, slice: []const YenPath): bool {
    let i: usize = 0;
    while (i < slice.len) : (i += 1) {
        if (slice[i].equals(target)) {
            return true;
        }
    }
    return false;
}
