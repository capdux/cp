// Origin: Pure-CP Stdlib Test Suite (lib/crypto/merkle_tree.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc RFC 6962 Domain-Separated Merkle Tree:
//   1. Exact RFC 6962 root hashes for empty, 1, 2, 3, and 5 leaves (largestPowerOfTwoLessThan)
//   2. Exhaustive inclusion proof generation & verification for non-power-of-two trees (3, 5, 7, 13 leaves)
//   3. Tamper detection (leaf data, sibling hash, sibling_is_left, leaf_index, depth, wrong root)
//   4. Domain separation against second-preimage confusion (64B internal node vs leaf)
//   5. Pre-hashed append_leaf_hash equivalence & constant-time MerkleHash.eql
//   6. Full 128-leaf capacity, depth-7 proofs & out-of-bounds / malformed proof guards
//   7. Coexistence & byte-exact compatibility with existing cap:crypto::hashLeaf

import { test, expect_eq } from "cap:test";
import {
    hashLeaf,
    MERKLE_HASH_LEN,
    MERKLE_MAX_LEAVES,
    MERKLE_MAX_PROOF_DEPTH,
    MerkleHash,
    MerkleProofStep,
    MerkleInclusionProof,
    merkleHashEmpty,
    merkleHashLeaf,
    merkleHashNode,
    Rfc6962MerkleTree,
    verifyMerkleInclusionProof,
} from "cap:crypto";

// ============================================================================
// Test 1: Exact RFC 6962 Root Hashes (0, 1, 2, 3, 5 Leaves)
// ============================================================================
test("merkle_tree: exact RFC 6962 root hashes for empty, 1, 2, 3 and 5 leaves", () => {
    let tree = Rfc6962MerkleTree.init();

    // 0 leaves: SHA-256("") = e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
    const r0 = tree.compute_root();
    const expected_empty = merkleHashEmpty();
    expect_eq(MerkleHash{ bytes: r0 }.eql(MerkleHash{ bytes: expected_empty }), true);
    expect_eq(r0[0], 0xe3);
    expect_eq(r0[1], 0xb0);
    expect_eq(r0[31], 0x55);

    // 1 leaf
    const h0 = merkleHashLeaf("leaf-0");
    expect_eq(tree.append_leaf("leaf-0"), true);
    expect_eq(MerkleHash{ bytes: tree.compute_root() }.eql(MerkleHash{ bytes: h0 }), true);

    // 2 leaves: node(h0, h1)
    const h1 = merkleHashLeaf("leaf-1");
    expect_eq(tree.append_leaf("leaf-1"), true);
    const r2_exp = merkleHashNode(h0, h1);
    expect_eq(MerkleHash{ bytes: tree.compute_root() }.eql(MerkleHash{ bytes: r2_exp }), true);

    // 3 leaves (k = 2): node(node(h0, h1), h2)
    const h2 = merkleHashLeaf("leaf-2");
    expect_eq(tree.append_leaf("leaf-2"), true);
    const r3_exp = merkleHashNode(r2_exp, h2);
    expect_eq(MerkleHash{ bytes: tree.compute_root() }.eql(MerkleHash{ bytes: r3_exp }), true);

    // 5 leaves (k = 4): node(node(node(h0, h1), node(h2, h3)), h4)
    const h3 = merkleHashLeaf("leaf-3");
    const h4 = merkleHashLeaf("leaf-4");
    expect_eq(tree.append_leaf("leaf-3"), true);
    expect_eq(tree.append_leaf("leaf-4"), true);
    const left4 = merkleHashNode(r2_exp, merkleHashNode(h2, h3));
    const r5_exp = merkleHashNode(left4, h4);
    expect_eq(MerkleHash{ bytes: tree.compute_root() }.eql(MerkleHash{ bytes: r5_exp }), true);
});

