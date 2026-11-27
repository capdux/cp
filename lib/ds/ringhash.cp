// Source: cp/lib/ringhash/ringhash.cp (Consistent Hash Ring, Jump Consistent Hash & Rendezvous HRW)
// Pure CP zero-C-dependency consistent hashing primitives:
//   - ConsistentRing: Virtual-node binary-search consistent hash ring with weighted vnodes & replica deduplication
//   - jumpConsistentHash: Google Lamping-Veach O(ln N) stateless consistent hash
//   - RendezvousHasher: Highest Random Weight (HRW) weighted consistent hasher

const std = @import("std");

/// Error set for consistent hashing operations.
pub err RingHashError {
    EmptyRing,
    InvalidWeight,
    InvalidBucketCount,
    DuplicateNode,
    NodeNotFound,
    OutOfMemory,
}

fn splitMix64(x: u64): u64 {
    let z = x +% 0x9e3779b97f4a7c15;
    z = (z ^ (z >> 30)) *% 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) *% 0x94d049bb133111eb;
    return z ^ (z >> 31);
}

fn hashKey(key: []const u8): u64 {
    return std.hash.Wyhash.hash(0, key);
}

fn hashVNode(node_id: []const u8, replica_idx: usize): u64 {
    return std.hash.Wyhash.hash(splitMix64(replica_idx as u64), node_id);
}

fn hashRendezvous(node_id: []const u8, key: []const u8): u64 {
    let hasher = std.hash.Wyhash.init(0x517cc1b727220a95);
    hasher.update(node_id);
    hasher.update("\x00");
    hasher.update(key);
    return hasher.final();
}

struct PhysicalNode {
    id: []u8,
    weight: usize,
}

struct VNode {
    hash: u64,
    node_id: []const u8,
}

fn vnodeLessThan(_: void, a: VNode, b: VNode): bool {
    if (a.hash != b.hash) {
        return a.hash < b.hash;
    }
    return std.mem.order(u8, a.node_id, b.node_id) == .lt;
}

/// Virtual-node binary-search consistent hash ring.
/// Owns copies of all registered `node_id` strings; returned slices remain valid
/// until the corresponding node is removed or `deinit()` is called.
pub struct ConsistentRing {
    alloc: std.mem.Allocator,
    default_vnodes: usize,
    nodes: std.ArrayListUnmanaged(PhysicalNode),
    vnodes: std.ArrayListUnmanaged(VNode),

    /// Initialize an empty consistent hash ring with `default_vnodes` virtual nodes per unit weight.
    pub static fn init(allocator: std.mem.Allocator, default_vnodes: usize): ConsistentRing ! RingHashError {
        if (default_vnodes == 0) {
            return RingHashError.InvalidWeight;
        }
        return ConsistentRing{
            alloc: allocator,
            default_vnodes: default_vnodes,
            nodes: .empty,
            vnodes: .empty,
        };
    }

    /// Release all owned node identifiers and internal ring buffers.
    pub fn deinit(*_) {
        for (n of _.nodes.items) {
            _.alloc.free(n.id);
        }
        _.nodes.deinit(_.alloc);
        _.vnodes.deinit(_.alloc);
    }

    fn findNodeIndex(node_id: []const u8): usize? {
        let i: usize = 0;
        while (i < _.nodes.items.len) : (i += 1) {
            if (std.mem.eql(u8, _.nodes.items[i].id, node_id)) {
                return i;
            }
        }
        return null;
    }

    /// Register a physical node with `weight * default_vnodes` virtual nodes on the ring.
    pub fn addNode(*_, node_id: []const u8, weight: usize): void ! RingHashError {
        if (weight == 0) {
            return RingHashError.InvalidWeight;
        }
        if (_.findNodeIndex(node_id) != null) {
            return RingHashError.DuplicateNode;
        }
        const total_vnodes = std.math.mul(usize, weight, _.default_vnodes) catch return RingHashError.InvalidWeight;

        const owned_id = _.alloc.dupe(u8, node_id) catch return RingHashError.OutOfMemory;
        errdefer _.alloc.free(owned_id);

        _.nodes.ensureUnusedCapacity(_.alloc, 1) catch return RingHashError.OutOfMemory;
        _.vnodes.ensureUnusedCapacity(_.alloc, total_vnodes) catch return RingHashError.OutOfMemory;

        _.nodes.appendAssumeCapacity(PhysicalNode{
            id: owned_id,
            weight: weight,
        });

        let r: usize = 0;
        while (r < total_vnodes) : (r += 1) {
            _.vnodes.appendAssumeCapacity(VNode{
                hash: hashVNode(owned_id, r),
                node_id: owned_id,
            });
        }

        std.sort.pdq(VNode, _.vnodes.items, {}, vnodeLessThan);
    }

