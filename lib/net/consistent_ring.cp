// Origin: Pure-CP Stdlib (Zero-Heap Weighted Virtual-Node Consistent Hash Ring, Bounded-Load & Multi-Replica Router) | Created: 2026-11-22
// Pure CP zero-heap-allocation O(log V) consistent hashing ring for distributed caching,
// AI Agent cluster sharding, and microservice load balancing:
//   1. `RingPhysicalNode`, `RingVirtualSlot`, `RingStats`
//   2. `hashRingKey`: deterministic FNV-1a 64-bit + SplitMix64 avalanche key hash
//   3. `ConsistentHashRing`:
//      - Weighted virtual nodes (`weight * vnodes_per_unit`) with deterministic SplitMix64 ring tokens
//      - O(log V) binary-search clockwise routing with Google Bounded-Load (`active_load < max_load`)
//      - Deduplicated multi-replica routing (`routeReplicas`) skipping duplicate physical hosts,
//        unhealthy nodes, and overloaded nodes

const std = @import("std");

/// SplitMix64 finalizer for uniform 64-bit ring token distribution.
fn splitMix64(x: u64): u64 {
    let z: u64 = x +% 0x9E3779B97F4A7C15;
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    return z ^ (z >> 31);
}

/// Computes a deterministic 64-bit ring hash for a routing key (FNV-1a 64-bit + SplitMix64 avalanche).
pub fn hashRingKey(key: string): u64 {
    let h: u64 = 0xCBF29CE484222325;
    let i: usize = 0;
    while (i < key.len) : (i += 1) {
        h = h ^ (key[i] as u64);
        h = h *% 0x100000001B3;
    }
    return splitMix64(h);
}

/// Generates a deterministic virtual-node ring token for `(node_id, vnode_idx)`.
fn vnodeToken(node_id: u32, vnode_idx: usize): u64 {
    const n_mix: u64 = splitMix64((node_id as u64) +% 0xD1B54A32D192ED03);
    const v_mix: u64 = (vnode_idx as u64) *% 0x9E3779B97F4A7C15;
    return splitMix64(n_mix ^ v_mix);
}

/// Physical backend node descriptor stored in caller-provided ring buffer.
pub struct RingPhysicalNode {
    active: bool = false;
    node_id: u32 = 0;
    weight: u32 = 1;
    healthy: bool = true;
    active_load: u32 = 0;
    max_load: u32 = 0;

    /// Returns `true` if this physical node is active, healthy, and within its load bound.
    /// When `max_load == 0`, load bounding is disabled (unbounded capacity).
    fn isAvailable(): bool {
        if (!_.active || !_.healthy) return false;
        if (_.max_load > 0 && _.active_load >= _.max_load) return false;
        return true;
    }
}

/// Single sorted virtual node slot on the 64-bit hash ring.
pub struct RingVirtualSlot {
    token: u64 = 0;
    node_id: u32 = 0;
    node_idx: u32 = 0;
}

/// Point-in-time telemetry snapshot for `ConsistentHashRing`.
pub struct RingStats {
    physical_nodes: usize = 0;
    healthy_nodes: usize = 0;
    overloaded_nodes: usize = 0;
    virtual_slots: usize = 0;
    virtual_capacity: usize = 0;
    total_weight: u64 = 0;
    total_load: u64 = 0;
}

/// Total ordering comparator for virtual ring slots: ascending `(token, node_id)`.
fn isSlotBefore(a: RingVirtualSlot, b: RingVirtualSlot): bool {
    if (a.token != b.token) return a.token < b.token;
    return a.node_id < b.node_id;
}

/// Zero-heap-allocation weighted virtual-node consistent hash ring with Bounded-Load
/// hotspot protection and physical-host-deduplicated multi-replica routing.
pub struct ConsistentHashRing {
    nodes: []RingPhysicalNode;
    vslots: []RingVirtualSlot;
    vnodes_per_unit: usize = 16;
    node_count: usize = 0;
    vslot_count: usize = 0;

