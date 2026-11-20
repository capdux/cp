// Source: cp/lib/graph/algorithms.cp (Dijkstra, Tarjan SCC, Topological Sort, Kruskal MST & PageRank)
// Production-grade graph algorithms for shortest-path routing, DAG dependency ordering,
// strongly connected components, minimum spanning trees, and GraphRAG PageRank centrality.

const std = @import("std");
import { GraphError, GraphEdge, Graph } from "./graph.cp";

const UNVISITED: usize = std.math.maxInt(usize);

/// Owned shortest-path computation result returned by `GraphAlgo.dijkstra`.
pub struct ShortestPathResult {
    alloc: std.mem.Allocator = undefined,
    found: bool = false,
    total_cost: f64 = 0.0,
    path: []string = [],

    /// Releases the owned `path` strings and backing slice.
    pub fn deinit(*_) {
        for (seg of _.path) {
            _.alloc.free(seg);
        }
        if (_.path.len > 0) {
            _.alloc.free(_.path);
        }
        _.path = [];
        _.found = false;
        _.total_cost = 0.0;
    }
}

/// Owned Tarjan Strongly Connected Components result returned by `GraphAlgo.tarjanScc`.
pub struct SccResult {
    alloc: std.mem.Allocator = undefined,
    components: [][]string = [],

    /// Releases each component slice and the outer `components` array.
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

    /// Returns the number of strongly connected components discovered.
    pub fn count(): usize {
        return _.components.len;
    }
}

/// Single undirected edge included in a Minimum Spanning Tree.
pub struct MstEdge {
    from: string = "",
    to: string = "",
    weight: f64 = 0.0,
    label: string = "",
}

/// Owned Kruskal Minimum Spanning Tree result returned by `GraphAlgo.kruskalMst`.
pub struct MstResult {
    alloc: std.mem.Allocator = undefined,
    total_weight: f64 = 0.0,
    edges: []MstEdge = [],

    /// Releases the MST edge slice.
    pub fn deinit(*_) {
        if (_.edges.len > 0) {
            _.alloc.free(_.edges);
        }
        _.edges = [];
        _.total_weight = 0.0;
    }
}

/// Node centrality score entry returned by `GraphAlgo.pageRank` in descending score order.
pub struct PageRankEntry {
    id: string = "",
    score: f64 = 0.0,
}

struct HeapItem {
    dist: f64 = 0.0,
    node: usize = 0,
}

fn heapLess(a: HeapItem, b: HeapItem): bool {
    if (a.dist != b.dist) return a.dist < b.dist;
    return a.node < b.node;
}

fn heapPush(alloc: std.mem.Allocator, heap: *std.ArrayList(HeapItem), item: HeapItem): void ! GraphError {
    heap.append(alloc, item) catch return GraphError.OutOfMemory;
    let idx = heap.items.len - 1;
    while (idx > 0) {
        const parent = (idx - 1) / 2;
        if (!heapLess(heap.items[idx], heap.items[parent])) break;
        const tmp = heap.items[idx];
        heap.items[idx] = heap.items[parent];
        heap.items[parent] = tmp;
        idx = parent;
    }
}

fn heapPop(heap: *std.ArrayList(HeapItem)): HeapItem {
    const top = heap.items[0];
    const last = heap.pop().?;
    if (heap.items.len > 0) {
        heap.items[0] = last;
        let idx: usize = 0;
        const n = heap.items.len;
        while (true) {
            const left = idx * 2 + 1;
            if (left >= n) break;
            const right = left + 1;
            let best = left;
            if (right < n && heapLess(heap.items[right], heap.items[left])) {
                best = right;
            }
            if (!heapLess(heap.items[best], heap.items[idx])) break;
            const tmp = heap.items[idx];
            heap.items[idx] = heap.items[best];
            heap.items[best] = tmp;
            idx = best;
        }
    }
    return top;
}

struct TarjanFrame {
    u: usize = 0,
    edge_idx: usize = 0,
}

struct CandidateEdge {
    u: usize = 0,
    v: usize = 0,
    weight: f64 = 0.0,
    label: string = "",
}

