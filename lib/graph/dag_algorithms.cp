// Source: cp/lib/graph/dag_algorithms.cp (DAG Topological Sort, Cycle Detection, Tarjan SCC, Shortest Paths)
// Pure CP algorithms for DirectedGraph: Kahn topological sort, 3-color cycle detection,
// reachability & transitive closure, iterative Tarjan SCC, Dijkstra, Bellman-Ford, and path reconstruction.

const std = @import("std");
import { GraphError } from "./graph.cp";
import { WeightedEdge, DirectedGraph } from "./directed_graph.cp";

pub const NO_PREV: usize = std.math.maxInt(usize);

/// Reachability matrix returned by `transitiveClosure`.
pub struct ReachabilityMatrix {
    alloc: std.mem.Allocator = undefined,
    size: usize = 0,
    matrix: [][]bool = [],

    /// Releases all matrix rows and the outer slice.
    pub fn deinit(*_) {
        for (row of _.matrix) {
            if (row.len > 0) {
                _.alloc.free(row);
            }
        }
        if (_.matrix.len > 0) {
            _.alloc.free(_.matrix);
        }
        _.matrix = [];
        _.size = 0;
    }

    /// Returns `true` if `v` is reachable from `u`.
    pub fn canReach(u: usize, v: usize): bool {
        if (u >= _.size || v >= _.size) return false;
        return _.matrix[u][v];
    }
}

/// Owned Tarjan Strongly Connected Components result returned by `tarjanScc`.
/// Components are ordered in reverse topological order of the condensation DAG.
pub struct TarjanSccResult {
    alloc: std.mem.Allocator = undefined,
    components: [][]usize = [],

    /// Releases each component slice and the outer array.
    pub fn deinit(*_) {
        for (comp of _.components) {
            if (comp.len > 0) {
                _.alloc.free(comp);
            }
        }
        if (_.components.len > 0) {
            _.alloc.free(_.components);
        }
        _.components = [];
    }

    /// Returns the number of strongly connected components.
    pub fn count(): usize {
        return _.components.len;
    }
}

/// Single-source shortest path distances and predecessor array returned by
/// `dijkstra` and `bellmanFord`.
pub struct PathResult {
    alloc: std.mem.Allocator = undefined,
    source: usize = 0,
    dist: []f64 = [],
    prev: []usize = [],

    /// Releases distance and predecessor slices.
    pub fn deinit(*_) {
        if (_.dist.len > 0) {
            _.alloc.free(_.dist);
        }
        if (_.prev.len > 0) {
            _.alloc.free(_.prev);
        }
        _.dist = [];
        _.prev = [];
    }

    /// Returns the shortest distance to `target`, or `GraphError.Disconnected` if unreachable.
    pub fn distanceTo(target: usize): f64 ! GraphError {
        if (target >= _.dist.len) return GraphError.InvalidVertex;
        if (std.math.isInf(_.dist[target])) return GraphError.Disconnected;
        return _.dist[target];
    }

    /// Returns `true` if `target` is reachable from `source`.
    pub fn isReachable(target: usize): bool {
        if (target >= _.dist.len) return false;
        return !std.math.isInf(_.dist[target]);
    }
}

struct DfsFrame {
    u: usize = 0,
    edge_idx: usize = 0,
}

struct MinHeapItem {
    dist: f64 = 0.0,
    node: usize = 0,
}

fn minHeapLess(a: MinHeapItem, b: MinHeapItem): bool {
    if (a.dist != b.dist) return a.dist < b.dist;
    return a.node < b.node;
}

fn minHeapPush(alloc: std.mem.Allocator, heap: *std.ArrayList(MinHeapItem), item: MinHeapItem): void ! GraphError {
    heap.append(alloc, item) catch return GraphError.OutOfMemory;
    let idx = heap.items.len - 1;
    while (idx > 0) {
        const parent = (idx - 1) / 2;
        if (!minHeapLess(heap.items[idx], heap.items[parent])) break;
        const tmp = heap.items[idx];
        heap.items[idx] = heap.items[parent];
        heap.items[parent] = tmp;
        idx = parent;
    }
}