    /// Initializes an empty consistent hash ring over caller-provided buffers.
    /// Returns `null` if `nodes_buf.len == 0`, `vslots_buf.len == 0`, or `vnodes_per_unit == 0`.
    static fn init(
        nodes_buf: *[]RingPhysicalNode,
        vslots_buf: *[]RingVirtualSlot,
        vnodes_per_unit: usize,
    ): ConsistentHashRing? {
        if (nodes_buf.len == 0 || vslots_buf.len == 0 || vnodes_per_unit == 0) return null;

        let i: usize = 0;
        while (i < nodes_buf.len) : (i += 1) {
            nodes_buf[i] = RingPhysicalNode{};
        }
        let j: usize = 0;
        while (j < vslots_buf.len) : (j += 1) {
            vslots_buf[j] = RingVirtualSlot{};
        }

        return ConsistentHashRing{
            nodes: nodes_buf,
            vslots: vslots_buf,
            vnodes_per_unit: vnodes_per_unit,
            node_count: 0,
            vslot_count: 0,
        };
    }

    /// Registers a physical node with `weight * vnodes_per_unit` deterministic virtual nodes.
    /// `max_load > 0` enables Google Bounded-Load (`active_load < max_load`); `max_load == 0` is unbounded.
    /// Returns `false` on `weight == 0`, duplicate `node_id`, node table full, or insufficient `vslots` capacity.
    fn addNode(*_, node_id: u32, weight: u32, max_load: u32): bool {
        if (weight == 0) return false;
        const w_usize: usize = weight as usize;
        if (w_usize > std.math.maxInt(usize) / _.vnodes_per_unit) return false;
        const needed_vnodes: usize = w_usize * _.vnodes_per_unit;
        if (needed_vnodes > _.vslots.len - _.vslot_count) return false;

        let free_idx: usize? = null;
        let i: usize = 0;
        while (i < _.nodes.len) : (i += 1) {
            if (_.nodes[i].active) {
                if (_.nodes[i].node_id == node_id) return false;
            } else if (free_idx == null) {
                free_idx = i;
            }
        }

        const n_idx: usize = free_idx ?? return false;
        _.nodes[n_idx] = RingPhysicalNode{
            active: true,
            node_id: node_id,
            weight: weight,
            healthy: true,
            active_load: 0,
            max_load: max_load,
        };
        _.node_count += 1;

        // Insert each virtual slot into the sorted ring via binary search + shift
        let v: usize = 0;
        while (v < needed_vnodes) : (v += 1) {
            const slot = RingVirtualSlot{
                token: vnodeToken(node_id, v),
                node_id: node_id,
                node_idx: n_idx as u32,
            };

            let lo: usize = 0;
            let hi: usize = _.vslot_count;
            while (lo < hi) {
                const mid: usize = lo + (hi - lo) / 2;
                if (isSlotBefore(_.vslots[mid], slot)) {
                    lo = mid + 1;
                } else {
                    hi = mid;
                }
            }

            let k: usize = _.vslot_count;
            while (k > lo) : (k -= 1) {
                _.vslots[k] = _.vslots[k - 1];
            }
            _.vslots[lo] = slot;
            _.vslot_count += 1;
        }
        return true;
    }

    /// Removes a physical node and all of its virtual slots from the ring in-place, preserving sorted order.
    /// Returns `false` if `node_id` is not currently registered.
    fn removeNode(*_, node_id: u32): bool {
        let found_idx: usize? = null;
        let i: usize = 0;
        while (i < _.nodes.len) : (i += 1) {
            if (_.nodes[i].active && _.nodes[i].node_id == node_id) {
                found_idx = i;
                break;
            }
        }
        const n_idx: usize = found_idx ?? return false;

        _.nodes[n_idx] = RingPhysicalNode{};
        if (_.node_count > 0) _.node_count -= 1;

        let write_idx: usize = 0;
        let read_idx: usize = 0;
        while (read_idx < _.vslot_count) : (read_idx += 1) {
            if (_.vslots[read_idx].node_id != node_id) {
                _.vslots[write_idx] = _.vslots[read_idx];
                write_idx += 1;
            }
        }
        _.vslot_count = write_idx;
        return true;
    }

    /// Updates the health status of a registered physical node.
    fn setHealthy(*_, node_id: u32, healthy: bool): bool {
        let i: usize = 0;
        while (i < _.nodes.len) : (i += 1) {
            if (_.nodes[i].active && _.nodes[i].node_id == node_id) {
                _.nodes[i].healthy = healthy;
                return true;
            }
        }
        return false;
    }

