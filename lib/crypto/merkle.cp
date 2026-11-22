// Source: cp/lib/merkle/merkle.cp (RFC 6962 Merkle Hash Tree Engine)
// Pure CP zero-C-dependency RFC 6962 Merkle Hash Tree engine with strict domain separation
// (0x00 leaf prefix, 0x01 internal node prefix) to prevent second-preimage attacks,
// O(log N) inclusion proof generation & stateless verification, and O(k log N) dual-tree diffing.

const std = @import("std");
import { sha256Raw } from "./hash.cp";

pub err MerkleError {
    EmptyTree,
    IndexOutOfBounds,
    InvalidProof,
    OutOfMemory,
}

pub enum ProofSide {
    Left,
    Right,
}

pub struct ProofStep {
    sibling_hash: [32]u8;
    sibling_side: ProofSide;
}

pub struct MerkleProof {
    leaf_index: usize;
    leaf_count: usize;
    leaf_hash: [32]u8;
    steps: []ProofStep;
    alloc: std.mem.Allocator;

    pub fn deinit(*_) {
        if (_.steps.len > 0) {
            _.alloc.free(_.steps);
            _.steps = [];
        }
    }
}

pub fn hashLeaf(data: []const u8): [32]u8 {
    let out: [32]u8 = undefined;
    let h = std.crypto.hash.sha2.Sha256.init(.{});
    const prefix: [1]u8 = [0x00];
    h.update(&prefix);
    h.update(data);
    h.final(&out);
    return out;
}

pub fn hashNode(left: [32]u8, right: [32]u8): [32]u8 {
    let buf: [65]u8 = undefined;
    buf[0] = 0x01;
    @memcpy(buf[1..33], &left);
    @memcpy(buf[33..65], &right);
    let out: [32]u8 = undefined;
    sha256Raw(&out, buf[0..65]);
    return out;
}

fn diffRec(
    alloc: std.mem.Allocator,
    a: &MerkleTree,
    b: &MerkleTree,
    level: usize,
    idx: usize,
    out: *std.ArrayListUnmanaged(usize),
): ! MerkleError {
    const shift: u6 = if (level >= 63) 63 else (level as u6);
    const start: usize = idx << shift;
    const span: usize = (1 as usize) << shift;
    const end: usize = start +| span;

    if (start >= a.leaf_count && start >= b.leaf_count) {
        return;
    }
    if (start >= a.leaf_count) {
        const limit = @min(end, b.leaf_count);
        for (k of start..limit) {
            out.append(alloc, k) catch return MerkleError.OutOfMemory;
        }
        return;
    }
    if (start >= b.leaf_count) {
        const limit = @min(end, a.leaf_count);
        for (k of start..limit) {
            out.append(alloc, k) catch return MerkleError.OutOfMemory;
        }
        return;
    }

    if (level == 0) {
        if (!std.mem.eql(u8, &a.levels[0][idx], &b.levels[0][idx])) {
            out.append(alloc, idx) catch return MerkleError.OutOfMemory;
        }
        return;
    }

    if (level < a.levels.len && level < b.levels.len && idx < a.levels[level].len && idx < b.levels[level].len) {
        if (@min(end, a.leaf_count) == @min(end, b.leaf_count)) {
            if (std.mem.eql(u8, &a.levels[level][idx], &b.levels[level][idx])) {
                return;
            }
        }
    }

    diffRec(alloc, a, b, level - 1, idx * 2, out)?;
    diffRec(alloc, a, b, level - 1, idx * 2 + 1, out)?;
}

pub class MerkleTree {
    alloc: std.mem.Allocator;
    levels: [][][32]u8;
    leaf_count: usize;
    root_hash: [32]u8;

    pub static fn build(alloc: std.mem.Allocator, items: []const []const u8): MerkleTree ! MerkleError {
        if (items.len == 0) {
            return MerkleError.EmptyTree;
        }

        let num_levels: usize = 1;
        let n: usize = items.len;
        while (n > 1) {
            n = (n + 1) / 2;
            num_levels += 1;
        }

        let levels = alloc.alloc([][32]u8, num_levels) catch return MerkleError.OutOfMemory;
        let built_levels: usize = 0;
        errdefer {
            for (i of 0..built_levels) {
                alloc.free(levels[i]);
            }
            alloc.free(levels);
        }

        let level0 = alloc.alloc([32]u8, items.len) catch return MerkleError.OutOfMemory;
        levels[0] = level0;
        built_levels = 1;

        for (i of 0..items.len) {
            level0[i] = hashLeaf(items[i]);
        }

        for (lvl of 1..num_levels) {
            const prev = levels[lvl - 1];
            const next_len = (prev.len + 1) / 2;
            let next = alloc.alloc([32]u8, next_len) catch return MerkleError.OutOfMemory;
            levels[lvl] = next;
            built_levels += 1;

            for (j of 0..next_len) {
                const left_idx = j * 2;
                if (left_idx + 1 < prev.len) {
                    next[j] = hashNode(prev[left_idx], prev[left_idx + 1]);
                } else {
                    next[j] = prev[left_idx];
                }
            }
        }

        const root = levels[num_levels - 1][0];
        return MerkleTree{
            alloc: alloc,
            levels: levels,
            leaf_count: items.len,
            root_hash: root,
        };
    }

