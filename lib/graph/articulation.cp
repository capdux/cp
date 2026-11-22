// Origin: Pure-CP Stdlib (Zero-Alloc Iterative Tarjan Articulation Points, Bridges & SPOF Analyzer) | Created: 2026-11-23
// Pure CP zero-heap-allocation undirected graph topology resilience analyzer:
//   - Fixed-capacity adjacency list (up to 64 nodes / 256 undirected edges, supports parallel multi-edges
//     without false bridge detection, self-loops, and disconnected multi-component forests)
//   - 100% iterative explicit-stack Tarjan DFS (zero recursion, zero stack-overflow risk) in O(V + E)
//   - Identifies all Articulation Points (cut vertices / SPOF), Bridges (cut edges), connected components,
//     edge-partitioned Biconnected Components (BCC IDs), and 2-edge-connectivity / biconnectivity flags

const std = @import("std");

// ============================================================================
// 1. Constants & Data Models
// ============================================================================

/// Maximum number of vertices supported inline (zero dynamic heap).
pub const ARTIC_MAX_NODES: usize = 64;

/// Maximum number of undirected edges supported inline (internal 512 directed half-edges).
pub const ARTIC_MAX_EDGES: usize = 256;

/// Sentinel indicating no edge / no parent edge.
pub const ARTIC_NO_EDGE: u16 = 0xFFFF;

/// Sentinel indicating an edge has no biconnected component assignment (e.g. self-loop or unused).
pub const ARTIC_NO_BCC: u16 = 0xFFFF;

/// Undirected edge descriptor `(u, v)`.
pub struct ArticEdge {
    u: u16 = 0;
    v: u16 = 0;
}

/// Complete topology resilience & SPOF analysis result returned by `ArticulationGraph.analyze`.
pub struct ArticulationResult {
    node_count: usize = 0;
    edge_count: usize = 0;
    connected_components: usize = 0;
    articulation_count: usize = 0;
    bridge_count: usize = 0;
    bcc_count: usize = 0;
    max_bcc_edges: usize = 0;
    is_two_edge_connected: bool = false;
    is_biconnected: bool = false;
    articulation_mask: [ARTIC_MAX_NODES]bool = [false; ARTIC_MAX_NODES];
    bridge_mask: [ARTIC_MAX_EDGES]bool = [false; ARTIC_MAX_EDGES];
    edge_bcc_id: [ARTIC_MAX_EDGES]u16 = [ARTIC_NO_BCC; ARTIC_MAX_EDGES];
    node_component_id: [ARTIC_MAX_NODES]u16 = [0; ARTIC_MAX_NODES];

    /// Returns `true` if `node` is an articulation point (Single Point of Failure).
    pub fn isArticulation(node: usize): bool {
        if (node >= node_count) return false;
        return articulation_mask[node];
    }

    /// Returns `true` if undirected edge `edge_idx` is a bridge (cut edge).
    pub fn isBridge(edge_idx: usize): bool {
        if (edge_idx >= edge_count) return false;
        return bridge_mask[edge_idx];
    }

    /// Returns the 0-based biconnected component ID for `edge_idx`, or `null` for self-loops / OOB.
    pub fn edgeBcc(edge_idx: usize): u16? {
        if (edge_idx >= edge_count) return null;
        const id = edge_bcc_id[edge_idx];
        if (id == ARTIC_NO_BCC) return null;
        return id;
    }

    /// Returns the 0-based connected component ID for `node`, or `null` if out of bounds.
    pub fn nodeComponent(node: usize): u16? {
        if (node >= node_count) return null;
        return node_component_id[node];
    }

    /// Collects all articulation point node indices in ascending order into `out_nodes`.
    /// Returns the number of indices written (truncation-safe).
    pub fn collectArticulations(out_nodes: *[]usize): usize {
        let written: usize = 0;
        let i: usize = 0;
        while (i < node_count && written < out_nodes.len) : (i += 1) {
            if (articulation_mask[i]) {
                out_nodes[written] = i;
                written += 1;
            }
        }
        return written;
    }

    /// Collects all bridge edge indices in ascending order into `out_edges`.
    /// Returns the number of indices written (truncation-safe).
    pub fn collectBridges(out_edges: *[]usize): usize {
        let written: usize = 0;
        let i: usize = 0;
        while (i < edge_count && written < out_edges.len) : (i += 1) {
            if (bridge_mask[i]) {
                out_edges[written] = i;
                written += 1;
            }
        }
        return written;
    }
}

