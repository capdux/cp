// Origin: Pure-CP Stdlib (Zero-Alloc A* / Weighted A* Heuristic Graph & Grid Shortest-Path Engine) | Created: 2026-11-23
// Pure CP zero-heap-allocation A* and Weighted A* shortest-path engine:
//   - Fixed-capacity 128-node / 512-edge intrusive adjacency graph (`AStarGraph`)
//   - Five admissible/standard heuristics (`zero_dijkstra`, `manhattan`, `euclidean`, `chebyshev`, `octile`)
//   - Zero-heap indexed binary min-heap with O(log V) decrease-key and goal-biased tie-breaking
//     (`f = g + w * h`, breaking ties by larger `g` to minimize grid plateau expansions)
//   - Dynamic obstacle blocking (`setNodeBlocked`), expansion budget guard (`max_expansions`),
//     and start-to-goal chronological path reconstruction

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Fixed maximum number of nodes supported inline in `AStarGraph`.
pub const ASTAR_MAX_NODES: usize = 128;

/// Fixed maximum number of directed edges supported inline in `AStarGraph`.
pub const ASTAR_MAX_EDGES: usize = 512;

/// Sentinel node/edge index indicating no node or end of adjacency list.
pub const ASTAR_NO_NODE: u16 = 0xFFFF;

const ASTAR_INF: f32 = 1.0e30;
const ASTAR_SQRT2_MINUS_1: f32 = 0.41421356;

/// Heuristic distance function used to guide A* expansion priority.
pub enum AStarHeuristic {
    zero_dijkstra,
    manhattan,
    euclidean,
    chebyshev,
    octile,
}

/// Termination status of `AStarGraph.findPath`.
pub enum AStarStatus {
    found,
    no_path,
    invalid_node,
    expansion_limit_hit,
}

/// 2D spatial coordinate attached to each node in `AStarGraph`.
pub struct AStarNodeCoord {
    x: f32 = 0.0;
    y: f32 = 0.0;
}

/// Directed weighted edge stored in the inline adjacency pool.
pub struct AStarEdge {
    to: u16 = ASTAR_NO_NODE;
    weight: f32 = 0.0;
    next: u16 = ASTAR_NO_NODE;
}

/// Search configuration for A* / Weighted A*.
pub struct AStarConfig {
    heuristic: AStarHeuristic = AStarHeuristic.euclidean;
    /// Heuristic inflation weight (`1.0` = optimal A*, `> 1.0` = faster Weighted A*).
    heuristic_weight: f32 = 1.0;
    /// Optional expansion budget (`0` = unlimited).
    max_expansions: usize = 0;

    /// Return standard exact A* configuration (`euclidean`, weight `1.0`, unlimited expansions).
    static fn defaults(): AStarConfig {
        return AStarConfig{};
    }
}

/// Shortest-path search result returned by `AStarGraph.findPath`.
pub struct AStarResult {
    status: AStarStatus = AStarStatus.no_path;
    total_cost: f32 = 0.0;
    path_len: usize = 0;
    path: [ASTAR_MAX_NODES]u16 = [ASTAR_NO_NODE; ASTAR_MAX_NODES];
    nodes_expanded: usize = 0;
    nodes_pushed: usize = 0;
}

// ============================================================================
// 2. Pure Heuristic Math
// ============================================================================

fn absF32(v: f32): f32 {
    return if (v < 0.0) 0.0 - v else v;
}

fn sqrtF32(x: f32): f32 {
    if (x <= 0.0) return 0.0;
    let g: f32 = if (x > 1.0) x * 0.5 else 1.0;
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        g = 0.5 * (g + x / g);
    }
    return g;
}

