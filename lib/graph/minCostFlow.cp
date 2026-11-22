// Origin: Pure-CP Stdlib (Zero-Alloc Min-Cost Max-Flow Solver with Johnson Potentials & SPFA) | Created: 2026-11-23
// Pure CP zero-heap-allocation Min-Cost Max-Flow (Successive Shortest Augmenting Path) engine:
//   - Fixed-capacity adjacency-list residual network (up to 64 nodes, 256 directed edges + 256 reverse edges)
//   - Exact i64 capacities and unit costs (zero floating-point drift)
//   - Multi-source SPFA potential initialization for negative-cost edges + strict negative-cycle detection
//   - Johnson reduced-cost potentials + zero-alloc binary min-heap Dijkstra augmentation
//   - Target flow limit `max_flow_limit`, per-edge flow inspection & bipartite min-cost transport helper

const std = @import("std");

// ============================================================================
// 1. Constants & Internal Min-Heap Helper
// ============================================================================

/// Maximum nodes supported inline in a zero-alloc `MinCostFlowNetwork`.
pub const MCMF_MAX_NODES: usize = 64;

/// Maximum original directed edges supported inline (internal residual array holds 2x = 512).
pub const MCMF_MAX_EDGES: usize = 256;

/// Sentinel adjacency edge index.
pub const MCMF_NO_EDGE: u16 = 0xFFFF;

/// Safe infinity sentinel for i64 shortest-path distances (headroom against overflow).
pub const MCMF_INF_COST: i64 = 0x3FFFFFFFFFFFFFFF;

/// Safe infinity sentinel for i64 flow limits.
pub const MCMF_INF_FLOW: i64 = 0x3FFFFFFFFFFFFFFF;

const MCMF_RESIDUAL_CAP: usize = 512;
const MCMF_HEAP_CAP: usize = 512;
const MCMF_MAX_UNIT_COST: i64 = 0x0FFFFFFFFFFFFFFF;

struct McmfHeapItem {
    node: u16 = 0;
    dist: i64 = 0;
}

fn heapPush(heap: *[]McmfHeapItem, size: *usize, item: McmfHeapItem): bool {
    if (size.* >= heap.len) return false;
    let idx: usize = size.*;
    heap[idx] = item;
    size.* = idx + 1;

    while (idx > 0) {
        const parent: usize = (idx - 1) / 2;
        if (heap[idx].dist < heap[parent].dist ||
            (heap[idx].dist == heap[parent].dist && heap[idx].node < heap[parent].node))
        {
            const tmp = heap[parent];
            heap[parent] = heap[idx];
            heap[idx] = tmp;
            idx = parent;
        } else {
            break;
        }
    }
    return true;
}

fn heapPop(heap: *[]McmfHeapItem, size: *usize): McmfHeapItem? {
    if (size.* == 0) return null;
    const top = heap[0];
    const last_idx: usize = size.* - 1;
    size.* = last_idx;
    if (last_idx == 0) return top;

    heap[0] = heap[last_idx];
    let root: usize = 0;
    while (true) {
        const left: usize = root * 2 + 1;
        if (left >= last_idx) break;
        let smallest: usize = root;
        if (heap[left].dist < heap[smallest].dist ||
            (heap[left].dist == heap[smallest].dist && heap[left].node < heap[smallest].node))
        {
            smallest = left;
        }
        const right: usize = left + 1;
        if (right < last_idx &&
            (heap[right].dist < heap[smallest].dist ||
                (heap[right].dist == heap[smallest].dist && heap[right].node < heap[smallest].node)))
        {
            smallest = right;
        }
        if (smallest == root) break;
        const tmp = heap[root];
        heap[root] = heap[smallest];
        heap[smallest] = tmp;
        root = smallest;
    }
    return top;
}

// ============================================================================
// 2. Public Models
// ============================================================================

/// Status codes returned by Min-Cost Max-Flow and transport solvers.
pub enum McmfStatus {
    ok,
    invalid_nodes,
    invalid_source_sink,
    negative_cycle,
    infeasible_demand,
    edge_capacity_exceeded,
}

/// Residual graph directed edge entry.
pub struct McmfEdge {
    to: u16 = 0;
    rev: u16 = 0;
    cap: i64 = 0;
    initial_cap: i64 = 0;
    cost: i64 = 0;
    next_edge: u16 = MCMF_NO_EDGE;
}

/// Outcome summary returned by `MinCostFlowNetwork.solve`.
pub struct McmfResult {
    status: McmfStatus = McmfStatus.ok;
    total_flow: i64 = 0;
    total_cost: i64 = 0;
    augmentations: usize = 0;
    reached_limit: bool = false;
    used_spfa_init: bool = false;
}

