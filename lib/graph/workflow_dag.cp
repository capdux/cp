// Origin: Pure-CP Stdlib (Zero-Heap DAG Topological Wave Planner, Critical Path Method & Cycle Finder) | Created: 2026-11-22
// Pure CP zero-heap-allocation deterministic workflow DAG planning primitives:
//   - Kahn topological sort + earliest parallel wave partitioning (`planParallelWaves`)
//   - Two-pass forward/backward Critical Path Method (CPM) DP computing ES/EF/LS/LF/slack/makespan (`analyzeCriticalPath`)
//   - 3-color DFS cycle extraction returning the exact cyclic node sequence (`findCycleNodes`)

const NO_CYCLE_PARENT: u32 = 0xFFFFFFFF;

fn satAddU64(a: u64, b: u64): u64 {
    if (a > 0xFFFFFFFFFFFFFFFF - b) return 0xFFFFFFFFFFFFFFFF;
    return a + b;
}

/// Optional node duration descriptor pairing `node_id` with execution cost `duration_ms`.
pub struct DagNodeCost {
    node_id: u32 = 0;
    duration_ms: u64 = 0;
}

/// Directed dependency edge (`from` must complete before `to` can start).
pub struct DagEdgeSpec {
    from: u32 = 0;
    to: u32 = 0;
}

/// Parallel wave assignment for a single node (`wave_index` is 0-based).
pub struct DagWaveAssignment {
    node_id: u32 = 0;
    wave_index: u32 = 0;
}

/// Full Critical Path Method (CPM) timing metrics for one DAG node.
pub struct CriticalPathMetrics {
    node_id: u32 = 0;
    duration_ms: u64 = 0;
    earliest_start: u64 = 0;
    earliest_finish: u64 = 0;
    latest_start: u64 = 0;
    latest_finish: u64 = 0;
    total_slack: u64 = 0;
    is_critical: bool = false;
}

/// Aggregate result of `planParallelWaves`.
pub struct WorkflowPlanSummary {
    node_count: usize = 0;
    scheduled_count: usize = 0;
    wave_count: u32 = 0;
    max_parallelism: usize = 0;
    has_cycle: bool = false;
    first_cycle_node: u32 = 0;
}

fn sortSliceAscU32(buf: *[]u32, start: usize, end: usize) {
    if (end <= start + 1) return;
    let i: usize = start + 1;
    while (i < end) : (i += 1) {
        const key = buf[i];
        let j: usize = i;
        while (j > start && buf[j - 1] > key) : (j -= 1) {
            buf[j] = buf[j - 1];
        }
        buf[j] = key;
    }
}

