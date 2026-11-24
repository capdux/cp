// Reference-hardened against RustCrypto & RFC 6962 (MIT OR Apache-2.0)
// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc RFC 6962 Domain-Separated Merkle Tree & Inclusion Proofs) | Created: 2026-11-22
// Pure CP zero-heap-allocation RFC 6962 Certificate Transparency style SHA-256 binary Merkle Tree:
//   - 16-lane @Vector(16, u8) SIMD (`CryptoProofVec16u8`) branchless constant-time equality (`simdProofCtEqual`)
//     and 16-lane payload/hash copy (`simdProofCopyBytes`) with 0-diff scalar references
//   - Strict domain separation: 0x00 leaf prefix, 0x01 internal node prefix, SHA-256("") empty root
//   - Canonical `largestPowerOfTwoLessThan(n)` tree splitting supporting arbitrary non-power-of-two leaf counts (1..128)
//   - Fixed inline leaf buffer & fixed-depth O(log N) inclusion proof generation (`Rfc6962MerkleTree`)
//   - Stateless constant-time verifier with full structural path-direction validation (`verifyMerkleInclusionProof`)

const std = @import("std");

/// Module-local 16-lane byte vector alias for crypto proof & envelope hot paths.
type CryptoProofVec16u8 = @Vector(16, u8);

// ============================================================================
// 1. Constants & 16-Lane SIMD Hot-Path Primitives (0-Diff with Scalar)
// ============================================================================

/// SHA-256 digest length in bytes.
pub const MERKLE_HASH_LEN: usize = 32;
/// Maximum leaves stored inline in `Rfc6962MerkleTree` (zero dynamic heap allocations).
pub const MERKLE_MAX_LEAVES: usize = 128;
/// Maximum inclusion proof depth (`ceil(log2(128)) = 7`, padded to 8).
pub const MERKLE_MAX_PROOF_DEPTH: usize = 8;

/// Scalar reference: branchless constant-time byte-slice equality (never short-circuits on mismatch).
pub fn scalarProofCtEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        diff |= a[i] ^ b[i];
    }
    return diff == 0;
}

/// 16-lane SIMD accelerated branchless constant-time byte-slice equality (zero early exits).
pub fn simdProofCtEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let acc_vec: CryptoProofVec16u8 = @splat(0);
    let i: usize = 0;
    while (i + 16 <= a.len) {
        const va: CryptoProofVec16u8 = a[i .. i + 16][0..16].*;
        const vb: CryptoProofVec16u8 = b[i .. i + 16][0..16].*;
        acc_vec = acc_vec | (va ^ vb);
        i += 16;
    }
    let diff: u8 = @reduce(.Or, acc_vec);
    while (i < a.len) : (i += 1) {
        diff |= a[i] ^ b[i];
    }
    return diff == 0;
}

/// Scalar reference: copy bytes from `src` into `dst`.
pub fn scalarProofCopyBytes(dst: *[]u8, src: []const u8) {
    const n = if (dst.len < src.len) dst.len else src.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
}

/// 16-lane SIMD accelerated payload & hash byte copy from `src` into `dst`.
pub fn simdProofCopyBytes(dst: *[]u8, src: []const u8) {
    const n = if (dst.len < src.len) dst.len else src.len;
    let i: usize = 0;
    while (i + 16 <= n) {
        const v: CryptoProofVec16u8 = src[i .. i + 16][0..16].*;
        dst[i .. i + 16][0..16].* = v;
        i += 16;
    }
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
}

/// Returns the largest power of two strictly less than `n` (requires `n >= 2`).
fn mtLargestPowerOfTwoLessThan(n: usize): usize {
    let k: usize = 1;
    while (k < n - k) {
        k = k << 1;
    }
    return k;
}

// ============================================================================
// 2. Domain-Separated RFC 6962 Hash Primitives
// ============================================================================

/// Computes the RFC 6962 empty tree root hash: `SHA256("")`.
pub fn merkleHashEmpty(): [MERKLE_HASH_LEN]u8 {
    let out: [MERKLE_HASH_LEN]u8 = [0; MERKLE_HASH_LEN];
    let h = std.crypto.hash.sha2.Sha256.init(.{});
    h.final(&out);
    return out;
}

/// Computes the RFC 6962 domain-separated leaf hash: `SHA256(0x00 || data)`.
pub fn merkleHashLeaf(data: []const u8): [MERKLE_HASH_LEN]u8 {
    let out: [MERKLE_HASH_LEN]u8 = [0; MERKLE_HASH_LEN];
    let h = std.crypto.hash.sha2.Sha256.init(.{});
    const prefix: [1]u8 = [0x00];
    h.update(&prefix);
    h.update(data);
    h.final(&out);
    return out;
}

