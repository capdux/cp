// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Network Flow: Dinic Max-Flow, S-T Min-Cut & SPFA Min-Cost Max-Flow)

/// Sentinel edge index indicating the end of an adjacency linked list.
pub const NO_EDGE: i32 = -1;

/// Overflow-safe infinity sentinel for flow capacities and shortest-path costs.
pub const INF_FLOW: i64 = 0x3FFFFFFFFFFFFFFF;

/// Directed edge stored in a compressed forward-star static edge pool.
/// Forward edges reside at even indices `2k` and their paired residual reverse edges at `2k + 1`.
pub struct FlowEdge {
    to: usize,
    rev_next: i32,
    cap: i64,
    flow: i64,
    cost: i64,

    /// Remaining residual capacity (`cap - flow`) along this edge.
    fn residual(): i64 {
        return _.cap - _.flow;
    }
}

/// Zero-allocation flow network backed by caller-owned adjacency head and edge pool slices.
pub struct FlowGraph {
    num_nodes: usize,
    head: []i32,
    edges: []FlowEdge,
    edge_count: usize,

    /// Initializes a flow graph with `num_nodes` vertices (`0..num_nodes`).
    /// Returns `null` if `num_nodes == 0` or `head.len < num_nodes`.
    fn init(num_nodes: usize, head: []i32, edges: []FlowEdge): FlowGraph? {
        if (num_nodes == 0 || head.len < num_nodes) {
            return null;
        }
        let i: usize = 0;
        while (i < num_nodes) {
            head[i] = NO_EDGE;
            i += 1;
        }
        return FlowGraph {
            num_nodes: num_nodes,
            head: head,
            edges: edges,
            edge_count: 0,
        };
    }

    /// Appends a zero-cost directed edge `(u -> v, cap)` and its paired residual edge `(v -> u, 0)`.
    /// Returns `false` if vertices are out of range, `cap < 0`, or the edge pool lacks 2 free slots.
    fn addEdge(*_, u: usize, v: usize, cap: i64): bool {
        return _.addCostEdge(u, v, cap, 0);
    }

    /// Appends a directed edge `(u -> v, cap, cost)` and its paired residual edge `(v -> u, 0, -cost)`.
    /// Returns `false` if vertices are out of range, `cap < 0`, or the edge pool lacks 2 free slots.
    fn addCostEdge(*_, u: usize, v: usize, cap: i64, cost: i64): bool {
        if (u >= _.num_nodes || v >= _.num_nodes || cap < 0) {
            return false;
        }
        if (_.edge_count + 2 > _.edges.len || _.edge_count > 0x7FFFFFFD) {
            return false;
        }

        const fwd_idx = _.edge_count;
        const rev_idx = _.edge_count + 1;

        _.edges[fwd_idx] = FlowEdge {
            to: v,
            rev_next: _.head[u],
            cap: cap,
            flow: 0,
            cost: cost,
        };
        _.head[u] = fwd_idx as i32;

        _.edges[rev_idx] = FlowEdge {
            to: u,
            rev_next: _.head[v],
            cap: 0,
            flow: 0,
            cost: -cost,
        };
        _.head[v] = rev_idx as i32;

        _.edge_count += 2;
        return true;
    }

    /// Resets `flow = 0` on all active edges so the same network topology can be solved repeatedly.
    fn resetFlows(*_) {
        let i: usize = 0;
        while (i < _.edge_count) {
            _.edges[i].flow = 0;
            i += 1;
        }
    }
}

/// Optimal total flow and minimum total cost returned by `minCostMaxFlow`.
pub struct MinCostResult {
    flow: i64,
    cost: i64,
}

fn dinicBfs(
    g: *FlowGraph,
    source: usize,
    sink: usize,
    level: *[]i32,
    queue: *[]usize,
): bool {
    const n = g.num_nodes;
    let i: usize = 0;
    while (i < n) {
        level.*[i] = -1;
        i += 1;
    }

    level.*[source] = 0;
    let q_head: usize = 0;
    let q_tail: usize = 0;
    queue.*[q_tail] = source;
    q_tail += 1;

    while (q_head < q_tail) {
        const u = queue.*[q_head];
        q_head += 1;
        const next_lvl = level.*[u] + 1;

        let e_idx = g.head[u];
        while (e_idx != NO_EDGE) {
            const eu = e_idx as usize;
            const e = g.edges[eu];
            if (e.cap - e.flow > 0 && level.*[e.to] < 0) {
                level.*[e.to] = next_lvl;
                queue.*[q_tail] = e.to;
                q_tail += 1;
            }
            e_idx = e.rev_next;
        }
    }

    return level.*[sink] >= 0;
}

