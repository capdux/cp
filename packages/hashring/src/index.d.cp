// Type definitions for CP

export class RingPoint {
    hash: u64,
    node_idx: u16,
}

export class ReplicaList {
    nodes: [8]string,
    count: usize,
    fn empty(): ReplicaList;
    fn nodeAt(i: usize): string?;
}

export fn mulWrap64(a: u64, b: u64): u64;
export fn avalanche64(x: u64): u64;
export fn fnv1a64Feed(state: u64, b: u8): u64;
export fn hashKey(key: string): u64;
export fn hashVnode(name: string, v_idx: u16): u64;
export fn eqStr(a: string, b: string): bool;
export class HashRing {
    nodes: [32]string,
    node_active: [32]bool,
    active_nodes: usize,
    vnodes_per_node: u16,
    ring: [256]RingPoint,
    ring_len: usize,
    fn init(vnodes_per_node: u16): HashRing;
    fn nodeCount(): usize;
    fn vnodeCount(): usize;
    fn addNode(_: auto, name: string): bool;
    fn removeNode(_: auto, name: string): bool;
    fn lowerBound(target_hash: u64): usize;
    fn getNode(key: string): string?;
    fn getReplicas(key: string, max_replicas: usize): ReplicaList;
}