fn minHeapPop(heap: *std.ArrayList(MinHeapItem)): MinHeapItem {
    const top = heap.items[0];
    const last = heap.pop().?;
    if (heap.items.len > 0) {
        heap.items[0] = last;
        let idx: usize = 0;
        const len = heap.items.len;
        while (true) {
            const left = idx * 2 + 1;
            const right = idx * 2 + 2;
            let smallest = idx;
            if (left < len && minHeapLess(heap.items[left], heap.items[smallest])) {
                smallest = left;
            }
            if (right < len && minHeapLess(heap.items[right], heap.items[smallest])) {
                smallest = right;
            }
            if (smallest == idx) break;
            const tmp = heap.items[idx];
            heap.items[idx] = heap.items[smallest];
            heap.items[smallest] = tmp;
            idx = smallest;
        }
    }
    return top;
}

/// Computes a topological ordering of `graph` using Kahn's algorithm.
/// Returns `GraphError.CycleDetected` if the graph contains any cycle.
/// Caller owns the returned slice and must free it with `alloc.free(...)`.
pub fn topologicalSort(alloc: std.mem.Allocator, graph: DirectedGraph): ([]usize) ! GraphError {
    const n = graph.vertexCount();
    if (n == 0) {
        return alloc.alloc(usize, 0) catch return GraphError.OutOfMemory;
    }

    let in_deg = alloc.alloc(usize, n) catch return GraphError.OutOfMemory;
    defer alloc.free(in_deg);

    let i: usize = 0;
    while (i < n) : (i += 1) {
        in_deg[i] = graph.inDegree(i)?;
    }

    let queue: std.ArrayList(usize) = .empty;
    defer queue.deinit(alloc);

    let v_idx: usize = 0;
    while (v_idx < n) : (v_idx += 1) {
        if (in_deg[v_idx] == 0) {
            queue.append(alloc, v_idx) catch return GraphError.OutOfMemory;
        }
    }

    let order: std.ArrayList(usize) = .empty;
    errdefer order.deinit(alloc);

    let head: usize = 0;
    while (head < queue.items.len) {
        const u = queue.items[head];
        head += 1;
        order.append(alloc, u) catch return GraphError.OutOfMemory;

        for (e of graph.neighborsUnchecked(u)) {
            in_deg[e.to] -= 1;
            if (in_deg[e.to] == 0) {
                queue.append(alloc, e.to) catch return GraphError.OutOfMemory;
            }
        }
    }

    if (order.items.len != n) {
        return GraphError.CycleDetected;
    }

    return order.toOwnedSlice(alloc) catch return GraphError.OutOfMemory;
}

/// Detects a directed cycle using iterative 3-color DFS.
/// Returns the closed cycle vertex sequence `[v0, v1, ..., v0]`, or `null` if acyclic.
/// Caller owns the returned slice when non-null and must free it with `alloc.free(...)`.
pub fn findCycle(alloc: std.mem.Allocator, graph: DirectedGraph): ([]usize)? ! GraphError {
    const n = graph.vertexCount();
    if (n == 0) return null;

    // 0 = unvisited, 1 = visiting (on path stack), 2 = finished
    let state = alloc.alloc(u8, n) catch return GraphError.OutOfMemory;
    defer alloc.free(state);
    let i: usize = 0;
    while (i < n) : (i += 1) {
        state[i] = 0;
    }

    let call_stack: std.ArrayList(DfsFrame) = .empty;
    defer call_stack.deinit(alloc);
    let path_stack: std.ArrayList(usize) = .empty;
    defer path_stack.deinit(alloc);

    let root: usize = 0;
    while (root < n) : (root += 1) {
        if (state[root] != 0) continue;

        state[root] = 1;
        path_stack.append(alloc, root) catch return GraphError.OutOfMemory;
        call_stack.append(alloc, DfsFrame{ u: root, edge_idx: 0 }) catch return GraphError.OutOfMemory;

        while (call_stack.items.len > 0) {
            const top = call_stack.items.len - 1;
            const u = call_stack.items[top].u;
            const ei = call_stack.items[top].edge_idx;
            const edges = graph.neighborsUnchecked(u);

            if (ei < edges.len) {
                call_stack.items[top].edge_idx = ei + 1;
                const v = edges[ei].to;
                if (state[v] == 1) {
                    let start_pos = path_stack.items.len;
                    let k = path_stack.items.len;
                    while (k > 0) {
                        k -= 1;
                        if (path_stack.items[k] == v) {
                            start_pos = k;
                            break;
                        }
                    }
                    const cycle_len = path_stack.items.len - start_pos + 1;
                    let cycle = alloc.alloc(usize, cycle_len) catch return GraphError.OutOfMemory;
                    let c_idx: usize = 0;
                    let p_idx = start_pos;
                    while (p_idx < path_stack.items.len) : (p_idx += 1) {
                        cycle[c_idx] = path_stack.items[p_idx];
                        c_idx += 1;
                    }
                    cycle[c_idx] = v;
                    return cycle;
                } else if (state[v] == 0) {
                    state[v] = 1;
                    path_stack.append(alloc, v) catch return GraphError.OutOfMemory;
                    call_stack.append(alloc, DfsFrame{ u: v, edge_idx: 0 }) catch return GraphError.OutOfMemory;
                }
            } else {
                * = call_stack.pop();
                * = path_stack.pop();
                state[u] = 2;
            }
        }
    }

    return null;
}

