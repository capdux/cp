// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Indexed-Heap Dijkstra, Bellman-Ford & Floyd-Warshall Shortest Path Algorithms)

import std from "std";

/// Sentinel indicating no predecessor / no next hop in shortest-path parent/next arrays.
pub const NO_PARENT: i64 = -1;

fn isFiniteF64(x: f64): bool {
    return (x == x) && ((x - x) == 0.0);
}

fn infF64(): f64 {
    return std.math.inf(f64);
}

/// Returns `true` if a shortest-path distance value is finite (i.e. vertex is reachable).
pub fn isReachable(dist: f64): bool {
    return isFiniteF64(dist);
}

/// Directed weighted edge `(from -> to)` with floating-point `weight`.
pub struct Edge {
    from: usize;
    to: usize;
    weight: f64;
}

/// Compressed Sparse Row (CSR) representation of a weighted directed graph.
///
/// Outgoing edges of vertex `u` occupy `col_idx[row_ptr[u]..row_ptr[u + 1]]`
/// with corresponding weights `weights[row_ptr[u]..row_ptr[u + 1]]`.
pub struct WeightedCsrGraph {
    num_nodes: usize;
    row_ptr: []const usize;
    col_idx: []const usize;
    weights: []const f64;

    /// Validates CSR structural invariants and ensures all active edge weights are finite.
    fn validate(): bool {
        if (_.row_ptr.len != _.num_nodes + 1) {
            return false;
        }
        if (_.row_ptr[0] != 0) {
            return false;
        }
        let u: usize = 0;
        while (u < _.num_nodes) {
            if (_.row_ptr[u] > _.row_ptr[u + 1]) {
                return false;
            }
            u += 1;
        }
        const total_edges = _.row_ptr[_.num_nodes];
        if (total_edges > _.col_idx.len || total_edges > _.weights.len) {
            return false;
        }
        let e: usize = 0;
        while (e < total_edges) {
            if (_.col_idx[e] >= _.num_nodes || !isFiniteF64(_.weights[e])) {
                return false;
            }
            e += 1;
        }
        return true;
    }

    /// Returns the total number of directed edges stored in the graph.
    fn numEdges(): usize {
        if (_.row_ptr.len <= _.num_nodes) {
            return 0;
        }
        return _.row_ptr[_.num_nodes];
    }

    /// Returns the out-degree of vertex `u` (or `0` if `u` is out of bounds or malformed).
    fn outDegree(u: usize): usize {
        if (u >= _.num_nodes || _.row_ptr.len <= u + 1) {
            return 0;
        }
        const start = _.row_ptr[u];
        const end = _.row_ptr[u + 1];
        if (end < start) {
            return 0;
        }
        return end - start;
    }

    /// Returns the outgoing neighbor slice of vertex `u`.
    fn neighbors(u: usize): []const usize {
        if (u >= _.num_nodes || _.row_ptr.len <= u + 1) {
            return _.col_idx[0..0];
        }
        const start = _.row_ptr[u];
        const end = _.row_ptr[u + 1];
        if (start > end || end > _.col_idx.len) {
            return _.col_idx[0..0];
        }
        return _.col_idx[start..end];
    }

    /// Returns the outgoing edge weight slice of vertex `u`.
    fn edgeWeights(u: usize): []const f64 {
        if (u >= _.num_nodes || _.row_ptr.len <= u + 1) {
            return _.weights[0..0];
        }
        const start = _.row_ptr[u];
        const end = _.row_ptr[u + 1];
        if (start > end || end > _.weights.len) {
            return _.weights[0..0];
        }
        return _.weights[start..end];
    }
}

/// Caller-provided zero-allocation workspace for the Indexed Binary Min-Heap used by `dijkstra`.
///
/// Each slice must have length `>= graph.num_nodes`.
pub struct DijkstraWorkspace {
    heap_nodes: []usize;
    heap_pos: []i64;
    visited: []u8;
}

/// Summary metrics returned by `dijkstra`.
pub struct DijkstraResult {
    settled_nodes: usize;
    relaxations: usize;
}

/// Summary metrics returned by `bellmanFord`.
pub struct BellmanFordResult {
    has_negative_cycle: bool;
    iterations: usize;
}

