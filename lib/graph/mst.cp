// Origin: Pure-CP Stdlib (Zero-Alloc Minimum/Maximum Spanning Tree & Forest: DisjointSet, Kruskal & Dense O(V^2) Prim) | Created: 2026-11-22

/// Sentinel weight representing absence of an edge in a dense adjacency matrix.
pub const MST_INF_WEIGHT: f64 = 1.0e300;

/// Undirected weighted edge with caller-assigned identifier.
pub struct WeightedEdge {
    u: usize,
    v: usize,
    weight: f64,
    id: u32,

    fn init(u: usize, v: usize, weight: f64, id: u32): WeightedEdge {
        return WeightedEdge {
            u: u,
            v: v,
            weight: weight,
            id: id,
        };
    }
}

/// Tree edge emitted by `primDenseMst`.
pub struct PrimEdge {
    u: usize,
    v: usize,
    weight: f64,
}

/// Summary returned by `kruskalMst`, `maxSpanningTree`, and `primDenseMst`.
pub struct MstResult {
    totalWeight: f64,
    edgeCount: usize,
    componentCount: usize,
    isConnected: bool,
}

/// Validation outcome returned by `isValidSpanningTree`.
pub struct SpanningTreeCheck {
    valid: bool,
    totalWeight: f64,
}

/// Zero-allocation Union-Find (Disjoint Set Union) with iterative path compression and union by rank.
pub struct DisjointSet {
    parent: []usize,
    rank: []u8,
    num_nodes: usize,
    component_count: usize,

    /// Initializes `num_nodes` singleton sets using caller-owned `parent` and `rank` slices.
    /// Returns `null` if either slice has length `< num_nodes`.
    static fn init(num_nodes: usize, parent: []usize, rank: []u8): DisjointSet? {
        if (parent.len < num_nodes || rank.len < num_nodes) {
            return null;
        }
        let i: usize = 0;
        while (i < num_nodes) {
            parent[i] = i;
            rank[i] = 0;
            i += 1;
        }
        return DisjointSet {
            parent: parent,
            rank: rank,
            num_nodes: num_nodes,
            component_count: num_nodes,
        };
    }

    /// Finds the representative root of `x` with two-pass iterative path compression.
    /// Returns `null` if `x >= num_nodes`.
    fn find(*_, x: usize): usize? {
        if (x >= _.num_nodes) {
            return null;
        }
        let root = x;
        while (_.parent[root] != root) {
            root = _.parent[root];
        }
        let cur = x;
        while (_.parent[cur] != root) {
            const nxt = _.parent[cur];
            _.parent[cur] = root;
            cur = nxt;
        }
        return root;
    }

    /// Unions the sets containing `a` and `b` using union by rank.
    /// Returns `true` if two distinct components were merged, `false` if already connected or out of bounds.
    fn unite(*_, a: usize, b: usize): bool {
        const ra = _.find(a) ?? return false;
        const rb = _.find(b) ?? return false;
        if (ra == rb) {
            return false;
        }
        if (_.rank[ra] < _.rank[rb]) {
            _.parent[ra] = rb;
        } else if (_.rank[ra] > _.rank[rb]) {
            _.parent[rb] = ra;
        } else {
            _.parent[rb] = ra;
            _.rank[ra] += 1;
        }
        _.component_count -= 1;
        return true;
    }

    /// Returns `true` if `a` and `b` belong to the same connected component.
    fn connected(*_, a: usize, b: usize): bool {
        const ra = _.find(a) ?? return false;
        const rb = _.find(b) ?? return false;
        return ra == rb;
    }

    /// Returns the current number of disjoint components.
    fn components(): usize {
        return _.component_count;
    }
}

fn edgeComesAfter(
    edges: []const WeightedEdge,
    a_idx: usize,
    b_idx: usize,
    descending: bool,
): bool {
    const ea = edges[a_idx];
    const eb = edges[b_idx];
    if (ea.weight != eb.weight) {
        if (descending) {
            return ea.weight < eb.weight;
        }
        return ea.weight > eb.weight;
    }
    if (ea.id != eb.id) {
        return ea.id > eb.id;
    }
    return a_idx > b_idx;
}

fn siftDownEdgeOrder(
    edges: []const WeightedEdge,
    order: *[]usize,
    start: usize,
    end: usize,
    descending: bool,
) {
    let root = start;
    while (true) {
        const left = (root << 1) + 1;
        if (left >= end) {
            break;
        }
        let swap_idx = root;
        if (edgeComesAfter(edges, order.*[left], order.*[swap_idx], descending)) {
            swap_idx = left;
        }
        const right = left + 1;
        if (right < end && edgeComesAfter(edges, order.*[right], order.*[swap_idx], descending)) {
            swap_idx = right;
        }
        if (swap_idx == root) {
            break;
        }
        const tmp = order.*[root];
        order.*[root] = order.*[swap_idx];
        order.*[swap_idx] = tmp;
        root = swap_idx;
    }
}

