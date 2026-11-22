// Source: cp/lib/cluster/hash_ring.cp (Virtual-Node Consistent Hashing Ring)
// Virtual-node consistent hashing ring using FNV-1a 64 + SplitMix64 avalanche hashing
// and O(log(N*V)) binary search for deterministic AI Agent shard routing and replica placement.

const std = @import("std");

/// Error set shared across `cap:cluster` distributed coordination primitives.
pub err ClusterErr {
    OutOfMemory,
    InvalidParameter,
    DuplicateNode,
    NodeNotFound,
}

/// A single virtual node entry placed on the 64-bit consistent hash ring.
pub struct RingVNode {
    hash: u64 = 0,
    node_id: string = "",
}

zig {
    const FNV1A_64_OFFSET: u64 = 14695981039346656037;
    const FNV1A_64_PRIME: u64 = 1099511628211;
    const GOLDEN_64: u64 = 0x9E3779B97F4A7C15;
    const MIX_MUL1: u64 = 0xBF58476D1CE4E5B9;
    const MIX_MUL2: u64 = 0x94D049BB133111EB;
}

fn fnv1a64(data: []const u8): u64 {
    let h: u64 = 0;
    zig {
        h = FNV1A_64_OFFSET;
        for (data) |b| {
            h ^= @as(u64, b);
            h *%= FNV1A_64_PRIME;
        }
    }
    return h;
}

fn splitMix64(x_in: u64): u64 {
    let res: u64 = 0;
    zig {
        var z: u64 = x_in +% GOLDEN_64;
        z = (z ^ (z >> 30)) *% MIX_MUL1;
        z = (z ^ (z >> 27)) *% MIX_MUL2;
        res = z ^ (z >> 31);
    }
    return res;
}

fn hashKey64(key: []const u8): u64 {
    return splitMix64(fnv1a64(key));
}

fn hashVNode64(node_id: []const u8, vnode_idx: usize): u64 {
    let h: u64 = 0;
    zig {
        h = FNV1A_64_OFFSET;
        for (node_id) |b| {
            h ^= @as(u64, b);
            h *%= FNV1A_64_PRIME;
        }
        // Domain separator between node_id and 64-bit virtual node index
        h ^= 0xFF;
        h *%= FNV1A_64_PRIME;
        var v: u64 = @as(u64, vnode_idx);
        var i: usize = 0;
        while (i < 8) : (i += 1) {
            h ^= (v & 0xFF);
            h *%= FNV1A_64_PRIME;
            v >>= 8;
        }
    }
    return splitMix64(h);
}

fn vnodeLessThan(a: RingVNode, b: RingVNode): bool {
    if (a.hash != b.hash) return a.hash < b.hash;
    return std.mem.order(u8, a.node_id, b.node_id) == .lt;
}

fn sortVNodes(items: []RingVNode) {
    zig {
        std.sort.pdq(RingVNode, items, {}, struct {
            fn lessThan(_: void, a: RingVNode, b: RingVNode) bool {
                return vnodeLessThan(a, b);
            }
        }.lessThan);
    }
}

/// Consistent hashing ring with virtual nodes, ownership-safe node IDs, and replica deduplication.
pub struct ConsistentHashRing {
    allocator: std.mem.Allocator = undefined,
    vnodes: std.ArrayList(RingVNode) = .empty,
    nodes: std.StringHashMapUnmanaged(usize) = .empty,

    /// Initializes an empty `ConsistentHashRing` backed by `alloc`.
    static fn init(alloc: std.mem.Allocator): ConsistentHashRing {
        return ConsistentHashRing{
            allocator: alloc,
            vnodes: .empty,
            nodes: .empty,
        };
    }

    /// Frees all owned node identifiers, virtual-node storage, and internal maps.
    fn deinit(*_) {
        let iter = _.nodes.iterator();
        while (entry := iter.next()) {
            _.allocator.free(entry.key_ptr.*);
        }
        _.nodes.deinit(_.allocator);
        _.vnodes.deinit(_.allocator);
    }

    /// Returns the number of distinct physical nodes registered in the ring.
    fn nodeCount(): usize {
        return _.nodes.count();
    }

    /// Returns the total number of virtual nodes currently placed on the ring.
    fn vnodeCount(): usize {
        return _.vnodes.items.len;
    }