/// Compute the 2D heuristic distance between coordinates `a` and `b`.
pub fn computeAStarHeuristic(a: AStarNodeCoord, b: AStarNodeCoord, kind: AStarHeuristic): f32 {
    if (kind == AStarHeuristic.zero_dijkstra) return 0.0;
    const dx: f32 = absF32(a.x - b.x);
    const dy: f32 = absF32(a.y - b.y);
    if (kind == AStarHeuristic.manhattan) return dx + dy;
    if (kind == AStarHeuristic.euclidean) return sqrtF32(dx * dx + dy * dy);
    const mx: f32 = if (dx > dy) dx else dy;
    const mn: f32 = if (dx < dy) dx else dy;
    if (kind == AStarHeuristic.chebyshev) return mx;
    // octile: max(dx, dy) + (sqrt(2) - 1) * min(dx, dy)
    return mx + ASTAR_SQRT2_MINUS_1 * mn;
}

fn heapBetter(
    a_node: u16,
    b_node: u16,
    f_score: []const f32,
    g_score: []const f32,
): bool {
    const fa = f_score[@as(usize, a_node)];
    const fb = f_score[@as(usize, b_node)];
    if (fa < fb) return true;
    if (fa > fb) return false;
    // Goal-biased tie-breaker: prefer larger g_score (closer to goal) on equal f_score.
    const ga = g_score[@as(usize, a_node)];
    const gb = g_score[@as(usize, b_node)];
    if (ga > gb) return true;
    if (ga < gb) return false;
    return a_node < b_node;
}

fn heapSiftUp(
    heap_nodes: *[]u16,
    heap_pos: *[]u16,
    start_idx: usize,
    f_score: []const f32,
    g_score: []const f32,
) {
    let idx = start_idx;
    while (idx > 0) {
        const parent_idx: usize = (idx - 1) / 2;
        const cur_node = heap_nodes[idx];
        const par_node = heap_nodes[parent_idx];
        if (!heapBetter(cur_node, par_node, f_score, g_score)) break;
        heap_nodes[idx] = par_node;
        heap_pos[@as(usize, par_node)] = @as(u16, @intCast(idx));
        heap_nodes[parent_idx] = cur_node;
        heap_pos[@as(usize, cur_node)] = @as(u16, @intCast(parent_idx));
        idx = parent_idx;
    }
}

fn heapSiftDown(
    heap_nodes: *[]u16,
    heap_pos: *[]u16,
    heap_size: usize,
    start_idx: usize,
    f_score: []const f32,
    g_score: []const f32,
) {
    let idx = start_idx;
    while (true) {
        const left: usize = idx * 2 + 1;
        const right: usize = left + 1;
        let smallest = idx;
        if (left < heap_size && heapBetter(heap_nodes[left], heap_nodes[smallest], f_score, g_score)) {
            smallest = left;
        }
        if (right < heap_size && heapBetter(heap_nodes[right], heap_nodes[smallest], f_score, g_score)) {
            smallest = right;
        }
        if (smallest == idx) break;
        const n_idx = heap_nodes[idx];
        const n_sm = heap_nodes[smallest];
        heap_nodes[idx] = n_sm;
        heap_pos[@as(usize, n_sm)] = @as(u16, @intCast(idx));
        heap_nodes[smallest] = n_idx;
        heap_pos[@as(usize, n_idx)] = @as(u16, @intCast(smallest));
        idx = smallest;
    }
}

// ============================================================================
// 3. AStarGraph — Zero-Heap Heuristic Shortest-Path Graph Engine
// ============================================================================

/// Zero-heap-allocation graph with 2D node coordinates, obstacle flags, and A* / Weighted A* search.
pub struct AStarGraph {
    coords: [ASTAR_MAX_NODES]AStarNodeCoord = [AStarNodeCoord{}; ASTAR_MAX_NODES];
    blocked: [ASTAR_MAX_NODES]bool = [false; ASTAR_MAX_NODES];
    head: [ASTAR_MAX_NODES]u16 = [ASTAR_NO_NODE; ASTAR_MAX_NODES];
    edges: [ASTAR_MAX_EDGES]AStarEdge = [AStarEdge{}; ASTAR_MAX_EDGES];
    node_count: usize = 0;
    edge_count: usize = 0;