// ============================================================================
// Test 2: Exhaustive Inclusion Proofs for Non-Power-of-Two Trees (3, 5, 7, 13)
// ============================================================================
test("merkle_tree: exhaustive inclusion proofs for 1, 3, 5, 7 and 13 leaves", () => {
    const sizes: [5]usize = [1, 3, 5, 7, 13];
    const payloads: [13]string = [
        "entry-00", "entry-01", "entry-02", "entry-03",
        "entry-04", "entry-05", "entry-06", "entry-07",
        "entry-08", "entry-09", "entry-10", "entry-11",
        "entry-12",
    ];

    let s: usize = 0;
    while (s < 5) : (s += 1) {
        const n = sizes[s];
        let tree = Rfc6962MerkleTree.init();
        let i: usize = 0;
        while (i < n) : (i += 1) {
            expect_eq(tree.append_leaf(payloads[i]), true);
        }
        expect_eq(tree.leaf_count(), n);
        const root = tree.compute_root();

        let idx: usize = 0;
        while (idx < n) : (idx += 1) {
            const proof_opt = tree.generate_inclusion_proof(idx);
            expect_eq(proof_opt != null, true);
            const proof = proof_opt ?? return;
            expect_eq(proof.leaf_index, idx);
            expect_eq(proof.leaf_count, n);
            expect_eq(verifyMerkleInclusionProof(payloads[idx], proof, root), true);
        }
    }
});

// ============================================================================
// Test 3: Tamper Detection (Leaf Data, Sibling, Direction, Index, Depth, Root)
// ============================================================================
test("merkle_tree: rejects tampered leaf data, sibling hash, direction, index & root", () => {
    let tree = Rfc6962MerkleTree.init();
    expect_eq(tree.append_leaf("alpha"), true);
    expect_eq(tree.append_leaf("beta"), true);
    expect_eq(tree.append_leaf("gamma"), true);
    expect_eq(tree.append_leaf("delta"), true);
    expect_eq(tree.append_leaf("epsilon"), true);

    const root = tree.compute_root();
    const proof_opt = tree.generate_inclusion_proof(2);
    expect_eq(proof_opt != null, true);
    const valid_proof = proof_opt ?? return;
    expect_eq(verifyMerkleInclusionProof("gamma", valid_proof, root), true);

    // 1. Tampered leaf data
    expect_eq(verifyMerkleInclusionProof("gamma!", valid_proof, root), false);

    // 2. Tampered sibling byte
    let bad_sib = valid_proof;
    bad_sib.steps[0].sibling[0] ^= 0x01;
    expect_eq(verifyMerkleInclusionProof("gamma", bad_sib, root), false);

    // 3. Tampered sibling_is_left direction
    let bad_dir = valid_proof;
    bad_dir.steps[0].sibling_is_left = !bad_dir.steps[0].sibling_is_left;
    expect_eq(verifyMerkleInclusionProof("gamma", bad_dir, root), false);

    // 4. Tampered leaf_index
    let bad_idx = valid_proof;
    bad_idx.leaf_index = 3;
    expect_eq(verifyMerkleInclusionProof("gamma", bad_idx, root), false);

    // 5. Tampered depth
    let bad_depth = valid_proof;
    bad_depth.depth -= 1;
    expect_eq(verifyMerkleInclusionProof("gamma", bad_depth, root), false);

    // 6. Tampered expected root
    let bad_root = root;
    bad_root[31] ^= 0x80;
    expect_eq(verifyMerkleInclusionProof("gamma", valid_proof, bad_root), false);
});

// ============================================================================
// Test 4: Domain Separation Against Second-Preimage Confusion
// ============================================================================
test("merkle_tree: domain separation prevents internal-node-as-leaf second-preimage", () => {
    // 2-leaf tree [A, B] -> root = SHA256(0x01 || H(A) || H(B))
    let t2 = Rfc6962MerkleTree.init();
    expect_eq(t2.append_leaf("record-A"), true);
    expect_eq(t2.append_leaf("record-B"), true);
    const root_2leaf = t2.compute_root();

    // Construct 64-byte concatenation H(A) || H(B) and submit it as a single leaf
    const ha = merkleHashLeaf("record-A");
    const hb = merkleHashLeaf("record-B");
    let forged_leaf: [64]u8 = [0; 64];
    let i: usize = 0;
    while (i < 32) : (i += 1) {
        forged_leaf[i] = ha[i];
        forged_leaf[32 + i] = hb[i];
    }

    let t1 = Rfc6962MerkleTree.init();
    expect_eq(t1.append_leaf(forged_leaf[0..]), true);
    const root_1leaf = t1.compute_root();

    // Domain separation (0x00 vs 0x01) guarantees roots MUST differ
    expect_eq(MerkleHash{ bytes: root_2leaf }.eql(MerkleHash{ bytes: root_1leaf }), false);

    // Single-leaf proof for forged_leaf cannot verify against the 2-leaf tree root
    const p1 = t1.generate_inclusion_proof(0) ?? return;
    expect_eq(verifyMerkleInclusionProof(forged_leaf[0..], p1, root_2leaf), false);
});

