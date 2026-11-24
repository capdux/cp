// Reference-hardened against RustCrypto & GF(256) Shamir / SMT (MIT OR Apache-2.0)
// Origin: Pure-CP Stdlib (Zero-Alloc Fixed-Depth Sparse Merkle Tree with Inclusion & Non-Inclusion Proofs + 16-Lane SIMD) | Created: 2026-11-22
// 100% pure CP zero-heap-allocation, zero-C-FFI Sparse Merkle Tree (SMT) engine powered by
// `std.crypto.hash.sha2.Sha256` with strict domain separation (`0x00` leaf / `0x01` internal node):
//   - Configurable tree depth `depth in 1..=32` with precomputed empty-subtree default hashes.
//   - Zero storage for empty branches; automatic subtree pruning back to default hashes on delete.
//   - Fixed-size inclusion & non-inclusion proof generation (`generateProof`) and stateless
//     16-lane SIMD constant-time verifiers (`verifySmtInclusionProof` / `verifySmtNonInclusionProof`).
//   - Full defensive guards: invalid depth (`0`/`>32`), out-of-range key guard, full-table guard,
//     tampered sibling/depth/root/value rejection, and idempotent delete on absent keys.

const std = @import("std");
import { simdShamirSmtCtEqual } from "./shamirSecret.cp";

// ============================================================================
// Constants
// ============================================================================

pub const SMT_HASH_LEN: usize = 32;
pub const SMT_MAX_DEPTH: usize = 32;
pub const SMT_MAX_LEAVES: usize = 64;

// ============================================================================
// Internal Helpers
// ============================================================================

fn smtCtEqual32(a: [32]u8, b: [32]u8): bool {
    return simdShamirSmtCtEqual(a[0..], b[0..]);
}

fn smtIsKeyInRange(key: u32, depth: u8): bool {
    if (depth == 0 || (depth as usize) > SMT_MAX_DEPTH) {
        return false;
    }
    if (depth == 32) {
        return true;
    }
    const limit: u32 = (1 as u32) << (depth as u5);
    return key < limit;
}

fn smtKeyBitAt(key: u32, height: usize): u32 {
    const shift: u5 = height as u5;
    return (key >> shift) & 1;
}

/// Binary-searches sorted `leaves[lo..hi)` for the first index whose bit at `bit_height` is `1`.
fn smtSplitRangeByBit(leaves: []const SmtLeafEntry, lo: usize, hi: usize, bit_height: usize): usize {
    let l = lo;
    let r = hi;
    while (l < r) {
        const mid = l + (r - l) / 2;
        if (smtKeyBitAt(leaves[mid].key, bit_height) == 0) {
            l = mid + 1;
        } else {
            r = mid;
        }
    }
    return l;
}

fn smtComputeSubtreeHash(
    leaves: []const SmtLeafEntry,
    default_hashes: []const [32]u8,
    height: usize,
    lo: usize,
    hi: usize,
): [32]u8 {
    if (lo >= hi) {
        return default_hashes[height];
    }
    if (height == 0) {
        return leaves[lo].leaf_hash;
    }
    const mid = smtSplitRangeByBit(leaves, lo, hi, height - 1);
    const left_h = smtComputeSubtreeHash(leaves, default_hashes, height - 1, lo, mid);
    const right_h = smtComputeSubtreeHash(leaves, default_hashes, height - 1, mid, hi);
    return smtHashNode(left_h, right_h);
}

fn smtUnusedSiblingsZero(depth: u8, siblings: [32][32]u8): bool {
    const zero32: [32]u8 = [0; 32];
    let h: usize = depth as usize;
    let ok: bool = true;
    while (h < SMT_MAX_DEPTH) : (h += 1) {
        if (!smtCtEqual32(siblings[h], zero32)) {
            ok = false;
        }
    }
    return ok;
}

// ============================================================================
// 1. Domain-Separated SHA-256 Primitives
// ============================================================================

/// Canonical empty-leaf hash (32 zero bytes), distinct from `smtHashLeaf("") = SHA256(0x00)`.
pub fn smtEmptyLeafHash(): [32]u8 {
    const z: [32]u8 = [0; 32];
    return z;
}

/// Computes domain-separated leaf hash: `SHA256(0x00 || value_bytes)`.
pub fn smtHashLeaf(value_bytes: []const u8): [32]u8 {
    let out: [32]u8 = [0; 32];
    let h = std.crypto.hash.sha2.Sha256.init(.{});
    const prefix: [1]u8 = [0x00];
    h.update(&prefix);
    h.update(value_bytes);
    h.final(&out);
    return out;
}

/// Computes domain-separated internal node hash: `SHA256(0x01 || left || right)`.
pub fn smtHashNode(left: [32]u8, right: [32]u8): [32]u8 {
    let out: [32]u8 = [0; 32];
    let h = std.crypto.hash.sha2.Sha256.init(.{});
    const prefix: [1]u8 = [0x01];
    h.update(&prefix);
    h.update(&left);
    h.update(&right);
    h.final(&out);
    return out;
}