struct TarjanDfsFrame {
    node: u16 = 0;
    next_half: u16 = ARTIC_NO_EDGE;
    in_edge: u16 = ARTIC_NO_EDGE;
    child_count: u16 = 0;
}

// ============================================================================
// 2. Zero-Alloc Undirected Graph & Iterative Tarjan SPOF Engine
// ============================================================================

/// Fixed-capacity undirected graph supporting parallel edges and iterative O(V + E)
/// Tarjan articulation point, bridge, and biconnected component analysis.
pub struct ArticulationGraph {
    node_count: usize = 0;
    edge_count: usize = 0;
    half_edge_count: usize = 0;
    head: [ARTIC_MAX_NODES]u16 = [ARTIC_NO_EDGE; ARTIC_MAX_NODES];
    half_to: [ARTIC_MAX_EDGES * 2]u16 = [0; ARTIC_MAX_EDGES * 2];
    half_next: [ARTIC_MAX_EDGES * 2]u16 = [ARTIC_NO_EDGE; ARTIC_MAX_EDGES * 2];
    half_edge_id: [ARTIC_MAX_EDGES * 2]u16 = [0; ARTIC_MAX_EDGES * 2];
    edges: [ARTIC_MAX_EDGES]ArticEdge = [ArticEdge{}; ARTIC_MAX_EDGES];

    /// Initializes an undirected graph with `node_count` vertices (`0..=ARTIC_MAX_NODES`).
    /// Returns `null` if `node_count > ARTIC_MAX_NODES`.
    pub fn init(node_count: usize): ArticulationGraph? {
        if (node_count > ARTIC_MAX_NODES) return null;
        return ArticulationGraph{
            node_count: node_count,
        };
    }

    /// Adds an undirected edge `(u, v)` and returns its 0-based `edge_idx`.
    /// Parallel edges between the same `(u, v)` are preserved and never falsely marked as bridges.
    /// Self-loops `(u == v)` are accepted safely without affecting articulation points or bridges.
    /// Returns `null` if `u`/`v` are out of bounds or edge capacity (`256`) is exhausted.
    pub fn addEdge(*_, u: usize, v: usize): usize? {
        if (u >= node_count || v >= node_count) return null;
        if (edge_count >= ARTIC_MAX_EDGES) return null;

        const eid = edge_count;
        edges[eid] = ArticEdge{ u: u as u16, v: v as u16 };
        edge_count += 1;

        // Self-loops cannot be bridges or create cut vertices; skip adjacency insertion.
        if (u == v) {
            return eid;
        }

        // Insert u -> v half-edge
        const h1 = half_edge_count;
        half_to[h1] = v as u16;
        half_edge_id[h1] = eid as u16;
        half_next[h1] = head[u];
        head[u] = h1 as u16;
        half_edge_count += 1;

        // Insert v -> u half-edge
        const h2 = half_edge_count;
        half_to[h2] = u as u16;
        half_edge_id[h2] = eid as u16;
        half_next[h2] = head[v];
        head[v] = h2 as u16;
        half_edge_count += 1;

        return eid;
    }

    pub fn nodeCount(): usize {
        return node_count;
    }

    pub fn edgeCount(): usize {
        return edge_count;
    }

    pub fn getEdge(edge_idx: usize): ArticEdge? {
        if (edge_idx >= edge_count) return null;
        return edges[edge_idx];
    }