/// Returns all vertices reachable from `start` (including `start`) in BFS order.
/// Caller owns the returned slice and must free it with `alloc.free(...)`.
pub fn reachableFrom(alloc: std.mem.Allocator, graph: DirectedGraph, start: usize): ([]usize) ! GraphError {
    const n = graph.vertexCount();
    if (start >= n) {
        return GraphError.InvalidVertex;
    }

    let visited = alloc.alloc(bool, n) catch return GraphError.OutOfMemory;
    defer alloc.free(visited);
    let i: usize = 0;
    while (i < n) : (i += 1) {
        visited[i] = false;
    }

    let queue: std.ArrayList(usize) = .empty;
    errdefer queue.deinit(alloc);

    visited[start] = true;
    queue.append(alloc, start) catch return GraphError.OutOfMemory;

    let head: usize = 0;
    while (head < queue.items.len) {
        const u = queue.items[head];
        head += 1;
        for (e of graph.neighborsUnchecked(u)) {
            if (!visited[e.to]) {
                visited[e.to] = true;
                queue.append(alloc, e.to) catch return GraphError.OutOfMemory;
            }
        }
    }

    return queue.toOwnedSlice(alloc) catch return GraphError.OutOfMemory;
}

/// Computes the all-pairs reachability matrix (transitive closure) of `graph`.
pub fn transitiveClosure(alloc: std.mem.Allocator, graph: DirectedGraph): ReachabilityMatrix ! GraphError {
    const n = graph.vertexCount();
    if (n == 0) {
        return ReachabilityMatrix{ alloc: alloc, size: 0, matrix: [] };
    }

    let matrix = alloc.alloc([]bool, n) catch return GraphError.OutOfMemory;
    let filled: usize = 0;
    errdefer {
        let fi: usize = 0;
        while (fi < filled) : (fi += 1) {
            alloc.free(matrix[fi]);
        }
        alloc.free(matrix);
    }

    let queue: std.ArrayList(usize) = .empty;
    defer queue.deinit(alloc);

    let src: usize = 0;
    while (src < n) : (src += 1) {
        let row = alloc.alloc(bool, n) catch return GraphError.OutOfMemory;
        let col: usize = 0;
        while (col < n) : (col += 1) {
            row[col] = false;
        }
        matrix[src] = row;
        filled += 1;

        queue.clearRetainingCapacity();
        row[src] = true;
        queue.append(alloc, src) catch return GraphError.OutOfMemory;

        let head: usize = 0;
        while (head < queue.items.len) {
            const u = queue.items[head];
            head += 1;
            for (e of graph.neighborsUnchecked(u)) {
                if (!row[e.to]) {
                    row[e.to] = true;
                    queue.append(alloc, e.to) catch return GraphError.OutOfMemory;
                }
            }
        }
    }

    return ReachabilityMatrix{
        alloc: alloc,
        size: n,
        matrix: matrix,
    };
}

