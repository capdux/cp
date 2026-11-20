// Source: cp/lib/graph/graph.cp (Weighted Directed/Undirected Adjacency Graph)
// Pure CP adjacency-list graph engine supporting weighted directed & undirected graphs,
// ownership-safe node/label storage, BFS, DFS, and GraphRAG k-hop subgraph recall.

const std = @import("std");

/// Error set returned by graph construction, traversal, and algorithm operations.
pub err GraphError {
    NodeNotFound,
    InvalidVertex,
    NegativeWeight,
    CycleDetected,
    Disconnected,
    DuplicateEdge,
    OutOfMemory,
}

/// Directed or half-undirected adjacency edge entry storing target node index,
/// edge weight, and relation label.
pub struct GraphEdge {
    to: usize = 0,
    weight: f64 = 0.0,
    label: string = "",
}

/// Weighted directed or undirected adjacency-list graph with deterministic O(1)
/// node ID lookup and ownership-safe string storage.
pub struct Graph {
    alloc: std.mem.Allocator = undefined,
    directed: bool = true,
    node_ids: std.ArrayList([]u8) = .empty,
    id_to_idx: std.StringHashMapUnmanaged(usize) = .empty,
    adj: std.ArrayList(std.ArrayList(GraphEdge)) = .empty,
    edge_labels: std.ArrayList([]u8) = .empty,
    edge_count: usize = 0,

    /// Initializes an empty `Graph`. When `directed == false`, `addEdge`
    /// automatically inserts symmetric half-edges in both directions.
    pub static fn init(alloc: std.mem.Allocator, directed: bool): Graph {
        return Graph{
            alloc: alloc,
            directed: directed,
            node_ids: .empty,
            id_to_idx: .empty,
            adj: .empty,
            edge_labels: .empty,
            edge_count: 0,
        };
    }

    /// Releases all owned node IDs, edge labels, adjacency lists, and index maps.
    pub fn deinit(*_) {
        for (*list of _.adj.items) {
            list.deinit(_.alloc);
        }
        _.adj.deinit(_.alloc);

        for (id_buf of _.node_ids.items) {
            _.alloc.free(id_buf);
        }
        _.node_ids.deinit(_.alloc);

        _.id_to_idx.deinit(_.alloc);

        for (lbl of _.edge_labels.items) {
            _.alloc.free(lbl);
        }
        _.edge_labels.deinit(_.alloc);

        _.edge_count = 0;
    }

    /// Returns `true` if the graph is directed, `false` if undirected.
    pub fn isDirected(): bool {
        return _.directed;
    }

    /// Returns the total number of vertices in the graph.
    pub fn nodeCount(): usize {
        return _.node_ids.items.len;
    }

    /// Returns the logical edge count (incremented by 1 per `addEdge` call).
    pub fn edgeCount(): usize {
        return _.edge_count;
    }

    /// Inserts a node with identifier `id` and returns its dense node index.
    /// Idempotent: if `id` already exists, returns its existing index without duplication.
    pub fn addNode(*_, id: string): usize ! GraphError {
        if (existing := _.id_to_idx.get(id)) {
            return existing;
        }

        const owned_id = _.alloc.dupe(u8, id) catch return GraphError.OutOfMemory;
        errdefer _.alloc.free(owned_id);

        const idx = _.node_ids.items.len;
        _.node_ids.append(_.alloc, owned_id) catch return GraphError.OutOfMemory;
        errdefer * = _.node_ids.pop();

        let empty_edges: std.ArrayList(GraphEdge) = .empty;
        _.adj.append(_.alloc, empty_edges) catch return GraphError.OutOfMemory;
        errdefer * = _.adj.pop();

        _.id_to_idx.put(_.alloc, owned_id, idx) catch return GraphError.OutOfMemory;
        return idx;
    }

    /// Looks up the dense node index for `id`, or `null` if not present.
    pub fn findNode(id: string): usize? {
        return _.id_to_idx.get(id);
    }

