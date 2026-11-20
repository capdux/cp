// Source: cp/lib/lock/wait_graph.cp (Directed Wait-For Graph & Cycle Detector for Deadlock Detection)
// Pure CP directed wait-for graph for tracking resource wait dependencies between
// owners/agents and detecting deadlocks via iterative 3-color DFS cycle detection.

const std = @import("std");

struct WaitNode {
    id: []u8,
    edges: std.ArrayList(usize) = .empty,

    fn deinit(*_, alloc: std.mem.Allocator) {
        _.edges.deinit(alloc);
        alloc.free(_.id);
    }
}

/// Directed wait-for graph tracking `waiter -> holder` dependencies.
pub struct WaitForGraph {
    alloc: std.mem.Allocator = undefined,
    nodes: std.ArrayList(WaitNode) = .empty,
    node_index: std.StringHashMapUnmanaged(usize) = .empty,

    /// Initializes an empty `WaitForGraph` backed by `alloc`.
    pub static fn init(alloc: std.mem.Allocator): WaitForGraph {
        return WaitForGraph{
            alloc: alloc,
            nodes: .empty,
            node_index: .empty,
        };
    }

    /// Releases all owned node identifiers, adjacency lists, and index maps.
    pub fn deinit(*_) {
        for (*n of _.nodes.items) {
            n.deinit(_.alloc);
        }
        _.nodes.deinit(_.alloc);
        _.node_index.deinit(_.alloc);
    }

    fn ensureNode(*_, id: string): usize ! {
        if (idx := _.node_index.get(id)) {
            return idx;
        }
        const owned = _.alloc.dupe(u8, id) catch return error.OutOfMemory;
        errdefer _.alloc.free(owned);

        const idx = _.nodes.items.len;
        _.nodes.append(_.alloc, WaitNode{
            id: owned,
            edges: .empty,
        }) catch return error.OutOfMemory;
        errdefer * = _.nodes.pop();

        _.node_index.put(_.alloc, owned, idx) catch return error.OutOfMemory;
        return idx;
    }

    /// Adds a directed wait edge `waiter -> holder` (idempotent if already present).
    pub fn addEdge(*_, waiter: string, holder: string): ! {
        const w_idx = _.ensureNode(waiter)?;
        const h_idx = _.ensureNode(holder)?;

        for (existing of _.nodes.items[w_idx].edges.items) {
            if (existing == h_idx) {
                return;
            }
        }
        _.nodes.items[w_idx].edges.append(_.alloc, h_idx) catch return error.OutOfMemory;
    }

    /// Removes the directed wait edge `waiter -> holder` if present.
    pub fn removeEdge(*_, waiter: string, holder: string) {
        const w_idx = _.node_index.get(waiter) ?? return;
        const h_idx = _.node_index.get(holder) ?? return;

        let edges = *_.nodes.items[w_idx].edges;
        let i: usize = 0;
        while (i < edges.items.len) {
            if (edges.items[i] == h_idx) {
                * = edges.swapRemove(i);
            } else {
                i += 1;
            }
        }
    }

    /// Removes all outgoing wait edges originating from `waiter`.
    pub fn clearWaiter(*_, waiter: string) {
        const w_idx = _.node_index.get(waiter) ?? return;
        _.nodes.items[w_idx].edges.clearRetainingCapacity();
    }

    /// Returns the number of tracked nodes in the graph.
    pub fn nodeCount(): usize {
        return _.nodes.items.len;
    }

    /// Returns the total number of directed wait edges in the graph.
    pub fn edgeCount(): usize {
        let total: usize = 0;
        for (n of _.nodes.items) {
            total += n.edges.items.len;
        }
        return total;
    }

    /// Detects whether any directed cycle exists in the wait-for graph using
    /// iterative 3-color DFS (0 = unvisited, 1 = visiting, 2 = visited).
    pub fn hasCycle(alloc: std.mem.Allocator): bool ! {
        const n = _.nodes.items.len;
        if (n == 0) {
            return false;
        }

        let state = alloc.alloc(u8, n) catch return error.OutOfMemory;
        defer alloc.free(state);
        @memset(state, 0);

        let stack_node: std.ArrayList(usize) = .empty;
        defer stack_node.deinit(alloc);
        let stack_cursor: std.ArrayList(usize) = .empty;
        defer stack_cursor.deinit(alloc);

        let start: usize = 0;
        while (start < n) : (start += 1) {
            if (state[start] != 0) {
                continue;
            }

            state[start] = 1;
            stack_node.append(alloc, start) catch return error.OutOfMemory;
            stack_cursor.append(alloc, 0) catch return error.OutOfMemory;

            while (stack_node.items.len > 0) {
                const depth = stack_node.items.len - 1;
                const u = stack_node.items[depth];
                const cur = stack_cursor.items[depth];
                const edges = _.nodes.items[u].edges.items;

                if (cur < edges.len) {
                    stack_cursor.items[depth] = cur + 1;
                    const v = edges[cur];
                    if (state[v] == 1) {
                        return true;
                    }
                    if (state[v] == 0) {
                        state[v] = 1;
                        stack_node.append(alloc, v) catch return error.OutOfMemory;
                        stack_cursor.append(alloc, 0) catch return error.OutOfMemory;
                    }
                } else {
                    state[u] = 2;
                    * = stack_node.pop();
                    * = stack_cursor.pop();
                }
            }
        }

        return false;
    }
}