/// Computes Kahn topological order and earliest executable parallel waves with zero heap allocation.
/// All output and scratch slices must have length `>= node_count`.
/// Returns `null` if any slice is undersized or any edge references a node `>= node_count`.
/// On cycle detection, sets `has_cycle = true` and `first_cycle_node` to the smallest unscheduled node.
pub fn planParallelWaves(
    node_count: usize,
    edges: []const DagEdgeSpec,
    out_order: *[]u32,
    out_waves: *[]DagWaveAssignment,
    scratch_in_deg: *[]u32,
    scratch_queue: *[]u32,
): WorkflowPlanSummary? {
    if (node_count == 0) {
        return WorkflowPlanSummary{};
    }
    if (out_order.len < node_count ||
        out_waves.len < node_count ||
        scratch_in_deg.len < node_count ||
        scratch_queue.len < node_count)
    {
        return null;
    }

    let i: usize = 0;
    while (i < node_count) : (i += 1) {
        scratch_in_deg[i] = 0;
        out_waves[i] = DagWaveAssignment{ node_id: i as u32, wave_index: 0 };
    }

    let e: usize = 0;
    while (e < edges.len) : (e += 1) {
        const u = edges[e].from as usize;
        const v = edges[e].to as usize;
        if (u >= node_count || v >= node_count) return null;
        scratch_in_deg[v] += 1;
    }

    let head: usize = 0;
    let tail: usize = 0;
    let n: usize = 0;
    while (n < node_count) : (n += 1) {
        if (scratch_in_deg[n] == 0) {
            scratch_queue[tail] = n as u32;
            tail += 1;
        }
    }

    let scheduled: usize = 0;
    let wave_idx: u32 = 0;
    let max_par: usize = 0;

    while (head < tail) {
        const wave_end = tail;
        const wave_size = wave_end - head;
        if (wave_size > max_par) max_par = wave_size;

        while (head < wave_end) {
            const u = scratch_queue[head];
            head += 1;
            out_order[scheduled] = u;
            out_waves[u as usize] = DagWaveAssignment{
                node_id: u,
                wave_index: wave_idx,
            };
            scheduled += 1;

            let k: usize = 0;
            while (k < edges.len) : (k += 1) {
                if (edges[k].from == u) {
                    const v = edges[k].to as usize;
                    if (scratch_in_deg[v] > 0) {
                        scratch_in_deg[v] -= 1;
                        if (scratch_in_deg[v] == 0) {
                            scratch_queue[tail] = v as u32;
                            tail += 1;
                        }
                    }
                }
            }
        }
        // Sort next wave ascending by node_id for 100% deterministic topological order.
        sortSliceAscU32(scratch_queue, wave_end, tail);
        wave_idx += 1;
    }

    let has_cycle = false;
    let first_cyc: u32 = 0;
    if (scheduled < node_count) {
        has_cycle = true;
        let c: usize = 0;
        while (c < node_count) : (c += 1) {
            if (scratch_in_deg[c] > 0) {
                first_cyc = c as u32;
                break;
            }
        }
    }

    return WorkflowPlanSummary{
        node_count: node_count,
        scheduled_count: scheduled,
        wave_count: wave_idx,
        max_parallelism: max_par,
        has_cycle: has_cycle,
        first_cycle_node: first_cyc,
    };
}

/// Runs forward & backward Critical Path Method (CPM) dynamic programming over `topo_order`.
/// Populates `out_metrics[node_id]` with `earliest_start`, `earliest_finish`, `latest_start`,
/// `latest_finish`, `total_slack`, and `is_critical`, and returns total project makespan (ms).
/// Returns `null` on undersized buffers or out-of-range node indices.
pub fn analyzeCriticalPath(
    node_count: usize,
    durations_ms: []const u64,
    edges: []const DagEdgeSpec,
    topo_order: []const u32,
    out_metrics: *[]CriticalPathMetrics,
): u64? {
    if (node_count == 0) return 0;
    if (durations_ms.len < node_count ||
        topo_order.len < node_count ||
        out_metrics.len < node_count)
    {
        return null;
    }

    let i: usize = 0;
    while (i < node_count) : (i += 1) {
        if ((topo_order[i] as usize) >= node_count) return null;
        out_metrics[i] = CriticalPathMetrics{
            node_id: i as u32,
            duration_ms: durations_ms[i],
            earliest_start: 0,
            earliest_finish: durations_ms[i],
        };
    }

    let e: usize = 0;
    while (e < edges.len) : (e += 1) {
        if ((edges[e].from as usize) >= node_count || (edges[e].to as usize) >= node_count) {
            return null;
        }
    }

    // Forward pass: earliest_start / earliest_finish
    let f: usize = 0;
    while (f < node_count) : (f += 1) {
        const u = topo_order[f] as usize;
        const ef = satAddU64(out_metrics[u].earliest_start, out_metrics[u].duration_ms);
        out_metrics[u].earliest_finish = ef;

        let k: usize = 0;
        while (k < edges.len) : (k += 1) {
            if ((edges[k].from as usize) == u) {
                const v = edges[k].to as usize;
                if (ef > out_metrics[v].earliest_start) {
                    out_metrics[v].earliest_start = ef;
                }
            }
        }
    }

    let makespan: u64 = 0;
    let m: usize = 0;
    while (m < node_count) : (m += 1) {
        if (out_metrics[m].earliest_finish > makespan) {
            makespan = out_metrics[m].earliest_finish;
        }
    }

    // Backward pass: latest_finish / latest_start / total_slack / is_critical
    let b: usize = node_count;
    while (b > 0) {
        b -= 1;
        const u = topo_order[b] as usize;
        let has_succ = false;
        let min_succ_ls: u64 = makespan;

        let k: usize = 0;
        while (k < edges.len) : (k += 1) {
            if ((edges[k].from as usize) == u) {
                const v = edges[k].to as usize;
                const succ_ls = out_metrics[v].latest_start;
                if (!has_succ || succ_ls < min_succ_ls) {
                    min_succ_ls = succ_ls;
                    has_succ = true;
                }
            }
        }

        const lf: u64 = if (has_succ) min_succ_ls else makespan;
        const dur = out_metrics[u].duration_ms;
        const ls: u64 = if (lf >= dur) lf - dur else 0;
        const slack: u64 = if (ls >= out_metrics[u].earliest_start)
            ls - out_metrics[u].earliest_start
        else
            0;

        out_metrics[u].latest_finish = lf;
        out_metrics[u].latest_start = ls;
        out_metrics[u].total_slack = slack;
        out_metrics[u].is_critical = (slack == 0);
    }

    return makespan;
}