/// Summary metrics returned by `floydWarshall`.
pub struct FloydWarshallResult {
    has_negative_cycle: bool;
}

fn heapSwap(heap_nodes: []usize, heap_pos: []i64, a: usize, b: usize) {
    const na = heap_nodes[a];
    const nb = heap_nodes[b];
    heap_nodes[a] = nb;
    heap_nodes[b] = na;
    heap_pos[nb] = a as i64;
    heap_pos[na] = b as i64;
}

fn heapSiftUp(heap_nodes: []usize, heap_pos: []i64, dist: []const f64, start_idx: usize) {
    let idx = start_idx;
    while (idx > 0) {
        const parent = (idx - 1) / 2;
        if (dist[heap_nodes[idx]] < dist[heap_nodes[parent]]) {
            heapSwap(heap_nodes, heap_pos, idx, parent);
            idx = parent;
        } else {
            break;
        }
    }
}

fn heapSiftDown(heap_nodes: []usize, heap_pos: []i64, dist: []const f64, heap_size: usize, start_idx: usize) {
    let idx = start_idx;
    while (true) {
        const left = idx * 2 + 1;
        if (left >= heap_size) {
            break;
        }
        let smallest = left;
        const right = left + 1;
        if (right < heap_size && dist[heap_nodes[right]] < dist[heap_nodes[left]]) {
            smallest = right;
        }
        if (dist[heap_nodes[smallest]] < dist[heap_nodes[idx]]) {
            heapSwap(heap_nodes, heap_pos, idx, smallest);
            idx = smallest;
        } else {
            break;
        }
    }
}

/// Single-source shortest paths on a non-negative weighted CSR directed graph using
/// an O((V + E) log V) Indexed Binary Min-Heap with in-place decrease-key.
///
/// Writes shortest distances into `dist_out[0..n]` (`+inf` for unreachable vertices) and
/// predecessors into `parent_out[0..n]` (`NO_PARENT` for `src` and unreachable vertices).
///
/// Returns `null` if the graph is invalid/empty, contains any negative edge weight,
/// `src >= graph.num_nodes`, or any output/workspace buffer has length `< graph.num_nodes`.
pub fn dijkstra(
    graph: WeightedCsrGraph,
    src: usize,
    dist_out: []f64,
    parent_out: []i64,
    ws: *DijkstraWorkspace,
): DijkstraResult? {
    if (!graph.validate() || graph.num_nodes == 0) {
        return null;
    }
    const n = graph.num_nodes;
    if (src >= n || dist_out.len < n || parent_out.len < n) {
        return null;
    }
    if (ws.heap_nodes.len < n || ws.heap_pos.len < n || ws.visited.len < n) {
        return null;
    }

    // Reject negative edge weights up front
    const total_edges = graph.row_ptr[n];
    let e_chk: usize = 0;
    while (e_chk < total_edges) {
        if (graph.weights[e_chk] < 0.0) {
            return null;
        }
        e_chk += 1;
    }

    const inf = infF64();
    let i: usize = 0;
    while (i < n) {
        dist_out[i] = inf;
        parent_out[i] = NO_PARENT;
        ws.heap_pos[i] = -1;
        ws.visited[i] = 0;
        i += 1;
    }

    dist_out[src] = 0.0;
    ws.heap_nodes[0] = src;
    ws.heap_pos[src] = 0;
    let heap_size: usize = 1;
    let settled: usize = 0;
    let relaxations: usize = 0;

    while (heap_size > 0) {
        const u = ws.heap_nodes[0];
        heap_size -= 1;
        ws.heap_pos[u] = -1;
        if (heap_size > 0) {
            const last = ws.heap_nodes[heap_size];
            ws.heap_nodes[0] = last;
            ws.heap_pos[last] = 0;
            heapSiftDown(ws.heap_nodes, ws.heap_pos, dist_out, heap_size, 0);
        }

        if (ws.visited[u] != 0) {
            continue;
        }
        ws.visited[u] = 1;
        settled += 1;

        const du = dist_out[u];
        const start = graph.row_ptr[u];
        const end = graph.row_ptr[u + 1];
        let e = start;
        while (e < end) {
            const v = graph.col_idx[e];
            if (ws.visited[v] == 0) {
                const cand = du + graph.weights[e];
                if (cand < dist_out[v]) {
                    dist_out[v] = cand;
                    parent_out[v] = u as i64;
                    relaxations += 1;
                    const pos = ws.heap_pos[v];
                    if (pos < 0) {
                        ws.heap_nodes[heap_size] = v;
                        ws.heap_pos[v] = heap_size as i64;
                        heapSiftUp(ws.heap_nodes, ws.heap_pos, dist_out, heap_size);
                        heap_size += 1;
                    } else {
                        heapSiftUp(ws.heap_nodes, ws.heap_pos, dist_out, pos as usize);
                    }
                }
            }
            e += 1;
        }
    }

    return DijkstraResult {
        settled_nodes: settled,
        relaxations: relaxations,
    };
}

