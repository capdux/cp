// Type definitions for CP

export fn pgcSatAdd(a: u64, b: u64): u64;
export class DiGraph {
    node_count: usize,
    edge_count: usize,
    edge_from: [256]usize,
    edge_to: [256]usize,
    edge_weight: [256]u64,
    head: [64]usize,
    tail: [64]usize,
    next_edge: [256]usize,
    out_deg: [64]usize,
    in_deg: [64]usize,
    fn new(): DiGraph;
    fn nodeCount(): usize;
    fn edgeCount(): usize;
    fn addNode(_: auto): usize?;
    fn addEdge(_: auto, from_node: usize, to_node: usize, weight: u64): bool;
    fn addUndirectedEdge(_: auto, a: usize, b: usize, weight: u64): bool;
    fn hasEdge(u: usize, v: usize): bool;
    fn outDegree(u: usize): usize;
    fn inDegree(u: usize): usize;
    fn bfs(start: usize, order_out: []usize): usize;
    fn dfs(start: usize, order_out: []usize): usize;
    fn topoSort(order_out: []usize): usize?;
    fn isCyclic(): bool;
    fn dijkstra(start: usize, dist_out: []u64): bool;
    fn hasPath(src: usize, dst: usize): bool;
}