    pub fn deinit(*_) {
        for (lvl of _.levels) {
            _.alloc.free(lvl);
        }
        if (_.levels.len > 0) {
            _.alloc.free(_.levels);
            _.levels = [];
        }
    }

    pub fn rootHash(): [32]u8 {
        return _.root_hash;
    }

    pub fn rootHex(): [64]u8 {
        return std.fmt.bytesToHex(_.root_hash, .lower);
    }

    pub fn leafCount(): usize {
        return _.leaf_count;
    }

    pub fn generateProof(alloc: std.mem.Allocator, leaf_index: usize): MerkleProof ! MerkleError {
        if (leaf_index >= _.leaf_count) {
            return MerkleError.IndexOutOfBounds;
        }

        let steps: std.ArrayListUnmanaged(ProofStep) = .empty;
        errdefer steps.deinit(alloc);

        let idx = leaf_index;
        for (lvl of 0..(_.levels.len - 1)) {
            const layer = _.levels[lvl];
            if (idx % 2 == 0) {
                if (idx + 1 < layer.len) {
                    steps.append(alloc, ProofStep{
                        sibling_hash: layer[idx + 1],
                        sibling_side: ProofSide.Right,
                    }) catch return MerkleError.OutOfMemory;
                }
            } else {
                steps.append(alloc, ProofStep{
                    sibling_hash: layer[idx - 1],
                    sibling_side: ProofSide.Left,
                }) catch return MerkleError.OutOfMemory;
            }
            idx = idx / 2;
        }

        const owned_steps = steps.toOwnedSlice(alloc) catch return MerkleError.OutOfMemory;
        return MerkleProof{
            leaf_index: leaf_index,
            leaf_count: _.leaf_count,
            leaf_hash: _.levels[0][leaf_index],
            steps: owned_steps,
            alloc: alloc,
        };
    }

    pub static fn verifyProof(root_hash: [32]u8, leaf_data: []const u8, proof: &MerkleProof): bool {
        if (proof.leaf_count == 0 || proof.leaf_index >= proof.leaf_count) {
            return false;
        }

        let cur = hashLeaf(leaf_data);
        if (!std.mem.eql(u8, &cur, &proof.leaf_hash)) {
            return false;
        }

        let idx = proof.leaf_index;
        let layer_len = proof.leaf_count;
        let step_idx: usize = 0;

        while (layer_len > 1) {
            if (idx % 2 == 0) {
                if (idx + 1 < layer_len) {
                    if (step_idx >= proof.steps.len) return false;
                    const step = proof.steps[step_idx];
                    if (step.sibling_side != ProofSide.Right) return false;
                    cur = hashNode(cur, step.sibling_hash);
                    step_idx += 1;
                }
            } else {
                if (step_idx >= proof.steps.len) return false;
                const step = proof.steps[step_idx];
                if (step.sibling_side != ProofSide.Left) return false;
                cur = hashNode(step.sibling_hash, cur);
                step_idx += 1;
            }
            idx = idx / 2;
            layer_len = (layer_len + 1) / 2;
        }

        if (step_idx != proof.steps.len) {
            return false;
        }
        return std.mem.eql(u8, &cur, &root_hash);
    }

    pub static fn diffTrees(alloc: std.mem.Allocator, a: &MerkleTree, b: &MerkleTree): []usize ! MerkleError {
        let out: std.ArrayListUnmanaged(usize) = .empty;
        errdefer out.deinit(alloc);

        const max_levels = @max(a.levels.len, b.levels.len);
        if (max_levels == 0) {
            return out.toOwnedSlice(alloc) catch return MerkleError.OutOfMemory;
        }

        diffRec(alloc, a, b, max_levels - 1, 0, &out)?;
        return out.toOwnedSlice(alloc) catch return MerkleError.OutOfMemory;
    }
}

pub fn verifyProof(root_hash: [32]u8, leaf_data: []const u8, proof: &MerkleProof): bool {
    return MerkleTree.verifyProof(root_hash, leaf_data, proof);
}

pub fn diffTrees(alloc: std.mem.Allocator, a: &MerkleTree, b: &MerkleTree): []usize ! MerkleError {
    return MerkleTree.diffTrees(alloc, a, b);
}