/// Reconstructs the forward path `src -> ... -> dst` from a predecessor array `parent`
/// and writes vertex indices into `path_out`.
///
/// Returns the number of vertices in the reconstructed path, or `null` if `src`/`dst`
/// is out of bounds, `dst` is unreachable from `src`, a predecessor cycle is detected,
/// or `path_out` is too small.
pub fn reconstructPath(
    parent: []const i64,
    src: usize,
    dst: usize,
    path_out: []usize,
): usize? {
    const n = parent.len;
    if (n == 0 || src >= n || dst >= n) {
        return null;
    }
    if (src == dst) {
        if (path_out.len < 1) {
            return null;
        }
        path_out[0] = src;
        return 1;
    }

    let len: usize = 0;
    let cur = dst;
    let steps: usize = 0;
    while (true) {
        if (len >= path_out.len) {
            return null;
        }
        path_out[len] = cur;
        len += 1;
        if (cur == src) {
            break;
        }
        steps += 1;
        if (steps >= n) {
            return null;
        }
        const p = parent[cur];
        if (p < 0 || (p as usize) >= n) {
            return null;
        }
        cur = p as usize;
    }

    // Reverse in-place to produce src -> ... -> dst order
    let lo: usize = 0;
    let hi: usize = len - 1;
    while (lo < hi) {
        const tmp = path_out[lo];
        path_out[lo] = path_out[hi];
        path_out[hi] = tmp;
        lo += 1;
        hi -= 1;
    }

    return len;
}

/// Single-source shortest paths supporting negative edge weights via Bellman-Ford,
/// with reachable negative-weight cycle detection.
///
/// Returns `null` if the graph is invalid/empty, `src >= n`, or output buffers have length `< n`.
pub fn bellmanFord(
    graph: WeightedCsrGraph,
    src: usize,
    dist_out: []f64,
    parent_out: []i64,
): BellmanFordResult? {
    if (!graph.validate() || graph.num_nodes == 0) {
        return null;
    }
    const n = graph.num_nodes;
    if (src >= n || dist_out.len < n || parent_out.len < n) {
        return null;
    }

    const inf = infF64();
    let i: usize = 0;
    while (i < n) {
        dist_out[i] = inf;
        parent_out[i] = NO_PARENT;
        i += 1;
    }
    dist_out[src] = 0.0;

    let iterations: usize = 0;
    let early_stop = false;
    let round: usize = 0;
    while (round + 1 < n) {
        iterations += 1;
        let updated = false;
        let u: usize = 0;
        while (u < n) {
            const du = dist_out[u];
            if (isFiniteF64(du)) {
                const start = graph.row_ptr[u];
                const end = graph.row_ptr[u + 1];
                let e = start;
                while (e < end) {
                    const v = graph.col_idx[e];
                    const cand = du + graph.weights[e];
                    if (cand < dist_out[v]) {
                        dist_out[v] = cand;
                        parent_out[v] = u as i64;
                        updated = true;
                    }
                    e += 1;
                }
            }
            u += 1;
        }
        if (!updated) {
            early_stop = true;
            break;
        }
        round += 1;
    }

    let has_neg_cycle = false;
    if (!early_stop) {
        let u: usize = 0;
        while (u < n && !has_neg_cycle) {
            const du = dist_out[u];
            if (isFiniteF64(du)) {
                const start = graph.row_ptr[u];
                const end = graph.row_ptr[u + 1];
                let e = start;
                while (e < end) {
                    const v = graph.col_idx[e];
                    if (du + graph.weights[e] < dist_out[v]) {
                        has_neg_cycle = true;
                        break;
                    }
                    e += 1;
                }
            }
            u += 1;
        }
    }

    return BellmanFordResult {
        has_negative_cycle: has_neg_cycle,
        iterations: iterations,
    };
}