/// Computes the RFC 6962 domain-separated internal node hash: `SHA256(0x01 || left || right)`.
pub fn merkleHashNode(left: [MERKLE_HASH_LEN]u8, right: [MERKLE_HASH_LEN]u8): [MERKLE_HASH_LEN]u8 {
    let out: [MERKLE_HASH_LEN]u8 = [0; MERKLE_HASH_LEN];
    let h = std.crypto.hash.sha2.Sha256.init(.{});
    const prefix: [1]u8 = [0x01];
    h.update(&prefix);
    h.update(&left);
    h.update(&right);
    h.final(&out);
    return out;
}

// ============================================================================
// 3. Public Models: MerkleHash, Proof Step & Inclusion Proof
// ============================================================================

/// Fixed 32-byte SHA-256 Merkle hash wrapper with 16-lane SIMD constant-time equality comparison.
pub struct MerkleHash {
    bytes: [MERKLE_HASH_LEN]u8 = [0; MERKLE_HASH_LEN];

    /// Compares two 32-byte Merkle hashes in branchless 16-lane SIMD constant time.
    fn eql(other: MerkleHash): bool {
        return simdProofCtEqual(_.bytes[0..], other.bytes[0..]);
    }
}

/// Single sibling step along a bottom-up Merkle inclusion proof path.
pub struct MerkleProofStep {
    sibling: [MERKLE_HASH_LEN]u8 = [0; MERKLE_HASH_LEN];
    sibling_is_left: bool = false;
}

/// Zero-heap-allocation RFC 6962 Merkle inclusion proof (bottom-up from leaf to root).
pub struct MerkleInclusionProof {
    leaf_index: usize = 0;
    leaf_count: usize = 0;
    steps: [MERKLE_MAX_PROOF_DEPTH]MerkleProofStep = [MerkleProofStep{}; MERKLE_MAX_PROOF_DEPTH];
    depth: usize = 0;
}

// ============================================================================
// 4. Internal Recursive RFC 6962 Subtree Evaluation & Proof Construction
// ============================================================================

fn mtComputeRangeRoot(
    leaves: []const [MERKLE_HASH_LEN]u8,
    start: usize,
    count: usize,
): [MERKLE_HASH_LEN]u8 {
    if (count == 0) {
        return merkleHashEmpty();
    }
    if (count == 1) {
        return leaves[start];
    }
    const k = mtLargestPowerOfTwoLessThan(count);
    const left = mtComputeRangeRoot(leaves, start, k);
    const right = mtComputeRangeRoot(leaves, start + k, count - k);
    return merkleHashNode(left, right);
}

fn mtBuildInclusionPath(
    leaves: []const [MERKLE_HASH_LEN]u8,
    start: usize,
    count: usize,
    rel_index: usize,
    proof: *MerkleInclusionProof,
) {
    if (count <= 1) {
        return;
    }
    const k = mtLargestPowerOfTwoLessThan(count);
    if (rel_index < k) {
        mtBuildInclusionPath(leaves, start, k, rel_index, proof);
        const right_root = mtComputeRangeRoot(leaves, start + k, count - k);
        if (proof.depth < MERKLE_MAX_PROOF_DEPTH) {
            proof.steps[proof.depth] = MerkleProofStep{
                sibling: right_root,
                sibling_is_left: false,
            };
            proof.depth += 1;
        }
    } else {
        mtBuildInclusionPath(leaves, start + k, count - k, rel_index - k, proof);
        const left_root = mtComputeRangeRoot(leaves, start, k);
        if (proof.depth < MERKLE_MAX_PROOF_DEPTH) {
            proof.steps[proof.depth] = MerkleProofStep{
                sibling: left_root,
                sibling_is_left: true,
            };
            proof.depth += 1;
        }
    }
}

// ============================================================================
// 5. Zero-Alloc RFC 6962 Merkle Tree & Stateless Verifier
// ============================================================================

/// Zero-heap-allocation RFC 6962 binary Merkle Tree holding up to 128 leaf digests inline.
pub struct Rfc6962MerkleTree {
    leaves: [MERKLE_MAX_LEAVES][MERKLE_HASH_LEN]u8 = [[0; MERKLE_HASH_LEN]; MERKLE_MAX_LEAVES];
    count: usize = 0;

    /// Initializes an empty RFC 6962 Merkle tree.
    static fn init(): Rfc6962MerkleTree {
        return Rfc6962MerkleTree{
            leaves: [[0; MERKLE_HASH_LEN]; MERKLE_MAX_LEAVES],
            count: 0,
        };
    }

    /// Hashes `data` with the `0x00` leaf domain separator and appends it to the tree.
    /// Returns `false` if the tree is already at `MERKLE_MAX_LEAVES` (128).
    fn append_leaf(*_, data: []const u8): bool {
        if (_.count >= MERKLE_MAX_LEAVES) {
            return false;
        }
        _.leaves[_.count] = merkleHashLeaf(data);
        _.count += 1;
        return true;
    }