fn candidateEdgeLess(a: CandidateEdge, b: CandidateEdge): bool {
    if (a.weight != b.weight) return a.weight < b.weight;
    if (a.u != b.u) return a.u < b.u;
    return a.v < b.v;
}

fn siftDownMaxEdge(items: []CandidateEdge, start: usize, len: usize) {
    let idx = start;
    while (true) {
        const left = idx * 2 + 1;
        if (left >= len) break;
        const right = left + 1;
        let largest = left;
        if (right < len && candidateEdgeLess(items[largest], items[right])) {
            largest = right;
        }
        if (!candidateEdgeLess(items[idx], items[largest])) break;
        const tmp = items[idx];
        items[idx] = items[largest];
        items[largest] = tmp;
        idx = largest;
    }
}

fn sortCandidateEdges(items: []CandidateEdge) {
    if (items.len <= 1) return;
    const n = items.len;
    let i: usize = n / 2;
    while (i > 0) {
        i -= 1;
        siftDownMaxEdge(items, i, n);
    }
    let end: usize = n;
    while (end > 1) {
        end -= 1;
        const tmp = items[0];
        items[0] = items[end];
        items[end] = tmp;
        siftDownMaxEdge(items, 0, end);
    }
}

fn dsuFind(parent: []usize, x_in: usize): usize {
    let x = x_in;
    while (parent[x] != x) {
        parent[x] = parent[parent[x]];
        x = parent[x];
    }
    let cur = x_in;
    while (parent[cur] != x) {
        const next_p = parent[cur];
        parent[cur] = x;
        cur = next_p;
    }
    return x;
}

fn dsuUnite(parent: []usize, rank: []usize, a: usize, b: usize): bool {
    const ra = dsuFind(parent, a);
    const rb = dsuFind(parent, b);
    if (ra == rb) return false;
    if (rank[ra] < rank[rb]) {
        parent[ra] = rb;
    } else if (rank[ra] > rank[rb]) {
        parent[rb] = ra;
    } else {
        parent[rb] = ra;
        rank[ra] += 1;
    }
    return true;
}

fn pageRankBetterDesc(a: PageRankEntry, b: PageRankEntry): bool {
    if (a.score != b.score) return a.score > b.score;
    return std.mem.order(u8, a.id, b.id) == .lt;
}

fn siftDownWorstPageRank(items: []PageRankEntry, start: usize, len: usize) {
    let idx = start;
    while (true) {
        const left = idx * 2 + 1;
        if (left >= len) break;
        const right = left + 1;
        let worst = left;
        if (right < len && pageRankBetterDesc(items[worst], items[right])) {
            worst = right;
        }
        if (!pageRankBetterDesc(items[idx], items[worst])) break;
        const tmp = items[idx];
        items[idx] = items[worst];
        items[worst] = tmp;
        idx = worst;
    }
}

fn sortPageRankDesc(items: []PageRankEntry) {
    if (items.len <= 1) return;
    const n = items.len;
    let i: usize = n / 2;
    while (i > 0) {
        i -= 1;
        siftDownWorstPageRank(items, i, n);
    }
    let end: usize = n;
    while (end > 1) {
        end -= 1;
        const tmp = items[0];
        items[0] = items[end];
        items[end] = tmp;
        siftDownWorstPageRank(items, 0, end);
    }
}

