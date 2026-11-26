// @cap/consistent-hash — Zero-allocation consistent hash ring with virtual nodes for CP
// Clean-room MIT implementation inspired by Rust `hashring` and npm `hashring`.

pub const MAX_NODES: usize = 32;
pub const MAX_VNODES: usize = 256;
pub const MAX_REPLICAS: usize = 8;

const FNV64_OFFSET_BASIS: u64 = ((0xcbf29ce4 as u64) << 32) | (0x84222325 as u64);
const FNV64_PRIME: u64 = ((0x00000100 as u64) << 32) | (0x000001b3 as u64);
const MIX_K1: u64 = ((0xbf58476d as u64) << 32) | (0x1ce4e5b9 as u64);
const MIX_K2: u64 = ((0x94d049bb as u64) << 32) | (0x133111eb as u64);

pub struct RingPoint {
    pub hash: u64,
    pub node_idx: u16,
}

pub struct ReplicaList {
    pub nodes: [8]string,
    pub count: usize,

    pub static fn empty() -> ReplicaList {
        return ReplicaList{
            nodes: ["", "", "", "", "", "", "", ""],
            count: 0,
        };
    }

    pub fn nodeAt(i: usize) -> string? {
        if (i < _.count) {
            return _.nodes[i];
        }
        return null;
    }
}

// ---------------------------------------------------------------------------
// Internal Overflow-Safe Hashing & String Helpers
// ---------------------------------------------------------------------------

fn mulWrap64(a: u64, b: u64) -> u64 {
    const a_lo = a & 0xFFFFFFFF;
    const a_hi = a >> 32;
    const b_lo = b & 0xFFFFFFFF;
    const b_hi = b >> 32;

    const p0 = a_lo * b_lo;
    const p1 = (a_lo * b_hi) & 0xFFFFFFFF;
    const p2 = (a_hi * b_lo) & 0xFFFFFFFF;
    const hi = ((p0 >> 32) + p1 + p2) & 0xFFFFFFFF;
    return (p0 & 0xFFFFFFFF) | (hi << 32);
}

fn avalanche64(x: u64) -> u64 {
    let z = x;
    z = mulWrap64(z ^ (z >> 30), MIX_K1);
    z = mulWrap64(z ^ (z >> 27), MIX_K2);
    return z ^ (z >> 31);
}

fn fnv1a64Feed(state: u64, b: u8) -> u64 {
    return mulWrap64(state ^ (b as u64), FNV64_PRIME);
}

fn hashKey(key: string) -> u64 {
    let h = FNV64_OFFSET_BASIS;
    let i: usize = 0;
    while (i < key.len) {
        h = fnv1a64Feed(h, key[i]);
        i += 1;
    }
    return avalanche64(h);
}

fn hashVnode(name: string, v_idx: u16) -> u64 {
    let h = FNV64_OFFSET_BASIS;
    let i: usize = 0;
    while (i < name.len) {
        h = fnv1a64Feed(h, name[i]);
        i += 1;
    }
    h = fnv1a64Feed(h, '#');
    h = fnv1a64Feed(h, (v_idx & 0xFF) as u8);
    h = fnv1a64Feed(h, ((v_idx >> 8) & 0xFF) as u8);
    return avalanche64(h);
}

fn eqStr(a: string, b: string) -> bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Public HashRing Struct
// ---------------------------------------------------------------------------

pub struct HashRing {
    pub nodes: [32]string,
    pub node_active: [32]bool,
    pub active_nodes: usize,
    pub vnodes_per_node: u16,
    pub ring: [256]RingPoint,
    pub ring_len: usize,

    /// Initializes an empty consistent hash ring with `vnodes_per_node` virtual nodes per physical node.
    pub static fn init(vnodes_per_node: u16) -> HashRing {
        let nodes: [32]string = undefined;
        let active: [32]bool = undefined;
        let i: usize = 0;
        while (i < MAX_NODES) {
            nodes[i] = "";
            active[i] = false;
            i += 1;
        }
        let ring: [256]RingPoint = undefined;
        let r: usize = 0;
        while (r < MAX_VNODES) {
            ring[r] = RingPoint{ hash: 0, node_idx: 0 };
            r += 1;
        }
        const eff_vnodes: u16 = if (vnodes_per_node == 0) 1 as u16 else vnodes_per_node;
        return HashRing{
            nodes: nodes,
            node_active: active,
            active_nodes: 0,
            vnodes_per_node: eff_vnodes,
            ring: ring,
            ring_len: 0,
        };
    }

