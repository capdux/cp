// Type definitions for CP

export class ShortestPaths {
    ok: bool,
    node_count: usize,
    source: usize,
    dist: [MAX_NODES]i64,
    prev: [MAX_NODES]i16,
    fn isReachable(target: usize): bool;
    fn distanceTo(target: usize): i64;
    fn reconstructPath(target: usize, out: []u16): usize;
}

export fn emptyShortestPaths(n_nodes: usize, src: usize, ok_val: bool): ShortestPaths;
export class WeightedGraph {
    node_count: usize,
    edge_count: usize,
    head: [MAX_NODES]i16,
    edge_src: [MAX_EDGES]u16,
    edge_dst: [MAX_EDGES]u16,
    edge_weight: [MAX_EDGES]i64,
    edge_next: [MAX_EDGES]i16,
    fn init(n_nodes: usize): WeightedGraph;
    fn nodeCount(): usize;
    fn edgeCount(): usize;
    fn addEdge(_: auto, src: usize, dst: usize, weight: i64): bool;
    fn addUndirectedEdge(_: auto, u: usize, v: usize, weight: i64): bool;
    fn reset(_: auto, n_nodes: usize);
    fn dijkstra(source: usize): ShortestPaths;
    fn bellmanFord(source: usize): ShortestPaths;
}

