// Type definitions for CP

export class TopoResult {
    ok: bool,
    count: usize,
    order: [MAX_NODES]u16,
    levels: [MAX_NODES]u16,
    fn empty(): TopoResult;
}

export fn clampNodes(n: usize): usize;
export class TopoGraph {
    node_count: usize,
    edge_count: usize,
    head: [MAX_NODES]i16,
    edge_to: [MAX_EDGES]u16,
    edge_next: [MAX_EDGES]i16,
    in_deg: [MAX_NODES]u16,
    out_deg: [MAX_NODES]u16,
    fn init(node_count: usize): TopoGraph;
    fn reset(_: auto, node_count: usize);
    fn add_edge(_: auto, src: usize, dst: usize): bool;
    fn in_degree(node: usize): u16;
    fn out_degree(node: usize): u16;
    fn sort(): TopoResult;
    fn has_cycle(): bool;
}