/// Static namespace providing core graph algorithms over `Graph`.
pub static struct GraphAlgo {
    /// Computes the shortest path from `start` to `goal` using Dijkstra's algorithm
    /// with a binary min-heap. Rejects any negative or NaN edge weight with
    /// `GraphError.NegativeWeight`, and missing endpoints with `GraphError.NodeNotFound`.
    static fn dijkstra(
        alloc: std.mem.Allocator,
        g: Graph,
        start: string,
        goal: string,
    ): ShortestPathResult ! GraphError {
        const opt_start = g.findNode(start);
        if (opt_start == null) return GraphError.NodeNotFound;
        const start_idx = opt_start.?;
        const opt_goal = g.findNode(goal);
        if (opt_goal == null) return GraphError.NodeNotFound;
        const goal_idx = opt_goal.?;

        const n = g.nodeCount();
        let ni: usize = 0;
        while (ni < n) : (ni += 1) {
            for (e of g.neighborsAt(ni)) {
                if (e.weight < 0.0 || std.math.isNan(e.weight)) {
                    return GraphError.NegativeWeight;
                }
            }
        }

        const pos_inf = std.math.inf(f64);
        let dist = alloc.alloc(f64, n) catch return GraphError.OutOfMemory;
        defer alloc.free(dist);
        let prev = alloc.alloc(usize, n) catch return GraphError.OutOfMemory;
        defer alloc.free(prev);

        let di: usize = 0;
        while (di < n) : (di += 1) {
            dist[di] = pos_inf;
            prev[di] = UNVISITED;
        }
        dist[start_idx] = 0.0;

        let pq: std.ArrayList(HeapItem) = .empty;
        defer pq.deinit(alloc);
        heapPush(alloc, &pq, HeapItem{ dist: 0.0, node: start_idx })?;

        while (pq.items.len > 0) {
            const top = heapPop(&pq);
            if (top.dist > dist[top.node]) continue;
            if (top.node == goal_idx) break;

            for (e of g.neighborsAt(top.node)) {
                const nd = top.dist + e.weight;
                if (nd < dist[e.to]) {
                    dist[e.to] = nd;
                    prev[e.to] = top.node;
                    heapPush(alloc, &pq, HeapItem{ dist: nd, node: e.to })?;
                }
            }
        }

        if (std.math.isInf(dist[goal_idx])) {
            return ShortestPathResult{
                alloc: alloc,
                found: false,
                total_cost: 0.0,
                path: [],
            };
        }

        let rev_indices: std.ArrayList(usize) = .empty;
        defer rev_indices.deinit(alloc);

        let cur = goal_idx;
        while (true) {
            rev_indices.append(alloc, cur) catch return GraphError.OutOfMemory;
            if (cur == start_idx) break;
            cur = prev[cur];
            if (cur == UNVISITED) break;
        }

        const path_len = rev_indices.items.len;
        let path_out = alloc.alloc(string, path_len) catch return GraphError.OutOfMemory;
        let filled: usize = 0;
        errdefer {
            let fk: usize = 0;
            while (fk < filled) : (fk += 1) {
                alloc.free(path_out[fk]);
            }
            alloc.free(path_out);
        }

        let k: usize = 0;
        while (k < path_len) : (k += 1) {
            const node_idx = rev_indices.items[path_len - 1 - k];
            const node_str = g.nodeId(node_idx).?;
            const duped = alloc.dupe(u8, node_str) catch return GraphError.OutOfMemory;
            path_out[k] = duped;
            filled += 1;
        }

        return ShortestPathResult{
            alloc: alloc,
            found: true,
            total_cost: dist[goal_idx],
            path: path_out,
        };
    }

    /// Computes a topological ordering of all vertices using Kahn's algorithm.
    /// Returns `GraphError.CycleDetected` if the graph contains any cycle.
    /// Caller must free the returned slice via `alloc.free(order)`.
    static fn topologicalSort(alloc: std.mem.Allocator, g: Graph): ([]string) ! GraphError {
        const n = g.nodeCount();
        if (n == 0) {
            return alloc.alloc(string, 0) catch return GraphError.OutOfMemory;
        }

        let in_degree = alloc.alloc(usize, n) catch return GraphError.OutOfMemory;
        defer alloc.free(in_degree);
        let di: usize = 0;
        while (di < n) : (di += 1) {
            in_degree[di] = 0;
        }

        let u_idx: usize = 0;
        while (u_idx < n) : (u_idx += 1) {
            for (e of g.neighborsAt(u_idx)) {
                in_degree[e.to] += 1;
            }
        }

        let queue: std.ArrayList(usize) = .empty;
        defer queue.deinit(alloc);
        let q_idx: usize = 0;
        while (q_idx < n) : (q_idx += 1) {
            if (in_degree[q_idx] == 0) {
                queue.append(alloc, q_idx) catch return GraphError.OutOfMemory;
            }
        }

        let order: std.ArrayList(string) = .empty;
        errdefer order.deinit(alloc);

        let head: usize = 0;
        while (head < queue.items.len) {
            const u = queue.items[head];
            head += 1;
            order.append(alloc, g.nodeId(u).?) catch return GraphError.OutOfMemory;

            for (e of g.neighborsAt(u)) {
                in_degree[e.to] -= 1;
                if (in_degree[e.to] == 0) {
                    queue.append(alloc, e.to) catch return GraphError.OutOfMemory;
                }
            }
        }

        if (order.items.len != n) {
            return GraphError.CycleDetected;
        }

        return order.toOwnedSlice(alloc) catch return GraphError.OutOfMemory;
    }