/// Outcome summary returned by `solveMinCostTransport`.
pub struct McmfTransportResult {
    status: McmfStatus = McmfStatus.ok;
    total_shipped: i64 = 0;
    total_demand: i64 = 0;
    total_cost: i64 = 0;
    fully_satisfied: bool = false;
}

// ============================================================================
// 3. Zero-Alloc Min-Cost Max-Flow Residual Network
// ============================================================================

/// Fixed-capacity zero-heap-allocation Min-Cost Max-Flow solver.
pub struct MinCostFlowNetwork {
    node_count: usize = 0;
    head: [64]u16 = [MCMF_NO_EDGE; 64],
    edges: [512]McmfEdge = [McmfEdge{}; 512],
    orig_edge_count: usize = 0,
    has_negative_edge: bool = false;

    /// Initializes a residual network with `node_count` nodes (`1 <= node_count <= 64`).
    static fn init(node_count: usize): MinCostFlowNetwork? {
        if (node_count == 0 || node_count > MCMF_MAX_NODES) return null;
        return MinCostFlowNetwork{
            node_count: node_count,
        };
    }

    /// Returns configured node count.
    fn nodeCount(): usize {
        return _.node_count;
    }

    /// Returns number of original directed edges added.
    fn edgeCount(): usize {
        return _.orig_edge_count;
    }

    /// Adds a directed edge `u -> v` with non-negative `capacity` and signed `unit_cost`,
    /// automatically wiring its paired reverse residual edge (`cap = 0`, `cost = -unit_cost`).
    /// Returns the 0-based original edge index for flow queries, or `null` if invalid/full.
    fn addEdge(*_, u: usize, v: usize, capacity: i64, unit_cost: i64): usize? {
        if (u >= _.node_count || v >= _.node_count) return null;
        if (capacity < 0) return null;
        if (unit_cost < (0 - MCMF_MAX_UNIT_COST) || unit_cost > MCMF_MAX_UNIT_COST) return null;
        if (_.orig_edge_count >= MCMF_MAX_EDGES) return null;

        const orig_idx: usize = _.orig_edge_count;
        const fwd_idx: usize = orig_idx * 2;
        const rev_idx: usize = fwd_idx + 1;

        _.edges[fwd_idx] = McmfEdge{
            to: v as u16,
            rev: rev_idx as u16,
            cap: capacity,
            initial_cap: capacity,
            cost: unit_cost,
            next_edge: _.head[u],
        };
        _.head[u] = fwd_idx as u16;

        _.edges[rev_idx] = McmfEdge{
            to: u as u16,
            rev: fwd_idx as u16,
            cap: 0,
            initial_cap: 0,
            cost: 0 - unit_cost,
            next_edge: _.head[v],
        };
        _.head[v] = rev_idx as u16;

        if (capacity > 0 && unit_cost < 0) {
            _.has_negative_edge = true;
        }
        _.orig_edge_count += 1;
        return orig_idx;
    }

    /// Returns the actual flow currently routed along original edge `edge_idx`.
    fn edgeFlow(edge_idx: usize): i64? {
        if (edge_idx >= _.orig_edge_count) return null;
        const e = _.edges[edge_idx * 2];
        return e.initial_cap - e.cap;
    }

    /// Returns the remaining residual capacity on original edge `edge_idx`.
    fn edgeResidualCap(edge_idx: usize): i64? {
        if (edge_idx >= _.orig_edge_count) return null;
        return _.edges[edge_idx * 2].cap;
    }

    /// Resets all edge flows to zero (restores `cap = initial_cap` on forward edges and `0` on reverse).
    fn resetFlows(*_) {
        let i: usize = 0;
        while (i < _.orig_edge_count) : (i += 1) {
            const fwd: usize = i * 2;
            const rev: usize = fwd + 1;
            _.edges[fwd].cap = _.edges[fwd].initial_cap;
            _.edges[rev].cap = 0;
        }
    }