/// Decomposes `graph` into Strongly Connected Components using iterative Tarjan's algorithm
/// (immune to deep call-stack overflow). Returns components in reverse topological order.
pub fn tarjanScc(alloc: std.mem.Allocator, graph: DirectedGraph): TarjanSccResult ! GraphError {
    const n = graph.vertexCount();
    if (n == 0) {
        return TarjanSccResult{ alloc: alloc, components: [] };
    }

    let indices = alloc.alloc(usize, n) catch return GraphError.OutOfMemory;
    defer alloc.free(indices);
    let lowlink = alloc.alloc(usize, n) catch return GraphError.OutOfMemory;
    defer alloc.free(lowlink);
    let on_stack = alloc.alloc(bool, n) catch return GraphError.OutOfMemory;
    defer alloc.free(on_stack);

    let i: usize = 0;
    while (i < n) : (i += 1) {
        indices[i] = NO_PREV;
        lowlink[i] = 0;
        on_stack[i] = false;
    }

    let tarjan_stack: std.ArrayList(usize) = .empty;
    defer tarjan_stack.deinit(alloc);
    let call_stack: std.ArrayList(DfsFrame) = .empty;
    defer call_stack.deinit(alloc);

    let comps: std.ArrayList([]usize) = .empty;
    errdefer {
        for (c of comps.items) {
            if (c.len > 0) alloc.free(c);
        }
        comps.deinit(alloc);
    }

    let index_counter: usize = 0;
    let root: usize = 0;
    while (root < n) : (root += 1) {
        if (indices[root] != NO_PREV) continue;

        indices[root] = index_counter;
        lowlink[root] = index_counter;
        index_counter += 1;
        on_stack[root] = true;
        tarjan_stack.append(alloc, root) catch return GraphError.OutOfMemory;
        call_stack.append(alloc, DfsFrame{ u: root, edge_idx: 0 }) catch return GraphError.OutOfMemory;

        while (call_stack.items.len > 0) {
            const top = call_stack.items.len - 1;
            const u = call_stack.items[top].u;
            const ei = call_stack.items[top].edge_idx;
            const edges = graph.neighborsUnchecked(u);

            if (ei < edges.len) {
                call_stack.items[top].edge_idx = ei + 1;
                const v = edges[ei].to;
                if (indices[v] == NO_PREV) {
                    indices[v] = index_counter;
                    lowlink[v] = index_counter;
                    index_counter += 1;
                    on_stack[v] = true;
                    tarjan_stack.append(alloc, v) catch return GraphError.OutOfMemory;
                    call_stack.append(alloc, DfsFrame{ u: v, edge_idx: 0 }) catch return GraphError.OutOfMemory;
                } else if (on_stack[v]) {
                    if (indices[v] < lowlink[u]) {
                        lowlink[u] = indices[v];
                    }
                }
            } else {
                * = call_stack.pop();
                if (call_stack.items.len > 0) {
                    const p = call_stack.items[call_stack.items.len - 1].u;
                    if (lowlink[u] < lowlink[p]) {
                        lowlink[p] = lowlink[u];
                    }
                }

                if (lowlink[u] == indices[u]) {
                    let comp: std.ArrayList(usize) = .empty;
                    errdefer comp.deinit(alloc);

                    while (true) {
                        const w = tarjan_stack.pop().?;
                        on_stack[w] = false;
                        comp.append(alloc, w) catch return GraphError.OutOfMemory;
                        if (w == u) break;
                    }

                    const owned_comp = comp.toOwnedSlice(alloc) catch return GraphError.OutOfMemory;
                    comps.append(alloc, owned_comp) catch {
                        alloc.free(owned_comp);
                        return GraphError.OutOfMemory;
                    };
                }
            }
        }
    }

    const owned_comps = comps.toOwnedSlice(alloc) catch return GraphError.OutOfMemory;
    return TarjanSccResult{
        alloc: alloc,
        components: owned_comps,
    };
}

/// Computes single-source shortest paths from `source` using Dijkstra's algorithm.
/// Rejects negative or NaN edge weights with `GraphError.NegativeWeight`.
pub fn dijkstra(alloc: std.mem.Allocator, graph: DirectedGraph, source: usize): PathResult ! GraphError {
    const n = graph.vertexCount();
    if (source >= n) {
        return GraphError.InvalidVertex;
    }

    let u_scan: usize = 0;
    while (u_scan < n) : (u_scan += 1) {
        for (e of graph.neighborsUnchecked(u_scan)) {
            if (e.weight < 0.0 || std.math.isNan(e.weight)) {
                return GraphError.NegativeWeight;
            }
        }
    }

    const pos_inf = std.math.inf(f64);
    let dist = alloc.alloc(f64, n) catch return GraphError.OutOfMemory;
    errdefer alloc.free(dist);
    let prev = alloc.alloc(usize, n) catch return GraphError.OutOfMemory;
    errdefer alloc.free(prev);

    let i: usize = 0;
    while (i < n) : (i += 1) {
        dist[i] = pos_inf;
        prev[i] = NO_PREV;
    }
    dist[source] = 0.0;

    let pq: std.ArrayList(MinHeapItem) = .empty;
    defer pq.deinit(alloc);
    minHeapPush(alloc, &pq, MinHeapItem{ dist: 0.0, node: source })?;

    while (pq.items.len > 0) {
        const top = minHeapPop(&pq);
        if (top.dist > dist[top.node]) continue;

        for (e of graph.neighborsUnchecked(top.node)) {
            const nd = top.dist + e.weight;
            if (nd < dist[e.to]) {
                dist[e.to] = nd;
                prev[e.to] = top.node;
                minHeapPush(alloc, &pq, MinHeapItem{ dist: nd, node: e.to })?;
            }
        }
    }

    return PathResult{
        alloc: alloc,
        source: source,
        dist: dist,
        prev: prev,
    };
}