    /// Returns the immutable node identifier for dense index `idx`, or `null` if out of range.
    pub fn nodeId(idx: usize): string? {
        if (idx >= _.node_ids.items.len) return null;
        return _.node_ids.items[idx];
    }

    /// Adds a weighted labeled edge from `src` to `dst`.
    /// When `directed == false` and `src != dst`, automatically adds the reverse edge.
    /// Returns `GraphError.NodeNotFound` if either endpoint is absent.
    pub fn addEdge(*_, src: string, dst: string, weight: f64, label: string = ""): void ! GraphError {
        const opt_u = _.id_to_idx.get(src);
        if (opt_u == null) return GraphError.NodeNotFound;
        const u = opt_u.?;
        const opt_v = _.id_to_idx.get(dst);
        if (opt_v == null) return GraphError.NodeNotFound;
        const v = opt_v.?;

        let stored_label: string = "";
        let appended_label = false;
        if (label.len > 0) {
            const duped = _.alloc.dupe(u8, label) catch return GraphError.OutOfMemory;
            _.edge_labels.append(_.alloc, duped) catch {
                _.alloc.free(duped);
                return GraphError.OutOfMemory;
            };
            stored_label = duped;
            appended_label = true;
        }
        errdefer if (appended_label) {
            if (popped := _.edge_labels.pop()) {
                _.alloc.free(popped);
            }
        }

        _.adj.items[u].append(_.alloc, GraphEdge{
            to: v,
            weight: weight,
            label: stored_label,
        }) catch return GraphError.OutOfMemory;

        if (!_.directed && u != v) {
            _.adj.items[v].append(_.alloc, GraphEdge{
                to: u,
                weight: weight,
                label: stored_label,
            }) catch {
                * = _.adj.items[u].pop();
                return GraphError.OutOfMemory;
            };
        }

        _.edge_count += 1;
    }

    /// Returns the outgoing adjacency edge slice for node `id`.
    /// Returns `GraphError.NodeNotFound` if `id` is not in the graph.
    pub fn neighbors(id: string): ([]const GraphEdge) ! GraphError {
        const opt_idx = _.id_to_idx.get(id);
        if (opt_idx == null) return GraphError.NodeNotFound;
        const idx = opt_idx.?;
        return _.adj.items[idx].items;
    }

    /// Returns the outgoing adjacency edge slice for dense node index `idx`,
    /// or an empty slice if `idx` is out of range.
    pub fn neighborsAt(idx: usize): []const GraphEdge {
        if (idx >= _.adj.items.len) return [];
        return _.adj.items[idx].items;
    }

    /// Performs Breadth-First Search starting from `start` and returns visited node IDs
    /// in traversal order. Returned string elements borrow from the graph; caller must
    /// free the outer slice with `alloc.free(order)`.
    pub fn bfs(alloc: std.mem.Allocator, start: string): ([]string) ! GraphError {
        const opt_start = _.id_to_idx.get(start);
        if (opt_start == null) return GraphError.NodeNotFound;
        const start_idx = opt_start.?;
        const n = _.node_ids.items.len;

        let visited = alloc.alloc(bool, n) catch return GraphError.OutOfMemory;
        defer alloc.free(visited);
        let i: usize = 0;
        while (i < n) : (i += 1) {
            visited[i] = false;
        }

        let queue: std.ArrayList(usize) = .empty;
        defer queue.deinit(alloc);

        let result: std.ArrayList(string) = .empty;
        errdefer result.deinit(alloc);

        visited[start_idx] = true;
        queue.append(alloc, start_idx) catch return GraphError.OutOfMemory;

        let head: usize = 0;
        while (head < queue.items.len) {
            const u = queue.items[head];
            head += 1;
            result.append(alloc, _.node_ids.items[u]) catch return GraphError.OutOfMemory;

            for (e of _.adj.items[u].items) {
                if (!visited[e.to]) {
                    visited[e.to] = true;
                    queue.append(alloc, e.to) catch return GraphError.OutOfMemory;
                }
            }
        }

        return result.toOwnedSlice(alloc) catch return GraphError.OutOfMemory;
    }