    /// Multi-source SPFA potential initialization: finds valid Johnson potentials across all
    /// positive-capacity residual edges and returns `false` iff a negative-cost cycle exists.
    fn initPotentialsSpfa(potentials: *[]i64): bool {
        const n: usize = _.node_count;
        let in_queue: [64]bool = [false; 64];
        let relax_cnt: [64]usize = [0; 64];
        let queue: [64]u16 = [0; 64];
        let q_head: usize = 0;
        let q_tail: usize = 0;
        let q_len: usize = 0;

        let i: usize = 0;
        while (i < n) : (i += 1) {
            potentials[i] = 0;
            in_queue[i] = true;
            queue[q_tail] = i as u16;
            q_tail = (q_tail + 1) % MCMF_MAX_NODES;
            q_len += 1;
        }

        while (q_len > 0) {
            const u: usize = queue[q_head] as usize;
            q_head = (q_head + 1) % MCMF_MAX_NODES;
            q_len -= 1;
            in_queue[u] = false;

            let e_idx: u16 = _.head[u];
            while (e_idx != MCMF_NO_EDGE) {
                const e = _.edges[e_idx as usize];
                if (e.cap > 0) {
                    const v: usize = e.to as usize;
                    const cand: i64 = potentials[u] + e.cost;
                    if (cand < potentials[v]) {
                        potentials[v] = cand;
                        relax_cnt[v] += 1;
                        if (relax_cnt[v] >= n) {
                            return false; // Negative-cost cycle detected
                        }
                        if (!in_queue[v]) {
                            in_queue[v] = true;
                            queue[q_tail] = v as u16;
                            q_tail = (q_tail + 1) % MCMF_MAX_NODES;
                            q_len += 1;
                        }
                    }
                }
                e_idx = e.next_edge;
            }
        }
        return true;
    }

    /// Computes Min-Cost Flow from `source` to `sink` up to `max_flow_limit`
    /// (pass `MCMF_INF_FLOW` for unconstrained Min-Cost Max-Flow).
    fn solve(*_, source: usize, sink: usize, max_flow_limit: i64): McmfResult {
        const n: usize = _.node_count;
        if (n == 0 || n > MCMF_MAX_NODES) {
            return McmfResult{ status: McmfStatus.invalid_nodes };
        }
        if (source >= n || sink >= n || source == sink || max_flow_limit < 0) {
            return McmfResult{ status: McmfStatus.invalid_source_sink };
        }
        if (max_flow_limit == 0) {
            return McmfResult{
                status: McmfStatus.ok,
                total_flow: 0,
                total_cost: 0,
                augmentations: 0,
                reached_limit: true,
                used_spfa_init: false,
            };
        }

        let potential: [64]i64 = [0; 64];
        let pot_s: []i64 = potential[0..n];
        let used_spfa: bool = false;

        if (_.has_negative_edge) {
            used_spfa = true;
            if (!_.initPotentialsSpfa(*pot_s)) {
                return McmfResult{
                    status: McmfStatus.negative_cycle,
                    used_spfa_init: true,
                };
            }
        }

        let total_flow: i64 = 0;
        let total_cost: i64 = 0;
        let augmentations: usize = 0;

        let dist: [64]i64 = [MCMF_INF_COST; 64];
        let prev_node: [64]u16 = [0; 64];
        let prev_edge: [64]u16 = [MCMF_NO_EDGE; 64];
        let heap: [512]McmfHeapItem = [McmfHeapItem{}; 512];

        while (total_flow < max_flow_limit) {
            let i: usize = 0;
            while (i < n) : (i += 1) {
                dist[i] = MCMF_INF_COST;
                prev_node[i] = 0;
                prev_edge[i] = MCMF_NO_EDGE;
            }
            dist[source] = 0;

            let heap_s: []McmfHeapItem = heap[0..MCMF_HEAP_CAP];
            let heap_sz: usize = 0;
            * = heapPush(*heap_s, *heap_sz, McmfHeapItem{
                node: source as u16,
                dist: 0,
            });

            while (top := heapPop(*heap_s, *heap_sz)) {
                const u: usize = top.node as usize;
                if (top.dist != dist[u]) continue;

                let e_idx: u16 = _.head[u];
                while (e_idx != MCMF_NO_EDGE) {
                    const e = _.edges[e_idx as usize];
                    if (e.cap > 0) {
                        const v: usize = e.to as usize;
                        const rc: i64 = e.cost + potential[u] - potential[v];
                        const cand: i64 = top.dist + rc;
                        if (cand < dist[v]) {
                            dist[v] = cand;
                            prev_node[v] = u as u16;
                            prev_edge[v] = e_idx;
                            * = heapPush(*heap_s, *heap_sz, McmfHeapItem{
                                node: v as u16,
                                dist: cand,
                            });
                        }
                    }
                    e_idx = e.next_edge;
                }
            }

            if (dist[sink] == MCMF_INF_COST) {
                break; // No augmenting path remains
            }

            // Update Johnson potentials for all reachable nodes.
            i = 0;
            while (i < n) : (i += 1) {
                if (dist[i] < MCMF_INF_COST) {
                    potential[i] += dist[i];
                }
            }

            // Determine bottleneck augmentation capacity along shortest path.
            let push: i64 = max_flow_limit - total_flow;
            let cur: usize = sink;
            while (cur != source) {
                const pe: usize = prev_edge[cur] as usize;
                if (_.edges[pe].cap < push) {
                    push = _.edges[pe].cap;
                }
                cur = prev_node[cur] as usize;
            }

            // Apply augmentation & accumulate exact edge cost.
            cur = sink;
            while (cur != source) {
                const pe: usize = prev_edge[cur] as usize;
                const re: usize = _.edges[pe].rev as usize;
                _.edges[pe].cap -= push;
                _.edges[re].cap += push;
                total_cost += push * _.edges[pe].cost;
                cur = prev_node[cur] as usize;
            }

            total_flow += push;
            augmentations += 1;
        }

        return McmfResult{
            status: McmfStatus.ok,
            total_flow: total_flow,
            total_cost: total_cost,
            augmentations: augmentations,
            reached_limit: (total_flow == max_flow_limit),
            used_spfa_init: used_spfa,
        };
    }
}