    /// Initialize an empty `AStarGraph`.
    static fn init(): AStarGraph {
        return AStarGraph{};
    }

    /// Return the number of registered nodes.
    fn nodeCount(): usize {
        return node_count;
    }

    /// Return the number of registered directed edges.
    fn edgeCount(): usize {
        return edge_count;
    }

    /// Add a new node with 2D coordinates `(x, y)`.
    /// Returns its `u16` node ID, or `null` if `ASTAR_MAX_NODES` (128) is reached.
    fn addNode(*_, x: f32, y: f32): u16? {
        if (node_count >= ASTAR_MAX_NODES) return null;
        const id: u16 = @as(u16, @intCast(node_count));
        coords[node_count] = AStarNodeCoord{ x: x, y: y };
        blocked[node_count] = false;
        head[node_count] = ASTAR_NO_NODE;
        node_count += 1;
        return id;
    }

    /// Add a non-negative weighted directed edge `from -> to`.
    /// Returns `false` if node IDs are invalid, `weight < 0.0` (or NaN), or edge pool is full.
    fn addDirectedEdge(*_, from_node: u16, to_node: u16, weight: f32): bool {
        const u: usize = @as(usize, from_node);
        const v: usize = @as(usize, to_node);
        if (u >= node_count || v >= node_count) return false;
        if (!(weight >= 0.0)) return false; // rejects negative and NaN
        if (edge_count >= ASTAR_MAX_EDGES) return false;

        const ei: u16 = @as(u16, @intCast(edge_count));
        edges[edge_count] = AStarEdge{
            to: to_node,
            weight: weight,
            next: head[u],
        };
        head[u] = ei;
        edge_count += 1;
        return true;
    }

    /// Atomically add an undirected edge (`a -> b` and `b -> a`) with non-negative `weight`.
    fn addUndirectedEdge(*_, a: u16, b: u16, weight: f32): bool {
        const ua: usize = @as(usize, a);
        const ub: usize = @as(usize, b);
        if (ua >= node_count || ub >= node_count) return false;
        if (!(weight >= 0.0)) return false;
        if (edge_count + 2 > ASTAR_MAX_EDGES) return false;
        * = addDirectedEdge(a, b, weight);
        * = addDirectedEdge(b, a, weight);
        return true;
    }

    /// Mark or unmark `node_id` as an impassable obstacle.
    fn setNodeBlocked(*_, node_id: u16, is_blocked: bool): bool {
        const idx: usize = @as(usize, node_id);
        if (idx >= node_count) return false;
        blocked[idx] = is_blocked;
        return true;
    }