fn sortEdgeIndices(
    edges: []const WeightedEdge,
    order: *[]usize,
    n: usize,
    descending: bool,
) {
    if (n <= 1) {
        return;
    }
    // Build max-heap
    let i: usize = n >> 1;
    while (i > 0) {
        i -= 1;
        siftDownEdgeOrder(edges, order, i, n, descending);
    }
    // Extract elements one by one
    let end: usize = n;
    while (end > 1) {
        end -= 1;
        const tmp = order.*[0];
        order.*[0] = order.*[end];
        order.*[end] = tmp;
        siftDownEdgeOrder(edges, order, 0, end, descending);
    }
}

fn kruskalSolve(
    num_nodes: usize,
    edges: []const WeightedEdge,
    parent_scratch: *[]usize,
    rank_scratch: *[]u8,
    order_scratch: *[]usize,
    selectedEdgesOut: *[]usize,
    descending: bool,
): MstResult? {
    if (parent_scratch.*.len < num_nodes || rank_scratch.*.len < num_nodes) {
        return null;
    }
    if (order_scratch.*.len < edges.len) {
        return null;
    }
    if (num_nodes > 0 && selectedEdgesOut.*.len < num_nodes - 1) {
        return null;
    }

    // Validate all edge endpoints and reject NaN weights
    let i: usize = 0;
    while (i < edges.len) {
        const e = edges[i];
        if (e.u >= num_nodes || e.v >= num_nodes || e.weight != e.weight) {
            return null;
        }
        order_scratch.*[i] = i;
        i += 1;
    }

    if (num_nodes == 0) {
        return MstResult {
            totalWeight: 0.0,
            edgeCount: 0,
            componentCount: 0,
            isConnected: false,
        };
    }

    sortEdgeIndices(edges, order_scratch, edges.len, descending);

    let dsu = DisjointSet.init(num_nodes, parent_scratch.*, rank_scratch.*) ?? return null;
    let total_weight: f64 = 0.0;
    let selected: usize = 0;

    i = 0;
    while (i < edges.len && selected + 1 < num_nodes) {
        const edge_idx = order_scratch.*[i];
        const e = edges[edge_idx];
        if (e.u != e.v && dsu.unite(e.u, e.v)) {
            selectedEdgesOut.*[selected] = edge_idx;
            selected += 1;
            total_weight += e.weight;
        }
        i += 1;
    }

    const comps = dsu.components();
    return MstResult {
        totalWeight: total_weight,
        edgeCount: selected,
        componentCount: comps,
        isConnected: comps == 1,
    };
}

/// Computes a Minimum Spanning Tree (or Minimum Spanning Forest on disconnected graphs) via Kruskal's algorithm.
/// Writes selected edge indices into `selectedEdgesOut[0..result.edgeCount]` without mutating `edges`.
/// Returns `null` on out-of-range vertices, NaN weights, or undersized scratch/output buffers.
pub fn kruskalMst(
    num_nodes: usize,
    edges: []const WeightedEdge,
    parent_scratch: *[]usize,
    rank_scratch: *[]u8,
    order_scratch: *[]usize,
    selectedEdgesOut: *[]usize,
): MstResult? {
    return kruskalSolve(num_nodes, edges, parent_scratch, rank_scratch, order_scratch, selectedEdgesOut, false);
}

/// Computes a Maximum Spanning Tree (or Maximum Spanning Forest on disconnected graphs) via Kruskal's algorithm.
/// Writes selected edge indices into `selectedEdgesOut[0..result.edgeCount]` without mutating `edges`.
/// Returns `null` on out-of-range vertices, NaN weights, or undersized scratch/output buffers.
pub fn maxSpanningTree(
    num_nodes: usize,
    edges: []const WeightedEdge,
    parent_scratch: *[]usize,
    rank_scratch: *[]u8,
    order_scratch: *[]usize,
    selectedEdgesOut: *[]usize,
): MstResult? {
    return kruskalSolve(num_nodes, edges, parent_scratch, rank_scratch, order_scratch, selectedEdgesOut, true);
}