    pub fn nodeCount() -> usize {
        return _.active_nodes;
    }

    pub fn vnodeCount() -> usize {
        return _.ring_len;
    }

    /// Adds a physical node and inserts `vnodes_per_node` sorted virtual points onto the ring.
    /// Returns `false` if `name` is empty, already exists, or capacity is exceeded.
    pub fn addNode(*_, name: string) -> bool {
        if (name.len == 0) {
            return false;
        }
        let free_slot: usize = MAX_NODES;
        let i: usize = 0;
        while (i < MAX_NODES) {
            if (_.node_active[i]) {
                if (eqStr(_.nodes[i], name)) {
                    return false;
                }
            } else if (free_slot == MAX_NODES) {
                free_slot = i;
            }
            i += 1;
        }
        if (free_slot == MAX_NODES) {
            return false;
        }
        const add_count = _.vnodes_per_node as usize;
        if (_.ring_len + add_count > MAX_VNODES) {
            return false;
        }

        _.nodes[free_slot] = name;
        _.node_active[free_slot] = true;
        _.active_nodes += 1;

        let v: u16 = 0;
        while (v < _.vnodes_per_node) {
            const h = hashVnode(name, v);
            const pt = RingPoint{ hash: h, node_idx: free_slot as u16 };
            // Insertion sort into _.ring[0.._.ring_len] maintaining ascending hash order
            let pos = _.ring_len;
            while (pos > 0 && _.ring[pos - 1].hash > h) {
                _.ring[pos] = _.ring[pos - 1];
                pos -= 1;
            }
            _.ring[pos] = pt;
            _.ring_len += 1;
            v += 1;
        }
        return true;
    }

    /// Removes a physical node and all of its virtual points from the ring.
    pub fn removeNode(*_, name: string) -> bool {
        let target_slot: usize = MAX_NODES;
        let i: usize = 0;
        while (i < MAX_NODES) {
            if (_.node_active[i] && eqStr(_.nodes[i], name)) {
                target_slot = i;
                break;
            }
            i += 1;
        }
        if (target_slot == MAX_NODES) {
            return false;
        }

        _.node_active[target_slot] = false;
        _.nodes[target_slot] = "";
        _.active_nodes -= 1;

        // Compact ring in-place preserving sorted order
        let write_idx: usize = 0;
        let read_idx: usize = 0;
        while (read_idx < _.ring_len) {
            if ((_.ring[read_idx].node_idx as usize) != target_slot) {
                _.ring[write_idx] = _.ring[read_idx];
                write_idx += 1;
            }
            read_idx += 1;
        }
        _.ring_len = write_idx;
        return true;
    }

    fn lowerBound(target_hash: u64) -> usize {
        let lo: usize = 0;
        let hi: usize = _.ring_len;
        while (lo < hi) {
            const mid = lo + (hi - lo) / 2;
            if (_.ring[mid].hash < target_hash) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return if (lo < _.ring_len) lo else 0 as usize;
    }

    /// Locates the primary clockwise physical node for `key` in O(log V) time.
    pub fn getNode(key: string) -> string? {
        if (_.ring_len == 0) {
            return null;
        }
        const idx = _.lowerBound(hashKey(key));
        return _.nodes[_.ring[idx].node_idx as usize];
    }

    /// Walks clockwise from `key`'s hash position to collect up to `max_replicas`
    /// distinct physical nodes for replication / failover.
    pub fn getReplicas(key: string, max_replicas: usize) -> ReplicaList {
        let res = ReplicaList.empty();
        if (_.ring_len == 0 || max_replicas == 0) {
            return res;
        }

        let want = max_replicas;
        if (want > MAX_REPLICAS) want = MAX_REPLICAS;
        if (want > _.active_nodes) want = _.active_nodes;

        let seen: [32]bool = undefined;
        let s_i: usize = 0;
        while (s_i < MAX_NODES) {
            seen[s_i] = false;
            s_i += 1;
        }

        const start = _.lowerBound(hashKey(key));
        let step: usize = 0;
        while (step < _.ring_len && res.count < want) {
            const pos = (start + step) % _.ring_len;
            const n_idx = _.ring[pos].node_idx as usize;
            if (!seen[n_idx]) {
                seen[n_idx] = true;
                res.nodes[res.count] = _.nodes[n_idx];
                res.count += 1;
            }
            step += 1;
        }

        return res;
    }
}
