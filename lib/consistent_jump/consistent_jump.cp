// Source: cp/lib/consistent_jump/consistent_jump.cp (Pure CP Jump Consistent Hash & Rendezvous HRW)
const std = @import("std");

pub const JumpError = error{
    InvalidBucketCount,
    EmptyNodes,
};

/// Google Jump Consistent Hash: O(ln(num_buckets)) time, 0 bytes memory
pub fn jumpConsistentHash(key: u64, num_buckets: u32): JumpError!u32 {
    if (num_buckets == 0) { return JumpError.InvalidBucketCount; }
    let k = key;
    let b: i64 = -1;
    let j: i64 = 0;
    const n: i64 = @intCast(num_buckets);
    while (j < n) {
        b = j;
        k = k *% 2862933555777941757 +% 1;
        const shift_val: u64 = (k >> 33) + 1;
        const b_f: f64 = @floatFromInt(b + 1);
        const s_f: f64 = @floatFromInt(shift_val);
        const next_f = b_f * (@as(f64, 2147483648.0) / s_f);
        j = @intFromFloat(@floor(next_f));
    }
    return @intCast(b);
}

pub fn jumpConsistentHashBytes(key_bytes: []const u8, num_buckets: u32): JumpError!u32 {
    const h = std.hash.Wyhash.hash(0x9e3779b97f4a7c15, key_bytes);
    return jumpConsistentHash(h, num_buckets);
}

pub const WeightedNode = struct {
    id: []const u8,
    weight: f64,
};

fn scoreNode(key_bytes: []const u8, node_id: []const u8): u64 {
    let h = std.hash.Wyhash.init(0x517cc1b727220a95);
    h.update(key_bytes);
    h.update("\x00");
    h.update(node_id);
    return h.final();
}

/// Rendezvous / Highest Random Weight (HRW) stateless selection
pub fn hrwSelect(key_bytes: []const u8, nodes: []const []const u8): JumpError![]const u8 {
    if (nodes.len == 0) { return JumpError.EmptyNodes; }
    let best_node = nodes[0];
    let best_score = scoreNode(key_bytes, nodes[0]);
    let i: usize = 1;
    while (i < nodes.len) : (i += 1) {
        const s = scoreNode(key_bytes, nodes[i]);
        if (s > best_score) {
            best_score = s;
            best_node = nodes[i];
        }
    }
    return best_node;
}

/// Weighted Rendezvous HRW selection: score = -ln(u) / weight
pub fn hrwSelectWeighted(key_bytes: []const u8, nodes: []const WeightedNode): JumpError![]const u8 {
    if (nodes.len == 0) { return JumpError.EmptyNodes; }
    let best_idx: usize = 0;
    let best_score: f64 = -std.math.inf(f64);
    let i: usize = 0;
    while (i < nodes.len) : (i += 1) {
        if (nodes[i].weight <= 0.0) { continue; }
        const h = scoreNode(key_bytes, nodes[i].id);
        // Map u64 to (0, 1]
        const u: f64 = (@as(f64, @floatFromInt(h >> 11)) + 1.0) / 9007199254740992.0;
        const s = nodes[i].weight / (-@log(u));
        if (s > best_score) {
            best_score = s;
            best_idx = i;
        }
    }
    return nodes[best_idx].id;
}
