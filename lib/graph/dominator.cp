// Origin: Pure-CP Stdlib (Zero-Alloc Iterative Cooper-Harvey-Kennedy Dominator Tree & Dominance Frontiers) | Created: 2026-11-23
// Pure CP zero-heap-allocation control-flow graph dominance & SSA frontier analysis engine:
//   - Fixed-capacity directed graph (up to 64 nodes / 256 directed edges) with automatic unreachable-node
//     isolation, self-loops, parallel edges, and irreducible multi-entry loops
//   - 100% iterative explicit-stack postorder traversal (zero recursion, zero stack-overflow risk)
//     paired with the Cooper-Harvey-Kennedy iterative fixed-point dominator intersection algorithm
//   - Computes immediate dominator (`idom`), dominator-tree depth (`domDepth`), dominance & strict dominance
//     queries (`dominates`, `strictlyDominates`), 64-bit bitmask Dominance Frontiers (`DF(n)` for SSA Phi
//     placement / workflow join analysis), and natural-loop back-edge detection (`isBackEdge`, `collectBackEdges`)

// ============================================================================
// 1. Constants & Data Models
// ============================================================================

/// Maximum number of vertices supported inline (matches 64-bit dominance-frontier bitmask).
pub const DOM_MAX_NODES: usize = 64;

/// Maximum number of directed edges supported inline (zero dynamic heap allocation).
pub const DOM_MAX_EDGES: usize = 256;

/// Sentinel indicating no immediate dominator (entry node or unreachable node).
pub const DOM_NO_NODE: u16 = 0xFFFF;

/// Sentinel indicating end of adjacency list chain.
pub const DOM_NO_EDGE: u16 = 0xFFFF;

/// Directed CFG edge descriptor `(u -> v)`.
pub struct DomEdge {
    u: u16 = 0,
    v: u16 = 0,
}

/// Complete Dominator Tree, Dominance Frontier & Back-Edge analysis result.
pub struct DominatorResult {
    valid: bool = false,
    node_count: usize = 0,
    entry: usize = 0,
    reachable_count: usize = 0,
    iterations: usize = 0,
    reachable_mask: [DOM_MAX_NODES]bool = [false; DOM_MAX_NODES],
    idom_arr: [DOM_MAX_NODES]u16 = [DOM_NO_NODE; DOM_MAX_NODES],
    dom_depth_arr: [DOM_MAX_NODES]u16 = [0; DOM_MAX_NODES],
    frontier_mask: [DOM_MAX_NODES]u64 = [0; DOM_MAX_NODES],
    adj_mask: [DOM_MAX_NODES]u64 = [0; DOM_MAX_NODES],

    /// Returns `true` if `node` is in bounds and reachable from `entry`.
    pub fn isReachable(node: usize): bool {
        if (!_.valid || node >= _.node_count) return false;
        return _.reachable_mask[node];
    }

    /// Returns the immediate dominator `idom(node)` for a reachable non-entry node,
    /// or `null` for the `entry` node, unreachable nodes, or out-of-bounds indices.
    pub fn idom(node: usize): usize? {
        if (!_.valid || node >= _.node_count) return null;
        if (!_.reachable_mask[node] || node == _.entry) return null;
        const d = _.idom_arr[node];
        if (d == DOM_NO_NODE) return null;
        return d as usize;
    }

    /// Returns the depth of `node` in the dominator tree (`entry` has depth 0),
    /// or `null` if unreachable or out of bounds.
    pub fn domDepth(node: usize): usize? {
        if (!_.valid || node >= _.node_count || !_.reachable_mask[node]) return null;
        return _.dom_depth_arr[node] as usize;
    }

    /// Returns `true` if `a` dominates `b` (both must be reachable from `entry`; reflexive: `a == b` is `true`).
    pub fn dominates(a: usize, b: usize): bool {
        if (!_.valid || a >= _.node_count || b >= _.node_count) return false;
        if (!_.reachable_mask[a] || !_.reachable_mask[b]) return false;
        if (a == b) return true;
        if (a == _.entry) return true;

        let cur: usize = b;
        let guard: usize = 0;
        while (cur != _.entry && guard < DOM_MAX_NODES) : (guard += 1) {
            const parent = _.idom_arr[cur];
            if (parent == DOM_NO_NODE) return false;
            const p = parent as usize;
            if (p == a) return true;
            if (p == cur) return false;
            cur = p;
        }
        return false;
    }