    /// Decomposes the graph into Strongly Connected Components in O(V + E) linear time
    /// using an iterative Tarjan's algorithm (immune to call-stack overflow).
    static fn tarjanScc(alloc: std.mem.Allocator, g: Graph): SccResult ! GraphError {
        const n = g.nodeCount();
        if (n == 0) {
            return SccResult{ alloc: alloc, components: [] };
        }

        let indices = alloc.alloc(usize, n) catch return GraphError.OutOfMemory;
        defer alloc.free(indices);
        let lowlink = alloc.alloc(usize, n) catch return GraphError.OutOfMemory;
        defer alloc.free(lowlink);
        let on_stack = alloc.alloc(bool, n) catch return GraphError.OutOfMemory;
        defer alloc.free(on_stack);

        let ti: usize = 0;
        while (ti < n) : (ti += 1) {
            indices[ti] = UNVISITED;
            lowlink[ti] = 0;
            on_stack[ti] = false;
        }

        let tarjan_stack: std.ArrayList(usize) = .empty;
        defer tarjan_stack.deinit(alloc);
        let call_stack: std.ArrayList(TarjanFrame) = .empty;
        defer call_stack.deinit(alloc);

        let comps: std.ArrayList([]string) = .empty;
        errdefer {
            for (c of comps.items) {
                if (c.len > 0) alloc.free(c);
            }
            comps.deinit(alloc);
        }

        let index_counter: usize = 0;

        let root: usize = 0;
        while (root < n) : (root += 1) {
            if (indices[root] != UNVISITED) continue;

            indices[root] = index_counter;
            lowlink[root] = index_counter;
            index_counter += 1;
            on_stack[root] = true;
            tarjan_stack.append(alloc, root) catch return GraphError.OutOfMemory;
            call_stack.append(alloc, TarjanFrame{ u: root, edge_idx: 0 }) catch return GraphError.OutOfMemory;

            while (call_stack.items.len > 0) {
                const top_idx = call_stack.items.len - 1;
                const u = call_stack.items[top_idx].u;
                const ei = call_stack.items[top_idx].edge_idx;
                const edges = g.neighborsAt(u);

                if (ei < edges.len) {
                    call_stack.items[top_idx].edge_idx = ei + 1;
                    const v = edges[ei].to;
                    if (indices[v] == UNVISITED) {
                        indices[v] = index_counter;
                        lowlink[v] = index_counter;
                        index_counter += 1;
                        on_stack[v] = true;
                        tarjan_stack.append(alloc, v) catch return GraphError.OutOfMemory;
                        call_stack.append(alloc, TarjanFrame{ u: v, edge_idx: 0 }) catch return GraphError.OutOfMemory;
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
                        let comp: std.ArrayList(string) = .empty;
                        errdefer comp.deinit(alloc);

                        while (true) {
                            const w = tarjan_stack.pop().?;
                            on_stack[w] = false;
                            comp.append(alloc, g.nodeId(w).?) catch return GraphError.OutOfMemory;
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

        return SccResult{
            alloc: alloc,
            components: comps.toOwnedSlice(alloc) catch return GraphError.OutOfMemory,
        };
    }

    /// Computes a Minimum Spanning Tree (or forest if disconnected) using Kruskal's algorithm
    /// backed by Disjoint-Set Union (DSU) with path compression and union by rank.
    static fn kruskalMst(alloc: std.mem.Allocator, g: Graph): MstResult ! GraphError {
        const n = g.nodeCount();
        if (n <= 1) {
            return MstResult{ alloc: alloc, total_weight: 0.0, edges: [] };
        }

        let candidates: std.ArrayList(CandidateEdge) = .empty;
        defer candidates.deinit(alloc);

        const is_dir = g.isDirected();
        let u: usize = 0;
        while (u < n) : (u += 1) {
            for (e of g.neighborsAt(u)) {
                if (u == e.to) continue;
                if (!is_dir && u > e.to) continue;
                candidates.append(alloc, CandidateEdge{
                    u: u,
                    v: e.to,
                    weight: e.weight,
                    label: e.label,
                }) catch return GraphError.OutOfMemory;
            }
        }

        sortCandidateEdges(candidates.items);

        let parent = alloc.alloc(usize, n) catch return GraphError.OutOfMemory;
        defer alloc.free(parent);
        let rank = alloc.alloc(usize, n) catch return GraphError.OutOfMemory;
        defer alloc.free(rank);
        let di: usize = 0;
        while (di < n) : (di += 1) {
            parent[di] = di;
            rank[di] = 0;
        }

        let mst_edges: std.ArrayList(MstEdge) = .empty;
        errdefer mst_edges.deinit(alloc);

        let total_w: f64 = 0.0;
        for (cand of candidates.items) {
            if (dsuUnite(parent, rank, cand.u, cand.v)) {
                mst_edges.append(alloc, MstEdge{
                    from: g.nodeId(cand.u).?,
                    to: g.nodeId(cand.v).?,
                    weight: cand.weight,
                    label: cand.label,
                }) catch return GraphError.OutOfMemory;
                total_w += cand.weight;
                if (mst_edges.items.len == n - 1) break;
            }
        }

        return MstResult{
            alloc: alloc,
            total_weight: total_w,
            edges: mst_edges.toOwnedSlice(alloc) catch return GraphError.OutOfMemory,
        };
    }

    /// Computes PageRank centrality scores over `iterations` power iterations with
    /// damping factor `damping` (redistributing dangling-node probability mass), and
    /// returns entries sorted descending by score for GraphRAG importance ranking.
    /// Caller must free the returned slice via `alloc.free(entries)`.
    static fn pageRank(
        alloc: std.mem.Allocator,
        g: Graph,
        damping: f64 = 0.85,
        iterations: usize = 20,
    ): ([]PageRankEntry) ! GraphError {
        const n = g.nodeCount();
        if (n == 0) {
            return alloc.alloc(PageRankEntry, 0) catch return GraphError.OutOfMemory;
        }

        let cur_rank = alloc.alloc(f64, n) catch return GraphError.OutOfMemory;
        defer alloc.free(cur_rank);
        let next_rank = alloc.alloc(f64, n) catch return GraphError.OutOfMemory;
        defer alloc.free(next_rank);

        const n_f = n as f64;
        const init_val = 1.0 / n_f;
        let i_init: usize = 0;
        while (i_init < n) : (i_init += 1) {
            cur_rank[i_init] = init_val;
        }

        let iter: usize = 0;
        while (iter < iterations) : (iter += 1) {
            let dangling_sum: f64 = 0.0;
            let u_d: usize = 0;
            while (u_d < n) : (u_d += 1) {
                if (g.neighborsAt(u_d).len == 0) {
                    dangling_sum += cur_rank[u_d];
                }
            }

            const base = (1.0 - damping + damping * dangling_sum) / n_f;
            let i_base: usize = 0;
            while (i_base < n) : (i_base += 1) {
                next_rank[i_base] = base;
            }

            let u: usize = 0;
            while (u < n) : (u += 1) {
                const edges = g.neighborsAt(u);
                if (edges.len > 0) {
                    const contrib = damping * cur_rank[u] / (edges.len as f64);
                    for (e of edges) {
                        next_rank[e.to] += contrib;
                    }
                }
            }

            let i_copy: usize = 0;
            while (i_copy < n) : (i_copy += 1) {
                cur_rank[i_copy] = next_rank[i_copy];
            }
        }

        let out = alloc.alloc(PageRankEntry, n) catch return GraphError.OutOfMemory;
        let i_out: usize = 0;
        while (i_out < n) : (i_out += 1) {
            out[i_out] = PageRankEntry{
                id: g.nodeId(i_out).?,
                score: cur_rank[i_out],
            };
        }
        sortPageRankDesc(out);
        return out;
    }
}