/// All-pairs shortest paths via Floyd-Warshall on an `n x n` row-major distance matrix
/// with next-hop matrix `next_mat` for path reconstruction and negative-cycle detection.
///
/// Requires `num_nodes > 0`, `dist_mat.len >= num_nodes * num_nodes`, `next_mat.len >= num_nodes * num_nodes`,
/// and all edges in `edges` to have valid endpoints `< num_nodes` and finite weights.
pub fn floydWarshall(
    num_nodes: usize,
    edges: []const Edge,
    dist_mat: []f64,
    next_mat: []i64,
): FloydWarshallResult? {
    if (num_nodes == 0) {
        return null;
    }
    if ((dist_mat.len / num_nodes) < num_nodes || (next_mat.len / num_nodes) < num_nodes) {
        return null;
    }

    // Validate all input edges before mutating matrices
    let e_chk: usize = 0;
    while (e_chk < edges.len) {
        const edge = edges[e_chk];
        if (edge.from >= num_nodes || edge.to >= num_nodes || !isFiniteF64(edge.weight)) {
            return null;
        }
        e_chk += 1;
    }

    const n = num_nodes;
    const inf = infF64();
    let r: usize = 0;
    while (r < n) {
        let c: usize = 0;
        while (c < n) {
            const idx = r * n + c;
            if (r == c) {
                dist_mat[idx] = 0.0;
                next_mat[idx] = r as i64;
            } else {
                dist_mat[idx] = inf;
                next_mat[idx] = NO_PARENT;
            }
            c += 1;
        }
        r += 1;
    }

    let ei: usize = 0;
    while (ei < edges.len) {
        const edge = edges[ei];
        const idx = edge.from * n + edge.to;
        if (edge.weight < dist_mat[idx]) {
            dist_mat[idx] = edge.weight;
            next_mat[idx] = edge.to as i64;
        }
        ei += 1;
    }

    let k: usize = 0;
    while (k < n) {
        let i: usize = 0;
        while (i < n) {
            const dik = dist_mat[i * n + k];
            if (isFiniteF64(dik)) {
                let j: usize = 0;
                while (j < n) {
                    const dkj = dist_mat[k * n + j];
                    if (isFiniteF64(dkj)) {
                        const cand = dik + dkj;
                        const ij = i * n + j;
                        if (cand < dist_mat[ij]) {
                            dist_mat[ij] = cand;
                            next_mat[ij] = next_mat[i * n + k];
                        }
                    }
                    j += 1;
                }
            }
            i += 1;
        }
        k += 1;
    }

    let has_neg_cycle = false;
    let d: usize = 0;
    while (d < n) {
        if (dist_mat[d * n + d] < 0.0) {
            has_neg_cycle = true;
            break;
        }
        d += 1;
    }

    return FloydWarshallResult {
        has_negative_cycle: has_neg_cycle,
    };
}

/// Reconstructs the shortest path `src -> ... -> dst` from a Floyd-Warshall `next_mat` matrix.
///
/// Returns the number of vertices written to `path_out`, or `null` if unreachable, invalid, or buffer too small.
pub fn reconstructFloydPath(
    next_mat: []const i64,
    num_nodes: usize,
    src: usize,
    dst: usize,
    path_out: []usize,
): usize? {
    if (num_nodes == 0 || (next_mat.len / num_nodes) < num_nodes || src >= num_nodes || dst >= num_nodes) {
        return null;
    }
    if (next_mat[src * num_nodes + dst] < 0) {
        return null;
    }
    if (path_out.len < 1) {
        return null;
    }
    let len: usize = 0;
    let cur = src;
    path_out[len] = cur;
    len += 1;
    while (cur != dst) {
        if (len >= num_nodes || len >= path_out.len) {
            return null;
        }
        const nxt = next_mat[cur * num_nodes + dst];
        if (nxt < 0 || (nxt as usize) >= num_nodes) {
            return null;
        }
        cur = nxt as usize;
        path_out[len] = cur;
        len += 1;
    }
    return len;
}