    /// Returns `true` if `a` strictly dominates `b` (`a != b && dominates(a, b)`).
    pub fn strictlyDominates(a: usize, b: usize): bool {
        if (a == b) return false;
        return _.dominates(a, b);
    }

    /// Returns `true` if directed edge `(u -> v)` exists in the CFG, both endpoints are reachable,
    /// and `v` dominates `u` (natural loop back-edge, including self-loops `u -> u`).
    pub fn isBackEdge(u: usize, v: usize): bool {
        if (!_.valid || u >= _.node_count || v >= _.node_count) return false;
        if (!_.reachable_mask[u] || !_.reachable_mask[v]) return false;
        const bit: u64 = (1 as u64) << (v as u6);
        if ((_.adj_mask[u] & bit) == 0) return false;
        return _.dominates(v, u);
    }

    /// Returns `true` if `target` belongs to the Dominance Frontier `DF(node)`.
    pub fn inFrontier(node: usize, target: usize): bool {
        if (!_.valid || node >= _.node_count || target >= _.node_count) return false;
        if (!_.reachable_mask[node] || !_.reachable_mask[target]) return false;
        const bit: u64 = (1 as u64) << (target as u6);
        return (_.frontier_mask[node] & bit) != 0;
    }

    /// Returns the raw 64-bit Dominance Frontier bitmask for `node` (0 if unreachable or out of bounds).
    pub fn frontierMask(node: usize): u64 {
        if (!_.valid || node >= _.node_count || !_.reachable_mask[node]) return 0;
        return _.frontier_mask[node];
    }

    /// Collects all immediate children of `node` in the dominator tree in ascending order.
    /// Returns the number of child indices written (truncation-safe).
    pub fn collectDomChildren(node: usize, out_children: *[]usize): usize {
        if (!_.valid || node >= _.node_count || !_.reachable_mask[node]) return 0;
        let written: usize = 0;
        let i: usize = 0;
        while (i < _.node_count && written < out_children.len) : (i += 1) {
            if (i == _.entry || !_.reachable_mask[i]) continue;
            if (_.idom_arr[i] as usize == node) {
                out_children[written] = i;
                written += 1;
            }
        }
        return written;
    }

    /// Collects all nodes in the Dominance Frontier `DF(node)` in ascending order.
    /// Returns the number of indices written (truncation-safe).
    pub fn collectFrontier(node: usize, out_nodes: *[]usize): usize {
        if (!_.valid || node >= _.node_count || !_.reachable_mask[node]) return 0;
        const mask = _.frontier_mask[node];
        if (mask == 0) return 0;
        let written: usize = 0;
        let i: usize = 0;
        while (i < _.node_count && written < out_nodes.len) : (i += 1) {
            const bit: u64 = (1 as u64) << (i as u6);
            if ((mask & bit) != 0) {
                out_nodes[written] = i;
                written += 1;
            }
        }
        return written;
    }

    /// Collects all unique natural-loop back-edges `(u -> v)` in lexicographical order.
    /// Returns the number of back-edges written (truncation-safe).
    pub fn collectBackEdges(out_edges: *[]DomEdge): usize {
        if (!_.valid) return 0;
        let written: usize = 0;
        let u: usize = 0;
        while (u < _.node_count && written < out_edges.len) : (u += 1) {
            if (!_.reachable_mask[u]) continue;
            const row = _.adj_mask[u];
            if (row == 0) continue;
            let v: usize = 0;
            while (v < _.node_count && written < out_edges.len) : (v += 1) {
                const bit: u64 = (1 as u64) << (v as u6);
                if ((row & bit) != 0 && _.dominates(v, u)) {
                    out_edges[written] = DomEdge{
                        u: u as u16,
                        v: v as u16,
                    };
                    written += 1;
                }
            }
        }
        return written;
    }
}

// ============================================================================
// 2. Cooper-Harvey-Kennedy Intersect Helper
// ============================================================================

