// Source: cp/lib/graph/directed_graph.cp (Indexed Weighted Directed Graph & Adjacency List)
// Pure CP adjacency-list directed graph engine with dense vertex indexing,
// O(1) degree queries, duplicate-edge protection, and graph transposition.

const std = @import("std");
import { GraphError } from "./graph.cp";

/// Weighted directed edge entry storing destination vertex index and edge weight.
pub struct WeightedEdge {
    to: usize = 0,
    weight: f64 = 0.0,
}

/// Dense-indexed weighted directed graph backed by per-vertex adjacency lists
/// and maintained in-degree counters.
pub struct DirectedGraph {
    alloc: std.mem.Allocator = undefined,
    adj: std.ArrayList(std.ArrayList(WeightedEdge)) = .empty,
    in_degrees: std.ArrayList(usize) = .empty,
    edge_count: usize = 0,

    /// Initializes a directed graph with `vertex_count` vertices indexed `0..vertex_count - 1`.
    pub static fn init(alloc: std.mem.Allocator, vertex_count: usize): DirectedGraph ! GraphError {
        let adj: std.ArrayList(std.ArrayList(WeightedEdge)) = .empty;
        errdefer {
            for (*list of adj.items) {
                list.deinit(alloc);
            }
            adj.deinit(alloc);
        }

        let in_degrees: std.ArrayList(usize) = .empty;
        errdefer in_degrees.deinit(alloc);

        if (vertex_count > 0) {
            adj.ensureTotalCapacity(alloc, vertex_count) catch return GraphError.OutOfMemory;
            in_degrees.ensureTotalCapacity(alloc, vertex_count) catch return GraphError.OutOfMemory;
            let i: usize = 0;
            while (i < vertex_count) : (i += 1) {
                let empty_edges: std.ArrayList(WeightedEdge) = .empty;
                adj.append(alloc, empty_edges) catch return GraphError.OutOfMemory;
                in_degrees.append(alloc, 0) catch return GraphError.OutOfMemory;
            }
        }

        return DirectedGraph{
            alloc: alloc,
            adj: adj,
            in_degrees: in_degrees,
            edge_count: 0,
        };
    }

    /// Releases all adjacency lists and degree counters.
    pub fn deinit(*_) {
        for (*list of _.adj.items) {
            list.deinit(_.alloc);
        }
        _.adj.deinit(_.alloc);
        _.in_degrees.deinit(_.alloc);
        _.edge_count = 0;
    }

    /// Appends a new isolated vertex and returns its dense vertex index.
    pub fn addVertex(*_): usize ! GraphError {
        const idx = _.adj.items.len;
        let empty_edges: std.ArrayList(WeightedEdge) = .empty;
        _.adj.append(_.alloc, empty_edges) catch return GraphError.OutOfMemory;
        errdefer * = _.adj.pop();

        _.in_degrees.append(_.alloc, 0) catch return GraphError.OutOfMemory;
        return idx;
    }

    /// Adds a directed weighted edge `u -> v`.
    /// Returns `GraphError.InvalidVertex` if `u` or `v` is out of bounds,
    /// or `GraphError.DuplicateEdge` if `u -> v` already exists.
    pub fn addEdge(*_, u: usize, v: usize, weight: f64): void ! GraphError {
        const n = _.adj.items.len;
        if (u >= n || v >= n) {
            return GraphError.InvalidVertex;
        }
        for (e of _.adj.items[u].items) {
            if (e.to == v) {
                return GraphError.DuplicateEdge;
            }
        }
        _.adj.items[u].append(_.alloc, WeightedEdge{ to: v, weight: weight }) catch {
            return GraphError.OutOfMemory;
        };
        _.in_degrees.items[v] += 1;
        _.edge_count += 1;
    }

    /// Returns the outgoing edge slice for vertex `u`.
    pub fn neighbors(u: usize): ([]const WeightedEdge) ! GraphError {
        if (u >= _.adj.items.len) {
            return GraphError.InvalidVertex;
        }
        return _.adj.items[u].items;
    }

    /// Internal unchecked neighbor slice lookup for validated vertex indices.
    pub fn neighborsUnchecked(u: usize): []const WeightedEdge {
        return _.adj.items[u].items;
    }

    /// Returns the in-degree of vertex `v`.
    pub fn inDegree(v: usize): usize ! GraphError {
        if (v >= _.in_degrees.items.len) {
            return GraphError.InvalidVertex;
        }
        return _.in_degrees.items[v];
    }

    /// Returns the out-degree of vertex `u`.
    pub fn outDegree(u: usize): usize ! GraphError {
        if (u >= _.adj.items.len) {
            return GraphError.InvalidVertex;
        }
        return _.adj.items[u].items.len;
    }

    /// Returns the total number of vertices in the graph.
    pub fn vertexCount(): usize {
        return _.adj.items.len;
    }

    /// Returns the total number of directed edges in the graph.
    pub fn edgeCount(): usize {
        return _.edge_count;
    }

    /// Builds and returns the transposed graph (all edge directions reversed).
    pub fn transpose(alloc: std.mem.Allocator): DirectedGraph ! GraphError {
        const n = _.adj.items.len;
        let rev = DirectedGraph.init(alloc, n)?;
        errdefer rev.deinit();

        let u: usize = 0;
        while (u < n) : (u += 1) {
            for (e of _.adj.items[u].items) {
                rev.addEdge(e.to, u, e.weight)?;
            }
        }
        return rev;
    }
}