fn reconstructCycle(
    start_node: u32,
    end_node: u32,
    scratch_parent: []const u32,
    out_cycle_nodes: *[]u32,
): usize {
    if (out_cycle_nodes.len == 0) return 0;

    let count: usize = 0;
    let cur: u32 = end_node;
    while (count < out_cycle_nodes.len) {
        out_cycle_nodes[count] = cur;
        count += 1;
        if (cur == start_node) break;
        const p = scratch_parent[cur as usize];
        if (p == NO_CYCLE_PARENT) break;
        cur = p;
    }

    // Reverse in-place into forward cycle order [start_node, ..., end_node].
    let lo: usize = 0;
    let hi: usize = count;
    while (lo + 1 < hi) {
        hi -= 1;
        const tmp = out_cycle_nodes[lo];
        out_cycle_nodes[lo] = out_cycle_nodes[hi];
        out_cycle_nodes[hi] = tmp;
        lo += 1;
    }
    return count;
}

fn dfsCycleVisit(
    u: u32,
    node_count: usize,
    edges: []const DagEdgeSpec,
    out_cycle_nodes: *[]u32,
    scratch_color: *[]u8,
    scratch_parent: *[]u32,
): usize {
    scratch_color[u as usize] = 1; // Gray

    let k: usize = 0;
    while (k < edges.len) : (k += 1) {
        if (edges[k].from != u) continue;
        const v = edges[k].to;
        if ((v as usize) >= node_count) continue;

        const c = scratch_color[v as usize];
        if (c == 1) {
            return reconstructCycle(v, u, scratch_parent, out_cycle_nodes);
        }
        if (c == 0) {
            scratch_parent[v as usize] = u;
            const found = dfsCycleVisit(v, node_count, edges, out_cycle_nodes, scratch_color, scratch_parent);
            if (found > 0) return found;
        }
    }

    scratch_color[u as usize] = 2; // Black
    return 0;
}

/// Performs 3-color DFS to extract the exact node sequence of the first detected cycle.
/// Returns the number of cycle nodes written to `out_cycle_nodes`, or `0` if acyclic or invalid buffers.
pub fn findCycleNodes(
    node_count: usize,
    edges: []const DagEdgeSpec,
    out_cycle_nodes: *[]u32,
    scratch_color: *[]u8,
    scratch_parent: *[]u32,
): usize {
    if (node_count == 0 ||
        out_cycle_nodes.len == 0 ||
        scratch_color.len < node_count ||
        scratch_parent.len < node_count)
    {
        return 0;
    }

    let i: usize = 0;
    while (i < node_count) : (i += 1) {
        scratch_color[i] = 0;
        scratch_parent[i] = NO_CYCLE_PARENT;
    }

    let u: usize = 0;
    while (u < node_count) : (u += 1) {
        if (scratch_color[u] == 0) {
            const found = dfsCycleVisit(u as u32, node_count, edges, out_cycle_nodes, scratch_color, scratch_parent);
            if (found > 0) return found;
        }
    }
    return 0;
}