    /// Runs 100% iterative explicit-stack Tarjan DFS in O(V + E) time and O(V + E) stack memory
    /// to identify all articulation points, bridges, connected components, and biconnected components.
    pub fn analyze(): ArticulationResult {
        let res = ArticulationResult{
            node_count: node_count,
            edge_count: edge_count,
        };
        if (node_count == 0) {
            res.is_two_edge_connected = true;
            res.is_biconnected = true;
            return res;
        }

        let disc: [ARTIC_MAX_NODES]u16 = [0; ARTIC_MAX_NODES];
        let low: [ARTIC_MAX_NODES]u16 = [0; ARTIC_MAX_NODES];
        let stack: [ARTIC_MAX_NODES]TarjanDfsFrame = [TarjanDfsFrame{}; ARTIC_MAX_NODES];
        let edge_stack: [ARTIC_MAX_EDGES]u16 = [0; ARTIC_MAX_EDGES];

        let timer: u16 = 0;
        let comp_id: u16 = 0;
        let bcc_id: u16 = 0;
        let edge_top: usize = 0;

        let start: usize = 0;
        while (start < node_count) : (start += 1) {
            if (disc[start] != 0) continue;

            timer += 1;
            disc[start] = timer;
            low[start] = timer;
            res.node_component_id[start] = comp_id;

            let stack_top: usize = 0;
            stack[0] = TarjanDfsFrame{
                node: start as u16,
                next_half: head[start],
                in_edge: ARTIC_NO_EDGE,
                child_count: 0,
            };
            stack_top = 1;

            while (stack_top > 0) {
                const top_idx = stack_top - 1;
                const u: usize = stack[top_idx].node as usize;
                const he: u16 = stack[top_idx].next_half;

                if (he != ARTIC_NO_EDGE) {
                    const he_idx: usize = he as usize;
                    stack[top_idx].next_half = half_next[he_idx];

                    const v: usize = half_to[he_idx] as usize;
                    const eid: u16 = half_edge_id[he_idx];

                    // Skip the exact parent tree edge (do NOT skip parallel edges to parent!)
                    if (eid == stack[top_idx].in_edge) {
                        continue;
                    }

                    if (disc[v] == 0) {
                        // Tree edge u -> v
                        timer += 1;
                        disc[v] = timer;
                        low[v] = timer;
                        res.node_component_id[v] = comp_id;
                        stack[top_idx].child_count += 1;

                        if (edge_top < ARTIC_MAX_EDGES) {
                            edge_stack[edge_top] = eid;
                            edge_top += 1;
                        }

                        stack[stack_top] = TarjanDfsFrame{
                            node: v as u16,
                            next_half: head[v],
                            in_edge: eid,
                            child_count: 0,
                        };
                        stack_top += 1;
                    } else if (disc[v] < disc[u]) {
                        // Back edge u -> v (visited ancestor, or parallel edge to parent)
                        if (edge_top < ARTIC_MAX_EDGES) {
                            edge_stack[edge_top] = eid;
                            edge_top += 1;
                        }
                        if (disc[v] < low[u]) {
                            low[u] = disc[v];
                        }
                    }
                } else {
                    // Finished all incident edges of u; backtrack to parent
                    const child_u = u;
                    const peid = stack[top_idx].in_edge;
                    stack_top -= 1;

                    if (stack_top > 0) {
                        const p_idx = stack_top - 1;
                        const parent: usize = stack[p_idx].node as usize;

                        if (low[child_u] < low[parent]) {
                            low[parent] = low[child_u];
                        }

                        // Bridge check: no back-edge from child_u's subtree reaches parent or above
                        if (low[child_u] > disc[parent]) {
                            const peid_idx: usize = peid as usize;
                            if (!res.bridge_mask[peid_idx]) {
                                res.bridge_mask[peid_idx] = true;
                                res.bridge_count += 1;
                            }
                        }

                        // Articulation point check
                        const parent_is_root = stack[p_idx].in_edge == ARTIC_NO_EDGE;
                        if (parent_is_root) {
                            if (stack[p_idx].child_count >= 2 && !res.articulation_mask[parent]) {
                                res.articulation_mask[parent] = true;
                                res.articulation_count += 1;
                            }
                        } else if (low[child_u] >= disc[parent]) {
                            if (!res.articulation_mask[parent]) {
                                res.articulation_mask[parent] = true;
                                res.articulation_count += 1;
                            }
                        }

                        // Biconnected component extraction when low[child_u] >= disc[parent]
                        if (low[child_u] >= disc[parent]) {
                            let bcc_edges: usize = 0;
                            while (edge_top > 0) {
                                edge_top -= 1;
                                const popped = edge_stack[edge_top];
                                res.edge_bcc_id[popped as usize] = bcc_id;
                                bcc_edges += 1;
                                if (popped == peid) break;
                            }
                            if (bcc_edges > 0) {
                                if (bcc_edges > res.max_bcc_edges) {
                                    res.max_bcc_edges = bcc_edges;
                                }
                                bcc_id += 1;
                            }
                        }
                    }
                }
            }

            comp_id += 1;
        }

        res.connected_components = comp_id as usize;
        res.bcc_count = bcc_id as usize;
        res.is_two_edge_connected = (res.connected_components == 1 && res.bridge_count == 0);
        res.is_biconnected = (res.connected_components == 1 && res.articulation_count == 0 && res.bridge_count == 0);
        return res;
    }
}
