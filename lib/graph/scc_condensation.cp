// Origin: Pure-CP Stdlib (Zero-Alloc Iterative Tarjan SCC & Condensation DAG Solver) | Created: 2026-11-22
// Pure CP zero-heap-allocation, zero-recursion Iterative Tarjan Strongly Connected Components (SCC) & Condensation DAG:
//   - Fixed inline forward-star adjacency (`SCC_MAX_NODES = 64`, `SCC_MAX_EDGES = 512`), 0 dynamic heap allocation
//   - Explicit iterative DFS stack eliminates recursion stack-overflow risk on deep chains
//   - Produces Tarjan reverse-topological SCC IDs: every condensed DAG edge `from_scc -> to_scc` satisfies `from_scc > to_scc`
//   - Exact cycle detection (`cyclic_scc_count`, `has_any_cycle`, `isNodeInCycle`) including single-node self-loops
//   - O(1) 64-bit bitmask deduplication of inter-SCC condensed DAG edges

/// Maximum supported directed graph nodes (zero dynamic heap allocation).
pub const SCC_MAX_NODES: usize = 64;

/// Maximum supported directed edges (zero dynamic heap allocation).
pub const SCC_MAX_EDGES: usize = 512;

/// Sentinel edge index indicating end of adjacency list.
pub const SCC_NO_EDGE: u16 = 0xFFFF;

// ============================================================================
// 1. Public Condensation DAG Edge & Result Models
// ============================================================================

/// Deduplicated inter-SCC directed edge in the condensed acyclic quotient graph.
/// Guaranteed by Tarjan's algorithm to satisfy `from_scc > to_scc` (strict reverse topological order).
pub struct CondensedDagEdge {
    from_scc: u16 = 0;
    to_scc: u16 = 0;
}

/// Complete Tarjan SCC partition and deduplicated condensation DAG result.
pub struct SccCondensationResult {
    valid: bool = false;
    node_count: usize = 0;
    scc_count: usize = 0;
    node_to_scc: [SCC_MAX_NODES]u16 = [0; SCC_MAX_NODES];
    scc_sizes: [SCC_MAX_NODES]u16 = [0; SCC_MAX_NODES];
    cyclic_scc_count: usize = 0;
    has_any_cycle: bool = false;
    condensed_edges: [SCC_MAX_EDGES]CondensedDagEdge = [CondensedDagEdge{}; SCC_MAX_EDGES];
    condensed_edge_count: usize = 0;
    /// Bitmask where bit `scc_id` is 1 iff that SCC has size > 1 or contains a self-loop.
    scc_cyclic_mask: u64 = 0;

    /// Returns `true` iff both nodes are in bounds and belong to the same strongly connected component.
    fn sameScc(u: usize, v: usize): bool {
        if (!_.valid || u >= _.node_count || v >= _.node_count) return false;
        return _.node_to_scc[u] == _.node_to_scc[v];
    }

    /// Returns `true` iff `u` belongs to a cyclic SCC (size > 1 or a self-loop on `u`).
    fn isNodeInCycle(u: usize): bool {
        if (!_.valid || u >= _.node_count) return false;
        const scc_id: u16 = _.node_to_scc[u];
        const bit: u64 = (1 as u64) << (scc_id as u6);
        return (_.scc_cyclic_mask & bit) != 0;
    }
}

// ============================================================================
// 2. Zero-Alloc Iterative Tarjan Graph & Condensation Solver
// ============================================================================

/// Zero-heap-allocation, zero-recursion Tarjan SCC and condensation DAG solver.
pub struct IterativeTarjanGraph {
    node_count: usize = 0;
    edge_count: usize = 0;
    head: [SCC_MAX_NODES]u16 = [SCC_NO_EDGE; SCC_MAX_NODES];
    edge_from: [SCC_MAX_EDGES]u16 = [0; SCC_MAX_EDGES];
    edge_to: [SCC_MAX_EDGES]u16 = [0; SCC_MAX_EDGES];
    edge_next: [SCC_MAX_EDGES]u16 = [SCC_NO_EDGE; SCC_MAX_EDGES];
    has_self_loop: [SCC_MAX_NODES]bool = [false; SCC_MAX_NODES];

    /// Initializes a directed graph with `node_count` vertices (`1 <= node_count <= SCC_MAX_NODES`).
    /// Returns `null` if `node_count == 0` or `node_count > SCC_MAX_NODES`.
    static fn init(node_count: usize): IterativeTarjanGraph? {
        if (node_count == 0 || node_count > SCC_MAX_NODES) {
            return null;
        }
        return IterativeTarjanGraph{
            node_count: node_count,
        };
    }