    /// Returns `true` if `node_id` is currently registered in the ring.
    fn hasNode(node_id: string): bool {
        return _.nodes.contains(node_id);
    }

    /// Registers a physical node with `virtual_nodes` replicas on the ring.
    /// Rejects empty `node_id`, zero `virtual_nodes`, or duplicate `node_id`.
    fn addNode(*_, node_id: string, virtual_nodes: usize): ! {
        if (node_id.len == 0 || virtual_nodes == 0) {
            return ClusterErr.InvalidParameter;
        }
        if (_.nodes.contains(node_id)) {
            return ClusterErr.DuplicateNode;
        }

        const owned_id = _.allocator.dupe(u8, node_id) catch return ClusterErr.OutOfMemory;
        errdefer _.allocator.free(owned_id);

        const prev_len = _.vnodes.items.len;
        _.vnodes.ensureTotalCapacity(_.allocator, prev_len + virtual_nodes) catch return ClusterErr.OutOfMemory;

        _.nodes.put(_.allocator, owned_id, virtual_nodes) catch return ClusterErr.OutOfMemory;

        let i: usize = 0;
        while (i < virtual_nodes) : (i += 1) {
            const h = hashVNode64(owned_id, i);
            _.vnodes.appendAssumeCapacity(RingVNode{
                hash: h,
                node_id: owned_id,
            });
        }

        sortVNodes(_.vnodes.items);
    }

    /// Removes `node_id` and all its virtual nodes from the ring.
    /// Returns `true` if the node existed and was removed, `false` otherwise.
    fn removeNode(*_, node_id: string): bool {
        if (!_.nodes.contains(node_id)) {
            return false;
        }

        // Compact vnodes in-place while preserving sorted order
        let write_idx: usize = 0;
        let read_idx: usize = 0;
        while (read_idx < _.vnodes.items.len) : (read_idx += 1) {
            const vn = _.vnodes.items[read_idx];
            if (!std.mem.eql(u8, vn.node_id, node_id)) {
                _.vnodes.items[write_idx] = vn;
                write_idx += 1;
            }
        }
        _.vnodes.items.len = write_idx;

        if (kv := _.nodes.fetchRemove(node_id)) {
            _.allocator.free(kv.key);
        }
        return true;
    }

    fn lowerBound(target_hash: u64): usize {
        let low: usize = 0;
        let high: usize = _.vnodes.items.len;
        while (low < high) {
            const mid = low + (high - low) / 2;
            if (_.vnodes.items[mid].hash < target_hash) {
                low = mid + 1;
            } else {
                high = mid;
            }
        }
        if (low >= _.vnodes.items.len) return 0;
        return low;
    }

    /// Routes `key` to its primary owner node in O(log(N*V)) time.
    /// Returns `null` when the ring is empty.
    fn getNode(key: string): string? {
        if (_.vnodes.items.len == 0) return null;
        const h = hashKey64(key);
        const idx = _.lowerBound(h);
        return _.vnodes.items[idx].node_id;
    }

    /// Walks clockwise from `key`'s hash position and returns up to `replication_factor`
    /// distinct physical nodes. Caller owns the returned slice and must free it with `alloc.free`.
    fn getReplicaNodes(alloc: std.mem.Allocator, key: string, replication_factor: usize): ([]string)! {
        const physical_count = _.nodes.count();
        if (physical_count == 0 || _.vnodes.items.len == 0 || replication_factor == 0) {
            return alloc.alloc(string, 0) catch return ClusterErr.OutOfMemory;
        }

        const target = if (replication_factor < physical_count) replication_factor else physical_count;
        let out = alloc.alloc(string, target) catch return ClusterErr.OutOfMemory;
        errdefer alloc.free(out);

        const h = hashKey64(key);
        const total = _.vnodes.items.len;
        const start_idx = _.lowerBound(h);

        let found: usize = 0;
        let step: usize = 0;
        while (step < total && found < target) : (step += 1) {
            const idx = (start_idx + step) % total;
            const cand = _.vnodes.items[idx].node_id;

            let seen = false;
            let j: usize = 0;
            while (j < found) : (j += 1) {
                if (std.mem.eql(u8, out[j], cand)) {
                    seen = true;
                    break;
                }
            }
            if (!seen) {
                out[found] = cand;
                found += 1;
            }
        }

        if (found < target) {
            return alloc.realloc(out, found) catch return ClusterErr.OutOfMemory;
        }
        return out;
    }
}