/// Computes a Minimum Spanning Tree / Forest on a dense `num_nodes x num_nodes` row-major adjacency matrix in O(V^2) time.
/// Entries `>= MST_INF_WEIGHT` or NaN denote absent edges. Writes chosen tree edges into `selectedEdgesOut`.
/// Returns `null` if `adj_matrix` or any scratch/output buffer is undersized.
pub fn primDenseMst(
    num_nodes: usize,
    adj_matrix: []const f64,
    min_weight_scratch: *[]f64,
    parent_scratch: *[]i32,
    in_mst_scratch: *[]u8,
    selectedEdgesOut: *[]PrimEdge,
): MstResult? {
    if (adj_matrix.len < num_nodes * num_nodes) {
        return null;
    }
    if (
        min_weight_scratch.*.len < num_nodes ||
        parent_scratch.*.len < num_nodes ||
        in_mst_scratch.*.len < num_nodes
    ) {
        return null;
    }
    if (num_nodes > 0 && selectedEdgesOut.*.len < num_nodes - 1) {
        return null;
    }
    if (num_nodes == 0) {
        return MstResult {
            totalWeight: 0.0,
            edgeCount: 0,
            componentCount: 0,
            isConnected: false,
        };
    }

    let i: usize = 0;
    while (i < num_nodes) {
        min_weight_scratch.*[i] = MST_INF_WEIGHT;
        parent_scratch.*[i] = -1;
        in_mst_scratch.*[i] = 0;
        i += 1;
    }

    let total_weight: f64 = 0.0;
    let edge_count: usize = 0;
    let component_count: usize = 0;

    let step: usize = 0;
    while (step < num_nodes) {
        let best_u: usize = num_nodes;
        let best_w: f64 = MST_INF_WEIGHT;
        let first_unvisited: usize = num_nodes;

        let v: usize = 0;
        while (v < num_nodes) {
            if (in_mst_scratch.*[v] == 0) {
                if (first_unvisited == num_nodes) {
                    first_unvisited = v;
                }
                const mw = min_weight_scratch.*[v];
                if (mw < best_w) {
                    best_w = mw;
                    best_u = v;
                }
            }
            v += 1;
        }

        if (best_u == num_nodes) {
            // Start a new connected component at the first unvisited vertex
            best_u = first_unvisited;
            component_count += 1;
            parent_scratch.*[best_u] = -1;
        }

        in_mst_scratch.*[best_u] = 1;
        const p = parent_scratch.*[best_u];
        if (p >= 0) {
            selectedEdgesOut.*[edge_count] = PrimEdge {
                u: p as usize,
                v: best_u,
                weight: best_w,
            };
            edge_count += 1;
            total_weight += best_w;
        }

        // Relax edges from best_u to all remaining unvisited vertices
        const row_off = best_u * num_nodes;
        v = 0;
        while (v < num_nodes) {
            if (in_mst_scratch.*[v] == 0 && v != best_u) {
                const w = adj_matrix[row_off + v];
                if (w == w && w < MST_INF_WEIGHT && w < min_weight_scratch.*[v]) {
                    min_weight_scratch.*[v] = w;
                    parent_scratch.*[v] = best_u as i32;
                }
            }
            v += 1;
        }

        step += 1;
    }

    return MstResult {
        totalWeight: total_weight,
        edgeCount: edge_count,
        componentCount: component_count,
        isConnected: component_count == 1,
    };
}

/// Validates whether `selected_indices` forms an acyclic connected spanning tree over `0..num_nodes`
/// and computes its total edge weight.
pub fn isValidSpanningTree(
    num_nodes: usize,
    edges: []const WeightedEdge,
    selected_indices: []const usize,
    parent_scratch: *[]usize,
    rank_scratch: *[]u8,
): SpanningTreeCheck {
    if (num_nodes == 0 || selected_indices.len != num_nodes - 1) {
        return SpanningTreeCheck { valid: false, totalWeight: 0.0 };
    }
    let dsu = DisjointSet.init(num_nodes, parent_scratch.*, rank_scratch.*) ?? {
        return SpanningTreeCheck { valid: false, totalWeight: 0.0 };
    };

    let total_weight: f64 = 0.0;
    let i: usize = 0;
    while (i < selected_indices.len) {
        const idx = selected_indices[i];
        if (idx >= edges.len) {
            return SpanningTreeCheck { valid: false, totalWeight: 0.0 };
        }
        const e = edges[idx];
        if (e.u >= num_nodes || e.v >= num_nodes || e.u == e.v || e.weight != e.weight) {
            return SpanningTreeCheck { valid: false, totalWeight: 0.0 };
        }
        if (!dsu.unite(e.u, e.v)) {
            return SpanningTreeCheck { valid: false, totalWeight: 0.0 };
        }
        total_weight += e.weight;
        i += 1;
    }

    if (dsu.components() != 1) {
        return SpanningTreeCheck { valid: false, totalWeight: 0.0 };
    }
    return SpanningTreeCheck {
        valid: true,
        totalWeight: total_weight,
    };
}