    /// Adds a directed edge `u -> v`. Records self-loops (`u == v`) for exact single-node cycle detection.
    /// Returns `false` if `u` or `v` is out of bounds or `SCC_MAX_EDGES` capacity is reached.
    fn add_edge(*_, u: usize, v: usize): bool {
        if (u >= _.node_count || v >= _.node_count) return false;
        if (_.edge_count >= SCC_MAX_EDGES) return false;

        if (u == v) {
            _.has_self_loop[u] = true;
        }

        const e_idx: usize = _.edge_count;
        _.edge_from[e_idx] = u as u16;
        _.edge_to[e_idx] = v as u16;
        _.edge_next[e_idx] = _.head[u];
        _.head[u] = e_idx as u16;
        _.edge_count += 1;
        return true;
    }

    /// Computes all strongly connected components using purely iterative Tarjan's algorithm
    /// and constructs the deduplicated acyclic condensation DAG.
    fn compute_scc_and_condensation(): SccCondensationResult {
        let res = SccCondensationResult{
            valid: true,
            node_count: _.node_count,
        };

        let disc: [SCC_MAX_NODES]u16 = [0; SCC_MAX_NODES];
        let low: [SCC_MAX_NODES]u16 = [0; SCC_MAX_NODES];
        let on_stack: [SCC_MAX_NODES]bool = [false; SCC_MAX_NODES];
        let tarjan_stack: [SCC_MAX_NODES]usize = [0; SCC_MAX_NODES];
        let tarjan_top: usize = 0;

        let dfs_node: [SCC_MAX_NODES]usize = [0; SCC_MAX_NODES];
        let dfs_edge: [SCC_MAX_NODES]u16 = [SCC_NO_EDGE; SCC_MAX_NODES];
        let dfs_top: usize = 0;

        let timer: u16 = 0;

        let root: usize = 0;
        while (root < _.node_count) : (root += 1) {
            if (disc[root] != 0) {
                continue;
            }

            timer += 1;
            disc[root] = timer;
            low[root] = timer;
            tarjan_stack[tarjan_top] = root;
            tarjan_top += 1;
            on_stack[root] = true;

            dfs_node[0] = root;
            dfs_edge[0] = _.head[root];
            dfs_top = 1;

            while (dfs_top > 0) {
                const frame: usize = dfs_top - 1;
                const u: usize = dfs_node[frame];
                const e: u16 = dfs_edge[frame];

                if (e != SCC_NO_EDGE) {
                    dfs_edge[frame] = _.edge_next[e as usize];
                    const v: usize = _.edge_to[e as usize] as usize;

                    if (disc[v] == 0) {
                        timer += 1;
                        disc[v] = timer;
                        low[v] = timer;
                        tarjan_stack[tarjan_top] = v;
                        tarjan_top += 1;
                        on_stack[v] = true;

                        dfs_node[dfs_top] = v;
                        dfs_edge[dfs_top] = _.head[v];
                        dfs_top += 1;
                    } else if (on_stack[v]) {
                        if (disc[v] < low[u]) {
                            low[u] = disc[v];
                        }
                    }
                } else {
                    dfs_top -= 1;

                    if (low[u] == disc[u]) {
                        const scc_id: u16 = res.scc_count as u16;
                        let scc_self_loop: bool = false;
                        while (tarjan_top > 0) {
                            tarjan_top -= 1;
                            const w: usize = tarjan_stack[tarjan_top];
                            on_stack[w] = false;
                            res.node_to_scc[w] = scc_id;
                            res.scc_sizes[res.scc_count] += 1;
                            if (_.has_self_loop[w]) {
                                scc_self_loop = true;
                            }
                            if (w == u) break;
                        }
                        if (res.scc_sizes[res.scc_count] > 1 || scc_self_loop) {
                            res.cyclic_scc_count += 1;
                            res.has_any_cycle = true;
                            res.scc_cyclic_mask = res.scc_cyclic_mask | ((1 as u64) << (scc_id as u6));
                        }
                        res.scc_count += 1;
                    }

                    if (dfs_top > 0) {
                        const parent: usize = dfs_node[dfs_top - 1];
                        if (low[u] < low[parent]) {
                            low[parent] = low[u];
                        }
                    }
                }
            }
        }

        // Build deduplicated condensation DAG edges using O(1) 64-bit adjacency masks
        let adj_mask: [SCC_MAX_NODES]u64 = [0; SCC_MAX_NODES];
        let e_idx: usize = 0;
        while (e_idx < _.edge_count) : (e_idx += 1) {
            const su: u16 = res.node_to_scc[_.edge_from[e_idx] as usize];
            const sv: u16 = res.node_to_scc[_.edge_to[e_idx] as usize];
            if (su == sv) {
                continue;
            }
            const bit: u64 = (1 as u64) << (sv as u6);
            if ((adj_mask[su as usize] & bit) == 0) {
                adj_mask[su as usize] = adj_mask[su as usize] | bit;
                if (res.condensed_edge_count < SCC_MAX_EDGES) {
                    res.condensed_edges[res.condensed_edge_count] = CondensedDagEdge{
                        from_scc: su,
                        to_scc: sv,
                    };
                    res.condensed_edge_count += 1;
                }
            }
        }

        return res;
    }
}