// ============================================================================
// 2. Data Models
// ============================================================================

/// Single stored leaf entry in sorted key order.
pub struct SmtLeafEntry {
    key: u32 = 0;
    leaf_hash: [32]u8 = [0; 32];
}

/// Fixed-size inclusion or non-inclusion proof for a key in an SMT of depth `1..=32`.
pub struct SmtProof {
    depth: u8 = 0;
    key: u32 = 0;
    leaf_present: bool = false;
    value_hash: [32]u8 = [0; 32];
    siblings: [32][32]u8 = [[0; 32]; 32];
}

/// Summary statistics for `SparseMerkleTree`.
pub struct SmtStats {
    depth: u8 = 0;
    leaf_count: usize = 0;
    capacity: usize = SMT_MAX_LEAVES;
}

// ============================================================================
// 3. Fixed-Depth Sparse Merkle Tree Engine
// ============================================================================

/// Zero-heap-allocation Sparse Merkle Tree supporting `depth in 1..=32` and up to 64 active leaves.
pub struct SparseMerkleTree {
    depth: u8 = 0;
    leaves: [64]SmtLeafEntry = [SmtLeafEntry{}; 64];
    leaf_count: usize = 0;
    default_hashes: [33][32]u8 = [[0; 32]; 33];

    /// Initializes an empty Sparse Merkle Tree of depth `depth` (`1..=32`) and precomputes
    /// `default_hashes[0..=depth]`. Returns `null` if `depth == 0` or `depth > 32`.
    pub static fn init(depth: u8): SparseMerkleTree? {
        if (depth == 0 || (depth as usize) > SMT_MAX_DEPTH) {
            return null;
        }
        let tree = SparseMerkleTree{
            depth: depth,
        };
        tree.default_hashes[0] = smtEmptyLeafHash();
        let h: usize = 1;
        while (h <= (depth as usize)) : (h += 1) {
            const prev = tree.default_hashes[h - 1];
            tree.default_hashes[h] = smtHashNode(prev, prev);
        }
        return tree;
    }

    /// Inserts or updates `(key, value_bytes)` in the tree.
    /// Returns `false` if `key` is out of range for `depth < 32` or if the 64-leaf table is full
    /// when inserting a new key.
    pub fn put(*_, key: u32, value_bytes: []const u8): bool {
        if (!smtIsKeyInRange(key, _.depth)) {
            return false;
        }
        const leaf_hash = smtHashLeaf(value_bytes);

        let lo: usize = 0;
        let hi: usize = _.leaf_count;
        while (lo < hi) {
            const mid = lo + (hi - lo) / 2;
            if (_.leaves[mid].key < key) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }

        if (lo < _.leaf_count && _.leaves[lo].key == key) {
            _.leaves[lo].leaf_hash = leaf_hash;
            return true;
        }

        if (_.leaf_count >= SMT_MAX_LEAVES) {
            return false;
        }
        let shift_i: usize = _.leaf_count;
        while (shift_i > lo) : (shift_i -= 1) {
            _.leaves[shift_i] = _.leaves[shift_i - 1];
        }
        _.leaves[lo] = SmtLeafEntry{
            key: key,
            leaf_hash: leaf_hash,
        };
        _.leaf_count += 1;
        return true;
    }