/// Walks two dominator-tree fingers upward by postorder number until they meet at their LCA.
fn chkIntersect(
    b1: usize,
    b2: usize,
    idom_arr: *const [DOM_MAX_NODES]u16,
    post_num: *const [DOM_MAX_NODES]u16,
): usize {
    let f1: usize = b1;
    let f2: usize = b2;
    while (f1 != f2) {
        while (post_num[f1] < post_num[f2]) {
            f1 = idom_arr[f1] as usize;
        }
        while (post_num[f2] < post_num[f1]) {
            f2 = idom_arr[f2] as usize;
        }
    }
    return f1;
}

// ============================================================================
// 3. Fixed-Capacity Directed CFG & Iterative Dominator Engine
// ============================================================================

/// Zero-heap-allocation directed control-flow graph for Cooper-Harvey-Kennedy dominator analysis.
pub struct DominatorGraph {
    node_count: usize = 0,
    edge_count: usize = 0,
    head: [DOM_MAX_NODES]u16 = [DOM_NO_EDGE; DOM_MAX_NODES],
    edge_to: [DOM_MAX_EDGES]u16 = [0; DOM_MAX_EDGES],
    edge_next: [DOM_MAX_EDGES]u16 = [DOM_NO_EDGE; DOM_MAX_EDGES],
    pred_head: [DOM_MAX_NODES]u16 = [DOM_NO_EDGE; DOM_MAX_NODES],
    pred_from: [DOM_MAX_EDGES]u16 = [0; DOM_MAX_EDGES],
    pred_next: [DOM_MAX_EDGES]u16 = [DOM_NO_EDGE; DOM_MAX_EDGES],
    adj_mask: [DOM_MAX_NODES]u64 = [0; DOM_MAX_NODES],

    /// Initializes a directed graph with `node_count` vertices (`1..=DOM_MAX_NODES`).
    /// Returns `null` if `node_count == 0` or `node_count > DOM_MAX_NODES`.
    pub fn init(node_count: usize): DominatorGraph? {
        if (node_count == 0 || node_count > DOM_MAX_NODES) return null;
        return DominatorGraph{
            node_count: node_count,
        };
    }

    /// Adds a directed edge `(u -> v)`. Returns the 0-based edge index, or `null` if out of bounds or full.
    pub fn addEdge(*_, u: usize, v: usize): usize? {
        if (u >= _.node_count || v >= _.node_count) return null;
        if (_.edge_count >= DOM_MAX_EDGES) return null;

        const idx = _.edge_count;
        _.edge_count += 1;

        // Forward adjacency list
        _.edge_to[idx] = v as u16;
        _.edge_next[idx] = _.head[u];
        _.head[u] = idx as u16;

        // Reverse predecessor list
        _.pred_from[idx] = u as u16;
        _.pred_next[idx] = _.pred_head[v];
        _.pred_head[v] = idx as u16;

        // Adjacency bitmask for O(1) edge existence & back-edge verification
        _.adj_mask[u] |= (1 as u64) << (v as u6);

        return idx;
    }