    /// Performs iterative Depth-First Search starting from `start` (matching recursive
    /// pre-order neighbor priority without recursion stack limits). Caller must free
    /// the outer slice with `alloc.free(order)`.
    pub fn dfs(alloc: std.mem.Allocator, start: string): ([]string) ! GraphError {
        const opt_start = _.id_to_idx.get(start);
        if (opt_start == null) return GraphError.NodeNotFound;
        const start_idx = opt_start.?;
        const n = _.node_ids.items.len;

        let visited = alloc.alloc(bool, n) catch return GraphError.OutOfMemory;
        defer alloc.free(visited);
        let i: usize = 0;
        while (i < n) : (i += 1) {
            visited[i] = false;
        }

        let stack: std.ArrayList(usize) = .empty;
        defer stack.deinit(alloc);

        let result: std.ArrayList(string) = .empty;
        errdefer result.deinit(alloc);

        stack.append(alloc, start_idx) catch return GraphError.OutOfMemory;

        while (stack.items.len > 0) {
            const u = stack.pop().?;
            if (visited[u]) continue;
            visited[u] = true;
            result.append(alloc, _.node_ids.items[u]) catch return GraphError.OutOfMemory;

            const edges = _.adj.items[u].items;
            let ei: usize = edges.len;
            while (ei > 0) {
                ei -= 1;
                const v = edges[ei].to;
                if (!visited[v]) {
                    stack.append(alloc, v) catch return GraphError.OutOfMemory;
                }
            }
        }

        return result.toOwnedSlice(alloc) catch return GraphError.OutOfMemory;
    }

    /// GraphRAG k-hop entity subgraph recall: returns all distinct node IDs reachable
    /// from `start` within at most `max_hops` edge traversals in BFS distance order
    /// (including `start` at hop 0). Caller must free the outer slice with `alloc.free(nodes)`.
    pub fn kHopSubgraph(alloc: std.mem.Allocator, start: string, max_hops: usize): ([]string) ! GraphError {
        const opt_start = _.id_to_idx.get(start);
        if (opt_start == null) return GraphError.NodeNotFound;
        const start_idx = opt_start.?;
        const n = _.node_ids.items.len;

        let visited = alloc.alloc(bool, n) catch return GraphError.OutOfMemory;
        defer alloc.free(visited);
        let i: usize = 0;
        while (i < n) : (i += 1) {
            visited[i] = false;
        }

        let queue_node: std.ArrayList(usize) = .empty;
        defer queue_node.deinit(alloc);

        let queue_depth: std.ArrayList(usize) = .empty;
        defer queue_depth.deinit(alloc);

        let result: std.ArrayList(string) = .empty;
        errdefer result.deinit(alloc);

        visited[start_idx] = true;
        queue_node.append(alloc, start_idx) catch return GraphError.OutOfMemory;
        queue_depth.append(alloc, 0) catch return GraphError.OutOfMemory;

        let head: usize = 0;
        while (head < queue_node.items.len) {
            const u = queue_node.items[head];
            const d = queue_depth.items[head];
            head += 1;

            result.append(alloc, _.node_ids.items[u]) catch return GraphError.OutOfMemory;

            if (d >= max_hops) continue;

            for (e of _.adj.items[u].items) {
                if (!visited[e.to]) {
                    visited[e.to] = true;
                    queue_node.append(alloc, e.to) catch return GraphError.OutOfMemory;
                    queue_depth.append(alloc, d + 1) catch return GraphError.OutOfMemory;
                }
            }
        }

        return result.toOwnedSlice(alloc) catch return GraphError.OutOfMemory;
    }
}