// ============================================================================
// Test 5: append_leaf_hash Equivalence & Constant-Time MerkleHash.eql
// ============================================================================
test("merkle_tree: append_leaf_hash matches append_leaf & MerkleHash.eql detects 1-bit flip", () => {
    let t_raw = Rfc6962MerkleTree.init();
    let t_pre = Rfc6962MerkleTree.init();

    const items: [4]string = ["tx-1", "tx-2", "tx-3", "tx-4"];
    let i: usize = 0;
    while (i < 4) : (i += 1) {
        expect_eq(t_raw.append_leaf(items[i]), true);
        expect_eq(t_pre.append_leaf_hash(merkleHashLeaf(items[i])), true);
    }

    const r1 = MerkleHash{ bytes: t_raw.compute_root() };
    const r2 = MerkleHash{ bytes: t_pre.compute_root() };
    expect_eq(r1.eql(r2), true);

    let r3 = r2;
    r3.bytes[15] ^= 0x04;
    expect_eq(r1.eql(r3), false);
});

// ============================================================================
// Test 6: Full 128-Leaf Capacity, Depth-7 Proofs & Out-of-Bounds Guards
// ============================================================================
test("merkle_tree: full 128-leaf capacity, depth-7 proofs & boundary guards", () => {
    expect_eq(MERKLE_HASH_LEN, 32);
    expect_eq(MERKLE_MAX_LEAVES, 128);
    expect_eq(MERKLE_MAX_PROOF_DEPTH, 8);

    let tree = Rfc6962MerkleTree.init();
    // Empty tree proof returns null
    expect_eq(tree.generate_inclusion_proof(0) == null, true);

    // Fill all 128 leaves with deterministic 2-byte payloads
    let i: usize = 0;
    while (i < MERKLE_MAX_LEAVES) : (i += 1) {
        const msg: [2]u8 = [0x40, i as u8];
        expect_eq(tree.append_leaf(msg[0..]), true);
    }
    expect_eq(tree.leaf_count(), 128);

    // 129th leaf rejected cleanly
    expect_eq(tree.append_leaf("overflow"), false);
    expect_eq(tree.append_leaf_hash(merkleHashEmpty()), false);
    expect_eq(tree.generate_inclusion_proof(128) == null, true);

    const root = tree.compute_root();
    // Verify boundary indices 0, 63, 64, 127 all have exact depth 7 and pass verification
    const check_indices: [4]usize = [0, 63, 64, 127];
    let k: usize = 0;
    while (k < 4) : (k += 1) {
        const idx = check_indices[k];
        const msg: [2]u8 = [0x40, idx as u8];
        const p = tree.generate_inclusion_proof(idx) ?? return;
        expect_eq(p.depth, 7);
        expect_eq(verifyMerkleInclusionProof(msg[0..], p, root), true);
    }

    // Malformed proofs rejected safely
    const empty_proof = MerkleInclusionProof{ leaf_index: 0, leaf_count: 0, depth: 0 };
    expect_eq(verifyMerkleInclusionProof("x", empty_proof, root), false);
    const oob_proof = MerkleInclusionProof{ leaf_index: 5, leaf_count: 5, depth: 3 };
    expect_eq(verifyMerkleInclusionProof("x", oob_proof, root), false);
    const deep_proof = MerkleInclusionProof{ leaf_index: 0, leaf_count: 4, depth: 99 };
    expect_eq(verifyMerkleInclusionProof("x", deep_proof, root), false);
});

// ============================================================================
// Test 7: Coexistence & Byte-Exact Compatibility with Existing cap:crypto::hashLeaf
// ============================================================================
test("merkle_tree: zero conflict & byte-exact parity with merkle.cp::hashLeaf", () => {
    const a = merkleHashLeaf("cp-stdlib-merkle");
    const b = hashLeaf("cp-stdlib-merkle");
    expect_eq(MerkleHash{ bytes: a }.eql(MerkleHash{ bytes: b }), true);
});
