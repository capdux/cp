// Source: native-cp-stdlib (Cryptographic SHA-256 Merkle Tree)

const std = @import("std");

const LEAF_PREFIX: [1]u8 = [0x00];
const INTERNAL_PREFIX: [1]u8 = [0x01];

export struct ProofStep {
    hash: [32]u8,
    is_left: bool,
}

export struct MerkleProof {
    leaf_index: usize,
    leaf_count: usize,
    steps: []ProofStep,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.steps.len > 0) {
            alloc.free(_.steps);
        }
        _.steps = [];
    }
}

export fn hashLeaf(data: []const u8): [32]u8 {
    let hasher = std.crypto.hash.sha2.Sha256.init(.{});
    hasher.update(&LEAF_PREFIX);
    hasher.update(data);
    let out: [32]u8 = undefined;
    hasher.final(&out);
    return out;
}

export fn hashInternal(left: [32]u8, right: [32]u8): [32]u8 {
    let hasher = std.crypto.hash.sha2.Sha256.init(.{});
    hasher.update(&INTERNAL_PREFIX);
    hasher.update(&left);
    hasher.update(&right);
    let out: [32]u8 = undefined;
    hasher.final(&out);
    return out;
}

export struct MerkleTree {
    alloc: std.mem.Allocator,
    levels: [][][32]u8,
    count: usize,

    pub fn init(alloc: std.mem.Allocator, leaves: []const []const u8): !MerkleTree {
        if (leaves.len == 0) {
            return MerkleTree{
                .alloc = alloc,
                .levels = [],
                .count = 0,
            };
        }

        let level_list = std.ArrayList([][32]u8).empty;
        errdefer {
            for (lvl of level_list.items) {
                alloc.free(lvl);
            }
            level_list.deinit(alloc);
        }

        let base_level = try alloc.alloc([32]u8, leaves.len);
        errdefer alloc.free(base_level);
        let leaf_idx: usize = 0;
        while (leaf_idx < leaves.len) {
            base_level[leaf_idx] = hashLeaf(leaves[leaf_idx]);
            leaf_idx = leaf_idx + 1;
        }
        try level_list.append(alloc, base_level);

        while (level_list.items[level_list.items.len - 1].len > 1) {
            const current = level_list.items[level_list.items.len - 1];
            const next_len = (current.len + 1) / 2;
            let next_level = try alloc.alloc([32]u8, next_len);
            errdefer alloc.free(next_level);

            let i: usize = 0;
            let out_idx: usize = 0;
            while (i < current.len) {
                const left = current[i];
                const right = if (i + 1 < current.len) current[i + 1] else current[i];
                next_level[out_idx] = hashInternal(left, right);
                i = i + 2;
                out_idx = out_idx + 1;
            }
            try level_list.append(alloc, next_level);
        }

        const owned_levels = try level_list.toOwnedSlice(alloc);
        return MerkleTree{
            .alloc = alloc,
            .levels = owned_levels,
            .count = leaves.len,
        };
    }

    pub fn deinit(*_) {
        if (_.levels.len > 0) {
            for (lvl of _.levels) {
                _.alloc.free(lvl);
            }
            _.alloc.free(_.levels);
        }
        _.levels = [];
        _.count = 0;
    }

    pub fn leafCount(): usize {
        return _.count;
    }

    pub fn rootHash(): [32]u8 {
        if (_.levels.len == 0) {
            return std.mem.zeroes([32]u8);
        }
        const top = _.levels[_.levels.len - 1];
        return top[0];
    }

    pub fn generateProof(alloc: std.mem.Allocator, index: usize): !MerkleProof {
        if (_.count == 0 or index >= _.count) {
            return error.IndexOutOfBounds;
        }

        let steps_list = std.ArrayList(ProofStep).empty;
        errdefer steps_list.deinit(alloc);

        let idx = index;
        let lvl_idx: usize = 0;
        while (lvl_idx + 1 < _.levels.len) {
            const current = _.levels[lvl_idx];
            if (idx % 2 == 0) {
                const sib_idx = if (idx + 1 < current.len) idx + 1 else idx;
                try steps_list.append(alloc, ProofStep{
                    .hash = current[sib_idx],
                    .is_left = false,
                });
            } else {
                const sib_idx = idx - 1;
                try steps_list.append(alloc, ProofStep{
                    .hash = current[sib_idx],
                    .is_left = true,
                });
            }
            idx = idx / 2;
            lvl_idx = lvl_idx + 1;
        }

        const owned_steps = try steps_list.toOwnedSlice(alloc);
        return MerkleProof{
            .leaf_index = index,
            .leaf_count = _.count,
            .steps = owned_steps,
        };
    }
}

export fn verifyProof(root_hash: [32]u8, leaf_data: []const u8, proof: MerkleProof): bool {
    if (proof.leaf_count == 0 or proof.leaf_index >= proof.leaf_count) {
        return false;
    }

    let current = hashLeaf(leaf_data);
    let idx = proof.leaf_index;

    for (step of proof.steps) {
        const expected_is_left = (idx % 2) == 1;
        if (step.is_left != expected_is_left) {
            return false;
        }
        if (step.is_left) {
            current = hashInternal(step.hash, current);
        } else {
            current = hashInternal(current, step.hash);
        }
        idx = idx / 2;
    }

    if (idx != 0) {
        return false;
    }

    return std.mem.eql(u8, &current, &root_hash);
}