/// Computes single-source shortest paths from `source` using Bellman-Ford,
/// supporting negative edge weights and detecting negative-weight cycles
/// (`GraphError.CycleDetected`).
pub fn bellmanFord(alloc: std.mem.Allocator, graph: DirectedGraph, source: usize): PathResult ! GraphError {
    const n = graph.vertexCount();
    if (source >= n) {
        return GraphError.InvalidVertex;
    }

    const pos_inf = std.math.inf(f64);
    let dist = alloc.alloc(f64, n) catch return GraphError.OutOfMemory;
    errdefer alloc.free(dist);
    let prev = alloc.alloc(usize, n) catch return GraphError.OutOfMemory;
    errdefer alloc.free(prev);

    let i: usize = 0;
    while (i < n) : (i += 1) {
        dist[i] = pos_inf;
        prev[i] = NO_PREV;
    }
    dist[source] = 0.0;

    if (n > 1) {
        let iter: usize = 0;
        while (iter < n - 1) : (iter += 1) {
            let changed = false;
            let u: usize = 0;
            while (u < n) : (u += 1) {
                if (std.math.isInf(dist[u])) continue;
                for (e of graph.neighborsUnchecked(u)) {
                    if (std.math.isNan(e.weight)) return GraphError.NegativeWeight;
                    const nd = dist[u] + e.weight;
                    if (nd < dist[e.to]) {
                        dist[e.to] = nd;
                        prev[e.to] = u;
                        changed = true;
                    }
                }
            }
            if (!changed) break;
        }
    }

    let u_chk: usize = 0;
    while (u_chk < n) : (u_chk += 1) {
        if (std.math.isInf(dist[u_chk])) continue;
        for (e of graph.neighborsUnchecked(u_chk)) {
            if (std.math.isNan(e.weight)) return GraphError.NegativeWeight;
            if (dist[u_chk] + e.weight < dist[e.to]) {
                return GraphError.CycleDetected;
            }
        }
    }

    return PathResult{
        alloc: alloc,
        source: source,
        dist: dist,
        prev: prev,
    };
}

/// Reconstructs the shortest path from `source` to `target` using a predecessor
/// slice `prev` produced by `dijkstra` or `bellmanFord`.
/// Returns `GraphError.Disconnected` if `target` is unreachable from `source`.
/// Caller owns the returned slice and must free it with `alloc.free(...)`.
pub fn reconstructPath(
    alloc: std.mem.Allocator,
    prev: []const usize,
    source: usize,
    target: usize,
): ([]usize) ! GraphError {
    const n = prev.len;
    if (source >= n || target >= n) {
        return GraphError.InvalidVertex;
    }
    if (source == target) {
        let single = alloc.alloc(usize, 1) catch return GraphError.OutOfMemory;
        single[0] = source;
        return single;
    }
    if (prev[target] == NO_PREV) {
        return GraphError.Disconnected;
    }

    let rev: std.ArrayList(usize) = .empty;
    defer rev.deinit(alloc);

    let cur = target;
    let steps: usize = 0;
    while (true) {
        rev.append(alloc, cur) catch return GraphError.OutOfMemory;
        if (cur == source) break;
        cur = prev[cur];
        if (cur == NO_PREV || steps >= n) {
            return GraphError.Disconnected;
        }
        steps += 1;
    }

    const len = rev.items.len;
    let path = alloc.alloc(usize, len) catch return GraphError.OutOfMemory;
    let k: usize = 0;
    while (k < len) : (k += 1) {
        path[k] = rev.items[len - 1 - k];
    }
    return path;
}
