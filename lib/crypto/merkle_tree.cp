// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc RFC 6962 Domain-Separated Merkle Tree & Inclusion Proofs) | Created: 2026-11-22
// Pure CP zero-heap-allocation RFC 6962 Certificate Transparency style SHA-256 binary Merkle Tree:
//   - Strict domain separation: 0x00 leaf prefix, 0x01 internal node prefix, SHA-256("") empty root
//   - Canonical `largestPowerOfTwoLessThan(n)` tree splitting supporting arbitrary non-power-of-two leaf counts (1..128)
//   - Fixed inline leaf buffer & fixed-depth O(log N) inclusion proof generation (`Rfc6962MerkleTree`)
//   - Stateless constant-time verifier with full structural path-direction validation (`verifyMerkleInclusionProof`)

const std = @import("std");

// ============================================================================
// 1. Constants & Constant-Time Helpers
// ============================================================================

/// SHA-256 digest length in bytes.
pub const MERKLE_HASH_LEN: usize = 32;
/// Maximum leaves stored inline in `Rfc6962MerkleTree` (zero dynamic heap allocations).
pub const MERKLE_MAX_LEAVES: usize = 128;
/// Maximum inclusion proof depth (`ceil(log2(128)) = 7`, padded to 8).
pub const MERKLE_MAX_PROOF_DEPTH: usize = 8;

fn constantTimeEql32(a: [MERKLE_HASH_LEN]u8, b: [MERKLE_HASH_LEN]u8): bool {
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < MERKLE_HASH_LEN) : (i += 1) {
        diff |= a[i] ^ b[i];
    }
    return diff == 0;
}

/// Returns the largest power of two strictly less than `n` (requires `n >= 2`).
fn largestPowerOfTwoLessThan(n: usize): usize {
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

/// Fixed 32-byte SHA-256 Merkle hash wrapper with constant-time equality comparison.
pub struct MerkleHash {
    bytes: [MERKLE_HASH_LEN]u8 = [0; MERKLE_HASH_LEN];

    /// Compares two 32-byte Merkle hashes in constant time.
    fn eql(other: MerkleHash): bool {
        return constantTimeEql32(_.bytes, other.bytes);
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

fn computeRangeRoot(
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
    const k = largestPowerOfTwoLessThan(count);
    const left = computeRangeRoot(leaves, start, k);
    const right = computeRangeRoot(leaves, start + k, count - k);
    return merkleHashNode(left, right);
}

fn buildInclusionPath(
    leaves: []const [MERKLE_HASH_LEN]u8,
    start: usize,
    count: usize,
    rel_index: usize,
    proof: *MerkleInclusionProof,
) {
    if (count <= 1) {
        return;
    }
    const k = largestPowerOfTwoLessThan(count);
    if (rel_index < k) {
        buildInclusionPath(leaves, start, k, rel_index, proof);
        const right_root = computeRangeRoot(leaves, start + k, count - k);
        if (proof.depth < MERKLE_MAX_PROOF_DEPTH) {
            proof.steps[proof.depth] = MerkleProofStep{
                sibling: right_root,
                sibling_is_left: false,
            };
            proof.depth += 1;
        }
    } else {
        buildInclusionPath(leaves, start + k, count - k, rel_index - k, proof);
        const left_root = computeRangeRoot(leaves, start, k);
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
        _.leaves[_.count] = h;
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
        return computeRangeRoot(_.leaves[0 .. _.count], 0, _.count);
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
        buildInclusionPath(_.leaves[0 .. _.count], 0, _.count, leaf_index, &proof);
        return proof;
    }
}

/// Statelessly verifies an RFC 6962 inclusion proof against `leaf_data` and `expected_root`.
/// Enforces exact structural path depth, canonical `sibling_is_left` direction matching `(leaf_index, leaf_count)`,
/// and constant-time root digest comparison.
pub fn verifyMerkleInclusionProof(
    leaf_data: []const u8,
    proof: MerkleInclusionProof,
    expected_root: [MERKLE_HASH_LEN]u8,
): bool {
    if (proof.leaf_count == 0 || proof.leaf_index >= proof.leaf_count || proof.depth > MERKLE_MAX_PROOF_DEPTH) {
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
        const k = largestPowerOfTwoLessThan(cur_n);
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

    let acc = merkleHashLeaf(leaf_data);
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

    return constantTimeEql32(acc, expected_root);
}