    /// Find a shortest path from `start` to `goal` using A* / Weighted A* with zero heap allocation.
    fn findPath(start: u16, goal: u16, cfg: AStarConfig): AStarResult {
        let res = AStarResult{};
        const s_idx: usize = @as(usize, start);
        const g_idx: usize = @as(usize, goal);

        if (s_idx >= node_count || g_idx >= node_count) {
            res.status = AStarStatus.invalid_node;
            return res;
        }
        if (blocked[s_idx] || blocked[g_idx]) {
            res.status = AStarStatus.no_path;
            return res;
        }
        if (start == goal) {
            res.status = AStarStatus.found;
            res.total_cost = 0.0;
            res.path_len = 1;
            res.path[0] = start;
            return res;
        }

        const w: f32 = if (cfg.heuristic_weight < 0.0 || !(cfg.heuristic_weight >= 0.0)) 0.0 else cfg.heuristic_weight;

        let g_score: [ASTAR_MAX_NODES]f32 = [ASTAR_INF; ASTAR_MAX_NODES];
        let f_score: [ASTAR_MAX_NODES]f32 = [ASTAR_INF; ASTAR_MAX_NODES];
        let parent: [ASTAR_MAX_NODES]u16 = [ASTAR_NO_NODE; ASTAR_MAX_NODES];
        let closed: [ASTAR_MAX_NODES]bool = [false; ASTAR_MAX_NODES];

        let heap_nodes: [ASTAR_MAX_NODES]u16 = [ASTAR_NO_NODE; ASTAR_MAX_NODES];
        let heap_pos: [ASTAR_MAX_NODES]u16 = [ASTAR_NO_NODE; ASTAR_MAX_NODES];
        let heap_size: usize = 0;

        let hn_s: []u16 = heap_nodes[0..];
        let hp_s: []u16 = heap_pos[0..];

        g_score[s_idx] = 0.0;
        f_score[s_idx] = w * computeAStarHeuristic(coords[s_idx], coords[g_idx], cfg.heuristic);
        heap_nodes[0] = start;
        heap_pos[s_idx] = 0;
        heap_size = 1;
        res.nodes_pushed = 1;

        while (heap_size > 0) {
            // Pop minimum-f node from indexed binary min-heap
            const u = heap_nodes[0];
            const u_idx: usize = @as(usize, u);
            heap_pos[u_idx] = ASTAR_NO_NODE;
            heap_size -= 1;
            if (heap_size > 0) {
                const last_node = heap_nodes[heap_size];
                heap_nodes[0] = last_node;
                heap_pos[@as(usize, last_node)] = 0;
                heapSiftDown(*hn_s, *hp_s, heap_size, 0, f_score[0..], g_score[0..]);
            }

            if (u == goal) {
                // Reconstruct path from goal back to start, then reverse into start -> goal order.
                let rev_len: usize = 0;
                let cur: u16 = goal;
                while (cur != ASTAR_NO_NODE && rev_len < ASTAR_MAX_NODES) {
                    res.path[rev_len] = cur;
                    rev_len += 1;
                    if (cur == start) break;
                    cur = parent[@as(usize, cur)];
                }
                let lo: usize = 0;
                let hi: usize = rev_len;
                while (lo + 1 < hi) {
                    hi -= 1;
                    const tmp = res.path[lo];
                    res.path[lo] = res.path[hi];
                    res.path[hi] = tmp;
                    lo += 1;
                }
                res.status = AStarStatus.found;
                res.total_cost = g_score[g_idx];
                res.path_len = rev_len;
                return res;
            }

            if (cfg.max_expansions > 0 && res.nodes_expanded >= cfg.max_expansions) {
                res.status = AStarStatus.expansion_limit_hit;
                return res;
            }

            closed[u_idx] = true;
            res.nodes_expanded += 1;

            let ei: u16 = head[u_idx];
            while (ei != ASTAR_NO_NODE) {
                const edge = edges[@as(usize, ei)];
                const v = edge.to;
                const v_idx: usize = @as(usize, v);
                if (!blocked[v_idx] && !closed[v_idx]) {
                    const tentative_g: f32 = g_score[u_idx] + edge.weight;
                    if (tentative_g < g_score[v_idx]) {
                        g_score[v_idx] = tentative_g;
                        const h: f32 = computeAStarHeuristic(coords[v_idx], coords[g_idx], cfg.heuristic);
                        f_score[v_idx] = tentative_g + w * h;
                        parent[v_idx] = u;

                        if (heap_pos[v_idx] == ASTAR_NO_NODE) {
                            const ins_pos = heap_size;
                            heap_nodes[ins_pos] = v;
                            heap_pos[v_idx] = @as(u16, @intCast(ins_pos));
                            heap_size += 1;
                            res.nodes_pushed += 1;
                            heapSiftUp(*hn_s, *hp_s, ins_pos, f_score[0..], g_score[0..]);
                        } else {
                            heapSiftUp(*hn_s, *hp_s, @as(usize, heap_pos[v_idx]), f_score[0..], g_score[0..]);
                        }
                    }
                }
                ei = edge.next;
            }
        }

        res.status = AStarStatus.no_path;
        return res;
    }
}