fn dinicDfs(
    g: *FlowGraph,
    u: usize,
    sink: usize,
    pushed: i64,
    level: *[]i32,
    iter: *[]i32,
): i64 {
    if (pushed == 0 || u == sink) {
        return pushed;
    }

    let e_idx = iter.*[u];
    while (e_idx != NO_EDGE) {
        iter.*[u] = e_idx;
        const eu = e_idx as usize;
        const e = g.edges[eu];
        const rem = e.cap - e.flow;
        if (rem > 0 && level.*[e.to] == level.*[u] + 1) {
            const next_push = if (pushed < rem) pushed else rem;
            const tr = dinicDfs(g, e.to, sink, next_push, level, iter);
            if (tr > 0) {
                g.edges[eu].flow += tr;
                g.edges[eu ^ 1].flow -= tr;
                return tr;
            }
        }
        e_idx = e.rev_next;
        iter.*[u] = e_idx;
    }

    return 0;
}

/// Computes the maximum `source -> sink` flow using Dinic's algorithm (BFS level graph + DFS with current-arc optimization).
/// All scratch slices (`level_scratch`, `iter_scratch`, `queue_scratch`) must have length `>= g.num_nodes`.
/// Returns `null` on invalid graph state, `source == sink`, out-of-range vertices, or undersized scratch buffers.
pub fn dinicMaxFlow(
    g: *FlowGraph,
    source: usize,
    sink: usize,
    level_scratch: *[]i32,
    iter_scratch: *[]i32,
    queue_scratch: *[]usize,
): i64? {
    const n = g.num_nodes;
    if (n == 0 || source >= n || sink >= n || source == sink) {
        return null;
    }
    if (g.head.len < n || g.edge_count > g.edges.len) {
        return null;
    }
    if (
        level_scratch.*.len < n ||
        iter_scratch.*.len < n ||
        queue_scratch.*.len < n
    ) {
        return null;
    }

    let total_flow: i64 = 0;
    while (dinicBfs(g, source, sink, level_scratch, queue_scratch)) {
        let i: usize = 0;
        while (i < n) {
            iter_scratch.*[i] = g.head[i];
            i += 1;
        }

        while (true) {
            const pushed = dinicDfs(g, source, sink, INF_FLOW, level_scratch, iter_scratch);
            if (pushed == 0) {
                break;
            }
            total_flow += pushed;
        }
    }

    return total_flow;
}

/// Performs a residual-graph BFS from `source` after a max-flow computation to extract the S-T minimum cut.
/// Marks vertices reachable from `source` via edges with positive residual capacity as `1` (S-side) and others as `0` (T-side).
/// Returns the number of S-side vertices, or `null` on invalid inputs or undersized scratch slices.
pub fn minCutReachable(
    g: FlowGraph,
    source: usize,
    reachable_out: *[]u8,
    queue_scratch: *[]usize,
): usize? {
    const n = g.num_nodes;
    if (n == 0 || source >= n) {
        return null;
    }
    if (g.head.len < n || g.edge_count > g.edges.len) {
        return null;
    }
    if (reachable_out.*.len < n || queue_scratch.*.len < n) {
        return null;
    }

    let i: usize = 0;
    while (i < n) {
        reachable_out.*[i] = 0;
        i += 1;
    }

    reachable_out.*[source] = 1;
    let q_head: usize = 0;
    let q_tail: usize = 0;
    queue_scratch.*[q_tail] = source;
    q_tail += 1;

    while (q_head < q_tail) {
        const u = queue_scratch.*[q_head];
        q_head += 1;

        let e_idx = g.head[u];
        while (e_idx != NO_EDGE) {
            const eu = e_idx as usize;
            const e = g.edges[eu];
            if (e.cap - e.flow > 0 && reachable_out.*[e.to] == 0) {
                reachable_out.*[e.to] = 1;
                queue_scratch.*[q_tail] = e.to;
                q_tail += 1;
            }
            e_idx = e.rev_next;
        }
    }

    return q_tail;
}