    /// Remove a physical node and all of its virtual nodes from the ring.
    pub fn removeNode(*_, node_id: []const u8): void ! RingHashError {
        const idx = _.findNodeIndex(node_id) orelse return RingHashError.NodeNotFound;
        const removed = _.nodes.swapRemove(idx);
        defer _.alloc.free(removed.id);

        // Compact vnodes in-place; preserves existing sorted hash order.
        let write_idx: usize = 0;
        let read_idx: usize = 0;
        while (read_idx < _.vnodes.items.len) : (read_idx += 1) {
            const v = _.vnodes.items[read_idx];
            if (v.node_id.ptr != removed.id.ptr) {
                _.vnodes.items[write_idx] = v;
                write_idx += 1;
            }
        }
        _.vnodes.items.len = write_idx;
    }

    fn lowerBound(h: u64): usize {
        let lo: usize = 0;
        let hi: usize = _.vnodes.items.len;
        while (lo < hi) {
            const mid = lo + (hi - lo) / 2;
            if (_.vnodes.items[mid].hash < h) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        if (lo >= _.vnodes.items.len) {
            return 0;
        }
        return lo;
    }

    /// Clockwise binary-search lookup for the nearest physical node owning `key`.
    pub fn getNode(key: []const u8): []const u8 ! RingHashError {
        if (_.vnodes.items.len == 0) {
            return RingHashError.EmptyRing;
        }
        const idx = _.lowerBound(hashKey(key));
        return _.vnodes.items[idx].node_id;
    }

    /// Clockwise lookup returning up to `min(count, nodeCount())` distinct physical nodes.
    /// Caller owns the returned outer slice (`allocator.free(replicas)`).
    pub fn getReplicas(allocator: std.mem.Allocator, key: []const u8, count: usize): [][]const u8 ! RingHashError {
        if (_.vnodes.items.len == 0 || _.nodes.items.len == 0) {
            return RingHashError.EmptyRing;
        }
        const target = @min(count, _.nodes.items.len);
        let res = allocator.alloc(string, target) catch return RingHashError.OutOfMemory;
        if (target == 0) {
            return res;
        }

        const start = _.lowerBound(hashKey(key));
        const total = _.vnodes.items.len;
        let found: usize = 0;
        let step: usize = 0;
        while (step < total && found < target) : (step += 1) {
            const cur = (start + step) % total;
            const candidate = _.vnodes.items[cur].node_id;

            let seen = false;
            let k: usize = 0;
            while (k < found) : (k += 1) {
                if (res[k].ptr == candidate.ptr) {
                    seen = true;
                    break;
                }
            }
            if (!seen) {
                res[found] = candidate;
                found += 1;
            }
        }
        return res;
    }

    /// Number of registered physical nodes.
    pub fn nodeCount(): usize {
        return _.nodes.items.len;
    }

    /// Total number of virtual nodes currently on the ring.
    pub fn vnodeCount(): usize {
        return _.vnodes.items.len;
    }
}

/// Google Lamping-Veach O(ln N) stateless consistent hash.
/// Maps `key_hash` uniformly to `[0, num_buckets)` with minimal monotonic key migration.
pub fn jumpConsistentHash(key_hash: u64, num_buckets: usize): usize ! RingHashError {
    if (num_buckets == 0 || num_buckets > (std.math.maxInt(i64) as usize)) {
        return RingHashError.InvalidBucketCount;
    }
    let k: u64 = key_hash;
    let b: i64 = -1;
    let j: i64 = 0;
    const n: i64 = num_buckets as i64;
    while (j < n) {
        b = j;
        k = k *% 2862933555777941757 +% 1;
        const denom: f64 = ((k >> 33) + 1) as f64;
        j = (((b + 1) as f64) * (2147483648.0 / denom)) as i64;
    }
    return b as usize;
}

struct ScoredNode {
    score: f64,
    node_id: []const u8,
}

fn scoredNodeDesc(_: void, a: ScoredNode, b: ScoredNode): bool {
    if (a.score != b.score) {
        return a.score > b.score;
    }
    return std.mem.order(u8, a.node_id, b.node_id) == .lt;
}

fn computeHrwScore(node_id: []const u8, weight: usize, key: []const u8): f64 {
    const h = hashRendezvous(node_id, key);
    // Map top 53 bits to strictly open interval (0, 1) so -ln(u) > 0 is always finite.
    const u = (((h >> 11) as f64) + 0.5) / 9007199254740992.0;
    return (weight as f64) / (-@log(u));
}

/// Highest Random Weight (HRW / Rendezvous) consistent hasher with weighted node support.
/// Owns copies of all registered `node_id` strings.
pub struct RendezvousHasher {
    alloc: std.mem.Allocator,
    nodes: std.ArrayListUnmanaged(PhysicalNode),