// ============================================================================
// 4. Bipartite Min-Cost Supply/Demand Transport Helper
// ============================================================================

/// Solves a bipartite min-cost supply-to-demand transport / resource allocation problem:
/// - `supplies[i]` (`>= 0`): available supply at source `i` (`0..n_s`)
/// - `demands[j]` (`>= 0`): required demand at consumer `j` (`0..n_d`)
/// - `unit_costs[i * n_d + j]`: unit transport cost from supplier `i` to consumer `j`
/// - `out_flows[i * n_d + j]`: populated with optimal shipped quantity from `i` to `j`
pub fn solveMinCostTransport(
    supplies: []const i64,
    demands: []const i64,
    unit_costs: []const i64,
    out_flows: *[]i64,
): McmfTransportResult {
    const n_s: usize = supplies.len;
    const n_d: usize = demands.len;
    if (n_s == 0 || n_d == 0) {
        return McmfTransportResult{ status: McmfStatus.invalid_nodes };
    }
    const total_nodes: usize = n_s + n_d + 2;
    if (total_nodes > MCMF_MAX_NODES) {
        return McmfTransportResult{ status: McmfStatus.invalid_nodes };
    }
    if (n_s > std.math.maxInt(usize) / n_d) {
        return McmfTransportResult{ status: McmfStatus.edge_capacity_exceeded };
    }
    const pair_count: usize = n_s * n_d;
    if (pair_count + n_s + n_d > MCMF_MAX_EDGES) {
        return McmfTransportResult{ status: McmfStatus.edge_capacity_exceeded };
    }
    if (unit_costs.len < pair_count || out_flows.len < pair_count) {
        return McmfTransportResult{ status: McmfStatus.invalid_nodes };
    }

    let net = MinCostFlowNetwork.init(total_nodes) orelse {
        return McmfTransportResult{ status: McmfStatus.invalid_nodes };
    };
    const super_src: usize = 0;
    const super_sink: usize = n_s + n_d + 1;

    let total_demand: i64 = 0;
    let i: usize = 0;
    while (i < n_s) : (i += 1) {
        if (supplies[i] < 0) return McmfTransportResult{ status: McmfStatus.invalid_source_sink };
        * = net.addEdge(super_src, 1 + i, supplies[i], 0) orelse {
            return McmfTransportResult{ status: McmfStatus.edge_capacity_exceeded };
        };
    }

    let j: usize = 0;
    while (j < n_d) : (j += 1) {
        if (demands[j] < 0) return McmfTransportResult{ status: McmfStatus.invalid_source_sink };
        total_demand += demands[j];
        * = net.addEdge(1 + n_s + j, super_sink, demands[j], 0) orelse {
            return McmfTransportResult{ status: McmfStatus.edge_capacity_exceeded };
        };
    }

    // Record starting edge index for the n_s * n_d transport edges.
    const first_pair_edge: usize = net.edgeCount();
    i = 0;
    while (i < n_s) : (i += 1) {
        j = 0;
        while (j < n_d) : (j += 1) {
            const cap_ij: i64 = if (supplies[i] < demands[j]) supplies[i] else demands[j];
            const cost_ij: i64 = unit_costs[i * n_d + j];
            * = net.addEdge(1 + i, 1 + n_s + j, cap_ij, cost_ij) orelse {
                return McmfTransportResult{ status: McmfStatus.edge_capacity_exceeded };
            };
        }
    }

    const res = net.solve(super_src, super_sink, total_demand);
    if (res.status != McmfStatus.ok) {
        return McmfTransportResult{
            status: res.status,
            total_demand: total_demand,
        };
    }

    let p: usize = 0;
    while (p < pair_count) : (p += 1) {
        out_flows[p] = net.edgeFlow(first_pair_edge + p) orelse 0;
    }

    const satisfied: bool = (res.total_flow == total_demand);
    return McmfTransportResult{
        status: if (satisfied) McmfStatus.ok else McmfStatus.infeasible_demand,
        total_shipped: res.total_flow,
        total_demand: total_demand,
        total_cost: res.total_cost,
        fully_satisfied: satisfied,
    };
}