    /// Updates the current active load of a registered physical node (used by Bounded-Load routing).
    fn setLoad(*_, node_id: u32, active_load: u32): bool {
        let i: usize = 0;
        while (i < _.nodes.len) : (i += 1) {
            if (_.nodes[i].active && _.nodes[i].node_id == node_id) {
                _.nodes[i].active_load = active_load;
                return true;
            }
        }
        return false;
    }

    /// Looks up a registered physical node by `node_id`.
    fn getNode(node_id: u32): RingPhysicalNode? {
        let i: usize = 0;
        while (i < _.nodes.len) : (i += 1) {
            if (_.nodes[i].active && _.nodes[i].node_id == node_id) {
                return _.nodes[i];
            }
        }
        return null;
    }

    /// O(log V) binary search returning the first ring index with `vslots[idx].token >= token`,
    /// wrapping around to `0` if `token` exceeds the largest slot token.
    fn lowerBoundIndex(token: u64): usize {
        let lo: usize = 0;
        let hi: usize = _.vslot_count;
        while (lo < hi) {
            const mid: usize = lo + (hi - lo) / 2;
            if (_.vslots[mid].token < token) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        if (lo == _.vslot_count) return 0;
        return lo;
    }

    /// Routes `key` in O(log V) to the first clockwise healthy, non-overloaded (`active_load < max_load`) node.
    /// Returns `null` if the ring is empty or all nodes are unhealthy/overloaded.
    fn routeKey(key: string): u32? {
        if (_.vslot_count == 0) return null;
        const h: u64 = hashRingKey(key);
        const start: usize = _.lowerBoundIndex(h);

        let step: usize = 0;
        while (step < _.vslot_count) : (step += 1) {
            let pos: usize = start + step;
            if (pos >= _.vslot_count) pos -= _.vslot_count;
            const slot: RingVirtualSlot = _.vslots[pos];
            const node: RingPhysicalNode = _.nodes[slot.node_idx as usize];
            if (node.isAvailable()) {
                return slot.node_id;
            }
        }
        return null;
    }

    /// Routes `key` clockwise to up to `min(replica_count, out_node_ids.len)` distinct physical nodes,
    /// automatically deduplicating multiple virtual nodes of the same physical host and skipping
    /// unhealthy or overloaded nodes. Returns the number of replicas written into `out_node_ids`.
    fn routeReplicas(key: string, replica_count: usize, out_node_ids: *[]u32): usize {
        if (_.vslot_count == 0 || replica_count == 0 || out_node_ids.len == 0) return 0;
        const target: usize = if (replica_count < out_node_ids.len) replica_count else out_node_ids.len;

        const h: u64 = hashRingKey(key);
        const start: usize = _.lowerBoundIndex(h);
        let found: usize = 0;

        let step: usize = 0;
        while (step < _.vslot_count && found < target) : (step += 1) {
            let pos: usize = start + step;
            if (pos >= _.vslot_count) pos -= _.vslot_count;
            const slot: RingVirtualSlot = _.vslots[pos];
            const node: RingPhysicalNode = _.nodes[slot.node_idx as usize];
            if (!node.isAvailable()) continue;

            // Deduplicate physical host ID against already-selected replicas
            let dup: bool = false;
            let k: usize = 0;
            while (k < found) : (k += 1) {
                if (out_node_ids[k] == slot.node_id) {
                    dup = true;
                    break;
                }
            }
            if (!dup) {
                out_node_ids[found] = slot.node_id;
                found += 1;
            }
        }
        return found;
    }

    /// Returns a point-in-time telemetry snapshot of the hash ring.
    fn stats(): RingStats {
        let st = RingStats{
            physical_nodes: _.node_count,
            healthy_nodes: 0,
            overloaded_nodes: 0,
            virtual_slots: _.vslot_count,
            virtual_capacity: _.vslots.len,
            total_weight: 0,
            total_load: 0,
        };

        let i: usize = 0;
        while (i < _.nodes.len) : (i += 1) {
            const n: RingPhysicalNode = _.nodes[i];
            if (!n.active) continue;
            st.total_weight += n.weight as u64;
            st.total_load += n.active_load as u64;
            if (n.healthy) {
                st.healthy_nodes += 1;
            }
            if (n.max_load > 0 && n.active_load >= n.max_load) {
                st.overloaded_nodes += 1;
            }
        }
        return st;
    }
}