    /// Runs 100% iterative explicit-stack postorder DFS + Cooper-Harvey-Kennedy fixed-point dominator
    /// intersection + Dominance Frontier computation rooted at `entry`.
    /// If `entry >= node_count`, returns a `DominatorResult` with `valid == false`.
    pub fn compute(entry: usize): DominatorResult {
        let res = DominatorResult{
            valid: false,
            node_count: _.node_count,
            entry: entry,
            adj_mask: _.adj_mask,
        };
        if (entry >= _.node_count) return res;

        // 1. Iterative explicit-stack postorder traversal (zero recursion)
        let stack_node: [DOM_MAX_NODES]u16 = [0; DOM_MAX_NODES];
        let stack_edge: [DOM_MAX_NODES]u16 = [DOM_NO_EDGE; DOM_MAX_NODES];
        let postorder: [DOM_MAX_NODES]u16 = [0; DOM_MAX_NODES];
        let post_num: [DOM_MAX_NODES]u16 = [0; DOM_MAX_NODES];
        let post_len: usize = 0;

        res.reachable_mask[entry] = true;
        stack_node[0] = entry as u16;
        stack_edge[0] = _.head[entry];
        let sp: usize = 1;

        while (sp > 0) {
            const top = sp - 1;
            const e = stack_edge[top];
            if (e != DOM_NO_EDGE) {
                stack_edge[top] = _.edge_next[e as usize];
                const next_v = _.edge_to[e as usize] as usize;
                if (!res.reachable_mask[next_v]) {
                    res.reachable_mask[next_v] = true;
                    stack_node[sp] = next_v as u16;
                    stack_edge[sp] = _.head[next_v];
                    sp += 1;
                }
            } else {
                const u = stack_node[top] as usize;
                sp -= 1;
                postorder[post_len] = u as u16;
                post_num[u] = post_len as u16;
                post_len += 1;
            }
        }

        res.valid = true;
        res.reachable_count = post_len;

        // 2. Cooper-Harvey-Kennedy iterative fixed-point immediate dominator calculation
        res.idom_arr[entry] = entry as u16;

        let changed: bool = true;
        let iters: usize = 0;
        while (changed) {
            changed = false;
            iters += 1;

            // Iterate reachable nodes in Reverse Postorder (skipping `entry` at postorder[post_len - 1])
            if (post_len > 1) {
                let rpo_idx: usize = post_len - 1;
                while (rpo_idx > 0) {
                    rpo_idx -= 1;
                    const b: usize = postorder[rpo_idx] as usize;
                    let new_idom: u16 = DOM_NO_NODE;

                    let pe: u16 = _.pred_head[b];
                    while (pe != DOM_NO_EDGE) {
                        const p: usize = _.pred_from[pe as usize] as usize;
                        if (res.reachable_mask[p] && res.idom_arr[p] != DOM_NO_NODE) {
                            if (new_idom == DOM_NO_NODE) {
                                new_idom = p as u16;
                            } else {
                                new_idom = chkIntersect(
                                    p,
                                    new_idom as usize,
                                    &res.idom_arr,
                                    &post_num,
                                ) as u16;
                            }
                        }
                        pe = _.pred_next[pe as usize];
                    }

                    if (new_idom != DOM_NO_NODE && res.idom_arr[b] != new_idom) {
                        res.idom_arr[b] = new_idom;
                        changed = true;
                    }
                }
            }
        }
        res.iterations = iters;

        // 3. Single-pass RPO dominator-tree depth calculation (entry has depth 0)
        res.dom_depth_arr[entry] = 0;
        if (post_len > 1) {
            let rpo_idx: usize = post_len - 1;
            while (rpo_idx > 0) {
                rpo_idx -= 1;
                const b: usize = postorder[rpo_idx] as usize;
                const parent = res.idom_arr[b] as usize;
                res.dom_depth_arr[b] = res.dom_depth_arr[parent] + 1;
            }
        }

        // 4. Dominance Frontiers DF(n) (Cooper-Harvey-Kennedy Section 5)
        // Entry implicitly has 1 virtual incoming edge, so a back-edge into entry makes it a join point.
        let b: usize = 0;
        while (b < _.node_count) : (b += 1) {
            if (!res.reachable_mask[b]) continue;

            let pred_count: usize = if (b == entry) 1 else 0;
            let pe_cnt: u16 = _.pred_head[b];
            while (pe_cnt != DOM_NO_EDGE) {
                const p = _.pred_from[pe_cnt as usize] as usize;
                if (res.reachable_mask[p]) pred_count += 1;
                pe_cnt = _.pred_next[pe_cnt as usize];
            }

            if (pred_count >= 2) {
                const stop: usize = res.idom_arr[b] as usize;
                const b_bit: u64 = (1 as u64) << (b as u6);
                let pe: u16 = _.pred_head[b];
                while (pe != DOM_NO_EDGE) {
                    const p = _.pred_from[pe as usize] as usize;
                    if (res.reachable_mask[p]) {
                        let runner: usize = p;
                        let guard: usize = 0;
                        while (runner != stop && guard < DOM_MAX_NODES) : (guard += 1) {
                            res.frontier_mask[runner] |= b_bit;
                            if (runner == entry) break;
                            const next_r = res.idom_arr[runner];
                            if (next_r == DOM_NO_NODE || next_r as usize == runner) break;
                            runner = next_r as usize;
                        }
                    }
                    pe = _.pred_next[pe as usize];
                }
            }
        }

        return res;
    }
}