/// Computes the Minimum-Cost Maximum Flow (MCMF) from `source` to `sink` using Successive Shortest Augmenting Paths (SPFA).
/// All scratch slices (`dist_scratch`, `prev_node`, `prev_edge`, `in_queue`, `queue_scratch`) must have length `>= g.num_nodes`.
/// Returns `null` on invalid inputs, undersized scratch buffers, or if a reachable negative-cost cycle is detected.
pub fn minCostMaxFlow(
    g: *FlowGraph,
    source: usize,
    sink: usize,
    dist_scratch: *[]i64,
    prev_node: *[]i32,
    prev_edge: *[]i32,
    in_queue: *[]u8,
    queue_scratch: *[]usize,
): MinCostResult? {
    const n = g.num_nodes;
    if (n == 0 || source >= n || sink >= n || source == sink) {
        return null;
    }
    if (g.head.len < n || g.edge_count > g.edges.len) {
        return null;
    }
    if (
        dist_scratch.*.len < n ||
        prev_node.*.len < n ||
        prev_edge.*.len < n ||
        in_queue.*.len < n ||
        queue_scratch.*.len < n
    ) {
        return null;
    }

    const q_cap = queue_scratch.*.len;
    const max_relax: usize = n * (g.edge_count + 1);
    let total_flow: i64 = 0;
    let total_cost: i64 = 0;

    while (true) {
        let i: usize = 0;
        while (i < n) {
            dist_scratch.*[i] = INF_FLOW;
            prev_node.*[i] = NO_EDGE;
            prev_edge.*[i] = NO_EDGE;
            in_queue.*[i] = 0;
            i += 1;
        }

        dist_scratch.*[source] = 0;
        in_queue.*[source] = 1;

        let q_head: usize = 0;
        let q_tail: usize = 0;
        let q_count: usize = 0;
        queue_scratch.*[q_tail] = source;
        q_tail += 1;
        if (q_tail == q_cap) {
            q_tail = 0;
        }
        q_count += 1;

        let relax_ops: usize = 0;
        while (q_count > 0) {
            const u = queue_scratch.*[q_head];
            q_head += 1;
            if (q_head == q_cap) {
                q_head = 0;
            }
            q_count -= 1;
            in_queue.*[u] = 0;

            const du = dist_scratch.*[u];
            if (du == INF_FLOW) {
                continue;
            }

            let e_idx = g.head[u];
            while (e_idx != NO_EDGE) {
                const eu = e_idx as usize;
                const e = g.edges[eu];
                if (e.cap - e.flow > 0) {
                    const nd = du + e.cost;
                    if (nd < dist_scratch.*[e.to]) {
                        dist_scratch.*[e.to] = nd;
                        prev_node.*[e.to] = u as i32;
                        prev_edge.*[e.to] = e_idx;
                        relax_ops += 1;
                        if (relax_ops > max_relax) {
                            return null;
                        }
                        if (in_queue.*[e.to] == 0) {
                            in_queue.*[e.to] = 1;
                            queue_scratch.*[q_tail] = e.to;
                            q_tail += 1;
                            if (q_tail == q_cap) {
                                q_tail = 0;
                            }
                            q_count += 1;
                        }
                    }
                }
                e_idx = e.rev_next;
            }
        }

        if (dist_scratch.*[sink] == INF_FLOW) {
            break;
        }

        // Compute bottleneck residual capacity along shortest augmenting path
        let push: i64 = INF_FLOW;
        let cur: usize = sink;
        while (cur != source) {
            const pe = prev_edge.*[cur];
            const pn = prev_node.*[cur];
            if (pe == NO_EDGE || pn == NO_EDGE) {
                push = 0;
                break;
            }
            const rem = g.edges[pe as usize].cap - g.edges[pe as usize].flow;
            if (rem < push) {
                push = rem;
            }
            cur = pn as usize;
        }

        if (push <= 0) {
            break;
        }

        // Augment flow along path and paired residual edges
        cur = sink;
        while (cur != source) {
            const peu = prev_edge.*[cur] as usize;
            g.edges[peu].flow += push;
            g.edges[peu ^ 1].flow -= push;
            cur = prev_node.*[cur] as usize;
        }

        total_flow += push;
        total_cost += push * dist_scratch.*[sink];
    }

    return MinCostResult {
        flow: total_flow,
        cost: total_cost,
    };
}