    /// Appends a pre-computed 32-byte leaf digest directly to the tree.
    /// Returns `false` if the tree is already at `MERKLE_MAX_LEAVES` (128).
    fn append_leaf_hash(*_, h: [MERKLE_HASH_LEN]u8): bool {
        if (_.count >= MERKLE_MAX_LEAVES) {
            return false;
        }
        simdProofCopyBytes(_.leaves[_.count][0..], h[0..]);
        _.count += 1;
        return true;
    }

    /// Returns the number of leaves currently stored in the tree.
    fn leaf_count(): usize {
        return _.count;
    }

    /// Computes the canonical RFC 6962 Merkle root hash.
    /// - 0 leaves -> `merkleHashEmpty()` (`SHA256("")`)
    /// - 1 leaf   -> `leaves[0]`
    /// - `n >= 2` -> `merkleHashNode(root(0..k), root(k..n))` where `k = largestPowerOfTwoLessThan(n)`
    fn compute_root(): [MERKLE_HASH_LEN]u8 {
        if (_.count == 0) {
            return merkleHashEmpty();
        }
        return mtComputeRangeRoot(_.leaves[0 .. _.count], 0, _.count);
    }

    /// Generates a zero-alloc RFC 6962 inclusion proof for `leaf_index`.
    /// Returns `null` if the tree is empty or `leaf_index >= leaf_count()`.
    fn generate_inclusion_proof(leaf_index: usize): MerkleInclusionProof? {
        if (_.count == 0 || leaf_index >= _.count) {
            return null;
        }
        let proof = MerkleInclusionProof{
            leaf_index: leaf_index,
            leaf_count: _.count,
            steps: [MerkleProofStep{}; MERKLE_MAX_PROOF_DEPTH],
            depth: 0,
        };
        mtBuildInclusionPath(_.leaves[0 .. _.count], 0, _.count, leaf_index, &proof);
        return proof;
    }
}

/// Statelessly verifies an RFC 6962 inclusion proof given a pre-hashed 32-byte `leaf_hash` and `expected_root`.
/// Enforces `leaf_count in 1..=MERKLE_MAX_LEAVES`, `leaf_index < leaf_count`, exact structural path depth,
/// canonical `sibling_is_left` directions, and 16-lane SIMD constant-time root digest comparison.
pub fn verifyMerkleInclusionProofHash(
    leaf_hash: [MERKLE_HASH_LEN]u8,
    proof: MerkleInclusionProof,
    expected_root: [MERKLE_HASH_LEN]u8,
): bool {
    if (proof.leaf_count == 0 || proof.leaf_count > MERKLE_MAX_LEAVES || proof.leaf_index >= proof.leaf_count || proof.depth > MERKLE_MAX_PROOF_DEPTH) {
        return false;
    }

    // Derive expected top-down sibling directions for (leaf_index, leaf_count)
    let exp_left: [MERKLE_MAX_PROOF_DEPTH]bool = [false; MERKLE_MAX_PROOF_DEPTH];
    let exp_depth: usize = 0;
    let cur_n = proof.leaf_count;
    let cur_idx = proof.leaf_index;
    while (cur_n > 1) {
        if (exp_depth >= MERKLE_MAX_PROOF_DEPTH) {
            return false;
        }
        const k = mtLargestPowerOfTwoLessThan(cur_n);
        if (cur_idx < k) {
            exp_left[exp_depth] = false;
            cur_n = k;
        } else {
            exp_left[exp_depth] = true;
            cur_idx -= k;
            cur_n -= k;
        }
        exp_depth += 1;
    }

    if (proof.depth != exp_depth) {
        return false;
    }

    let acc = leaf_hash;
    let i: usize = 0;
    while (i < proof.depth) : (i += 1) {
        const expected_is_left = exp_left[exp_depth - 1 - i];
        if (proof.steps[i].sibling_is_left != expected_is_left) {
            return false;
        }
        if (proof.steps[i].sibling_is_left) {
            acc = merkleHashNode(proof.steps[i].sibling, acc);
        } else {
            acc = merkleHashNode(acc, proof.steps[i].sibling);
        }
    }

    return simdProofCtEqual(acc[0..], expected_root[0..]);
}

/// Statelessly verifies an RFC 6962 inclusion proof against raw `leaf_data` and `expected_root`.
pub fn verifyMerkleInclusionProof(
    leaf_data: []const u8,
    proof: MerkleInclusionProof,
    expected_root: [MERKLE_HASH_LEN]u8,
): bool {
    return verifyMerkleInclusionProofHash(merkleHashLeaf(leaf_data), proof, expected_root);
}