    /// Deletes `key` from the tree, automatically pruning empty subtrees back to default hashes.
    /// Returns `true` if `key` was present and removed, `false` otherwise (idempotent).
    pub fn delete(*_, key: u32): bool {
        if (!smtIsKeyInRange(key, _.depth) || _.leaf_count == 0) {
            return false;
        }
        let lo: usize = 0;
        let hi: usize = _.leaf_count;
        while (lo < hi) {
            const mid = lo + (hi - lo) / 2;
            if (_.leaves[mid].key < key) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        if (lo >= _.leaf_count || _.leaves[lo].key != key) {
            return false;
        }
        let i: usize = lo;
        while (i + 1 < _.leaf_count) : (i += 1) {
            _.leaves[i] = _.leaves[i + 1];
        }
        _.leaf_count -= 1;
        _.leaves[_.leaf_count] = SmtLeafEntry{};
        return true;
    }

    /// Returns `true` iff `key` is currently stored in the tree.
    pub fn contains(key: u32): bool {
        return _.getLeafHash(key) != null;
    }

    /// Returns the stored leaf hash for `key`, or `null` if absent or out of range.
    pub fn getLeafHash(key: u32): ([32]u8)? {
        if (!smtIsKeyInRange(key, _.depth) || _.leaf_count == 0) {
            return null;
        }
        let lo: usize = 0;
        let hi: usize = _.leaf_count;
        while (lo < hi) {
            const mid = lo + (hi - lo) / 2;
            if (_.leaves[mid].key < key) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        if (lo < _.leaf_count && _.leaves[lo].key == key) {
            return _.leaves[lo].leaf_hash;
        }
        return null;
    }

    /// Computes and returns the current 32-byte Sparse Merkle root hash.
    pub fn rootHash(): [32]u8 {
        return smtComputeSubtreeHash(
            _.leaves[0.._.leaf_count],
            _.default_hashes[0..],
            _.depth as usize,
            0,
            _.leaf_count,
        );
    }

    /// Returns the precomputed empty-tree root hash `default_hashes[depth]`.
    pub fn defaultRootHash(): [32]u8 {
        return _.default_hashes[_.depth as usize];
    }

    /// Generates an inclusion proof (if `key` exists) or non-inclusion proof (if `key` is absent).
    /// Returns `null` only if `key` is out of range for `depth < 32`.
    pub fn generateProof(key: u32): SmtProof? {
        if (!smtIsKeyInRange(key, _.depth)) {
            return null;
        }
        let proof = SmtProof{
            depth: _.depth,
            key: key,
            leaf_present: false,
            value_hash: smtEmptyLeafHash(),
        };

        const active = _.leaves[0.._.leaf_count];
        let lo: usize = 0;
        let hi: usize = _.leaf_count;
        let h: usize = _.depth as usize;
        while (h > 0) : (h -= 1) {
            const bit_h = h - 1;
            const mid = smtSplitRangeByBit(active, lo, hi, bit_h);
            if (smtKeyBitAt(key, bit_h) == 0) {
                proof.siblings[bit_h] = smtComputeSubtreeHash(active, _.default_hashes[0..], bit_h, mid, hi);
                hi = mid;
            } else {
                proof.siblings[bit_h] = smtComputeSubtreeHash(active, _.default_hashes[0..], bit_h, lo, mid);
                lo = mid;
            }
        }

        if (lo < hi && active[lo].key == key) {
            proof.leaf_present = true;
            proof.value_hash = active[lo].leaf_hash;
        }
        return proof;
    }

    /// Removes all leaves, resetting the tree back to its empty default root.
    pub fn clear(*_) {
        let i: usize = 0;
        while (i < _.leaf_count) : (i += 1) {
            _.leaves[i] = SmtLeafEntry{};
        }
        _.leaf_count = 0;
    }

    /// Returns current tree depth, active leaf count, and inline capacity.
    pub fn stats(): SmtStats {
        return SmtStats{
            depth: _.depth,
            leaf_count: _.leaf_count,
            capacity: SMT_MAX_LEAVES,
        };
    }
}

// ============================================================================
// 4. Stateless 16-Lane SIMD Constant-Time Inclusion & Non-Inclusion Verifiers
// ============================================================================

fn smtFoldProofToRoot(leaf_hash: [32]u8, key: u32, depth: u8, siblings: [32][32]u8): [32]u8 {
    let cur = leaf_hash;
    let h: usize = 0;
    while (h < (depth as usize)) : (h += 1) {
        if (smtKeyBitAt(key, h) == 0) {
            cur = smtHashNode(cur, siblings[h]);
        } else {
            cur = smtHashNode(siblings[h], cur);
        }
    }
    return cur;
}

/// Statelessly verifies an SMT inclusion proof that `(key, value_bytes)` is committed in `root`.
/// Enforces depth bounds, unused sibling zeroing, domain-separated leaf hash, and 16-lane SIMD CT root match.
pub fn verifySmtInclusionProof(
    root: [32]u8,
    key: u32,
    value_bytes: []const u8,
    proof: SmtProof,
): bool {
    if (proof.depth == 0 || (proof.depth as usize) > SMT_MAX_DEPTH) {
        return false;
    }
    if (!proof.leaf_present || proof.key != key || !smtIsKeyInRange(key, proof.depth)) {
        return false;
    }
    if (!smtUnusedSiblingsZero(proof.depth, proof.siblings)) {
        return false;
    }
    const expected_leaf = smtHashLeaf(value_bytes);
    if (!smtCtEqual32(proof.value_hash, expected_leaf)) {
        return false;
    }
    const computed_root = smtFoldProofToRoot(expected_leaf, key, proof.depth, proof.siblings);
    return smtCtEqual32(computed_root, root);
}

/// Statelessly verifies an SMT non-inclusion proof that `key` is absent from `root`.
/// Enforces depth bounds, unused sibling zeroing, empty leaf hash, and 16-lane SIMD CT root match.
pub fn verifySmtNonInclusionProof(
    root: [32]u8,
    key: u32,
    proof: SmtProof,
): bool {
    if (proof.depth == 0 || (proof.depth as usize) > SMT_MAX_DEPTH) {
        return false;
    }
    if (proof.leaf_present || proof.key != key || !smtIsKeyInRange(key, proof.depth)) {
        return false;
    }
    if (!smtUnusedSiblingsZero(proof.depth, proof.siblings)) {
        return false;
    }
    const empty_leaf = smtEmptyLeafHash();
    if (!smtCtEqual32(proof.value_hash, empty_leaf)) {
        return false;
    }
    const computed_root = smtFoldProofToRoot(empty_leaf, key, proof.depth, proof.siblings);
    return smtCtEqual32(computed_root, root);
}