    /// Initialize an empty RendezvousHasher.
    pub static fn init(allocator: std.mem.Allocator): RendezvousHasher {
        return RendezvousHasher{
            alloc: allocator,
            nodes: .empty,
        };
    }

    /// Release all owned node identifiers and internal node list.
    pub fn deinit(*_) {
        for (n of _.nodes.items) {
            _.alloc.free(n.id);
        }
        _.nodes.deinit(_.alloc);
    }

    fn findNodeIndex(node_id: []const u8): usize? {
        let i: usize = 0;
        while (i < _.nodes.items.len) : (i += 1) {
            if (std.mem.eql(u8, _.nodes.items[i].id, node_id)) {
                return i;
            }
        }
        return null;
    }

    /// Register a physical node with positive `weight`.
    pub fn addNode(*_, node_id: []const u8, weight: usize): void ! RingHashError {
        if (weight == 0) {
            return RingHashError.InvalidWeight;
        }
        if (_.findNodeIndex(node_id) != null) {
            return RingHashError.DuplicateNode;
        }
        const owned_id = _.alloc.dupe(u8, node_id) catch return RingHashError.OutOfMemory;
        errdefer _.alloc.free(owned_id);

        _.nodes.append(_.alloc, PhysicalNode{
            id: owned_id,
            weight: weight,
        }) catch return RingHashError.OutOfMemory;
    }

    /// Remove a physical node from the hasher.
    pub fn removeNode(*_, node_id: []const u8): void ! RingHashError {
        const idx = _.findNodeIndex(node_id) orelse return RingHashError.NodeNotFound;
        const removed = _.nodes.swapRemove(idx);
        _.alloc.free(removed.id);
    }

    /// Select the highest-weighted-random-score node for `key`.
    pub fn getNode(key: []const u8): []const u8 ! RingHashError {
        if (_.nodes.items.len == 0) {
            return RingHashError.EmptyRing;
        }
        let best_id: []const u8 = _.nodes.items[0].id;
        let best_score: f64 = computeHrwScore(best_id, _.nodes.items[0].weight, key);

        let i: usize = 1;
        while (i < _.nodes.items.len) : (i += 1) {
            const n = _.nodes.items[i];
            const s = computeHrwScore(n.id, n.weight, key);
            if (s > best_score || (s == best_score && std.mem.order(u8, n.id, best_id) == .lt)) {
                best_score = s;
                best_id = n.id;
            }
        }
        return best_id;
    }

    /// Select up to `min(count, nodeCount())` distinct physical nodes ordered by descending HRW score.
    /// Caller owns the returned outer slice (`allocator.free(replicas)`).
    pub fn getReplicas(allocator: std.mem.Allocator, key: []const u8, count: usize): [][]const u8 ! RingHashError {
        if (_.nodes.items.len == 0) {
            return RingHashError.EmptyRing;
        }
        const target = @min(count, _.nodes.items.len);
        let res = allocator.alloc(string, target) catch return RingHashError.OutOfMemory;
        if (target == 0) {
            return res;
        }
        errdefer allocator.free(res);

        let scored = allocator.alloc(ScoredNode, _.nodes.items.len) catch return RingHashError.OutOfMemory;
        defer allocator.free(scored);

        let i: usize = 0;
        while (i < _.nodes.items.len) : (i += 1) {
            const n = _.nodes.items[i];
            scored[i] = ScoredNode{
                score: computeHrwScore(n.id, n.weight, key),
                node_id: n.id,
            };
        }

        std.sort.pdq(ScoredNode, scored, {}, scoredNodeDesc);

        let k: usize = 0;
        while (k < target) : (k += 1) {
            res[k] = scored[k].node_id;
        }
        return res;
    }

    /// Number of registered physical nodes.
    pub fn nodeCount(): usize {
        return _.nodes.items.len;
    }
}
