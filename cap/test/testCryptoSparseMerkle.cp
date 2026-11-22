// Origin: Pure-CP Stdlib Test Suite (lib/crypto/sparseMerkle.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc Fixed-Depth Sparse Merkle Tree (SMT):
//   1. Default empty root & insertion-order invariance across two independent trees
//   2. Full 32-bit depth=32 inclusion & non-inclusion proofs (extreme keys 0, 1, 0x80000000, 0xFFFFFFFF & empty value "")
//   3. In-place value update, delete with automatic subtree pruning back to defaultRootHash(), and idempotent delete
//   4. Strict tamper rejection (flipped sibling bit, tampered root, tampered value_hash, wrong key/value, flipped flag)
//   5. Cryptographic domain separation (0x00 leaf vs 0x01 internal node)
//   6. 64-leaf capacity guard, in-place update while full, and slot reclamation after delete
//   7. Boundary guards (depth 0 / 33, out-of-range keys for depth < 32) & cap.crypto dynamic facade

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    SMT_MAX_LEAVES,
    smtHashLeaf,
    smtHashNode,
    SparseMerkleTree,
    verifySmtInclusionProof,
    verifySmtNonInclusionProof,
} from "cap:crypto";

fn hashEq(a: [32]u8, b: [32]u8): bool {
    let i: usize = 0;
    while (i < 32) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// Test 1: Empty Default Root & Insertion-Order Invariance
// ============================================================================
test("sparseMerkle: empty default root & insertion-order invariance", () => {
    let t1 = SparseMerkleTree.init(16) ?? return;
    let t2 = SparseMerkleTree.init(16) ?? return;

    // Empty tree root equals precomputed defaultRootHash
    expect_true(hashEq(t1.rootHash(), t1.defaultRootHash()));

    // Insert 5 entries in forward order into t1 and reverse order into t2
    expect_true(t1.put(10, "alpha"));
    expect_true(t1.put(500, "beta"));
    expect_true(t1.put(1024, "gamma"));
    expect_true(t1.put(32768, "delta"));
    expect_true(t1.put(65535, "epsilon"));

    expect_true(t2.put(65535, "epsilon"));
    expect_true(t2.put(32768, "delta"));
    expect_true(t2.put(1024, "gamma"));
    expect_true(t2.put(500, "beta"));
    expect_true(t2.put(10, "alpha"));

    expect_true(hashEq(t1.rootHash(), t2.rootHash()));
    expect_eq(hashEq(t1.rootHash(), t1.defaultRootHash()), false);
});

// ============================================================================
// Test 2: Depth-32 Inclusion & Non-Inclusion Proofs (Extreme Keys & Empty Value)
// ============================================================================
test("sparseMerkle: depth=32 inclusion & non-inclusion proofs including empty value", () => {
    let tree = SparseMerkleTree.init(32) ?? return;
    expect_true(tree.put(0, "zero"));
    expect_true(tree.put(1, "one"));
    expect_true(tree.put(42, "")); // empty value must be distinct from absent key
    expect_true(tree.put(0x80000000, "high-bit"));
    expect_true(tree.put(0xFFFFFFFF, "max-u32"));

    const root = tree.rootHash();

    // Inclusion proofs for all 5 keys
    if (p0 := tree.generateProof(0)) {
        expect_true(verifySmtInclusionProof(root, 0, "zero", p0));
        expect_eq(verifySmtNonInclusionProof(root, 0, p0), false);
    } else {
        expect_true(false);
    }
    if (p42 := tree.generateProof(42)) {
        expect_true(p42.leaf_present);
        expect_true(verifySmtInclusionProof(root, 42, "", p42));
        expect_eq(verifySmtNonInclusionProof(root, 42, p42), false);
    } else {
        expect_true(false);
    }
    if (phi := tree.generateProof(0x80000000)) {
        expect_true(verifySmtInclusionProof(root, 0x80000000, "high-bit", phi));
    } else {
        expect_true(false);
    }
    if (pmax := tree.generateProof(0xFFFFFFFF)) {
        expect_true(verifySmtInclusionProof(root, 0xFFFFFFFF, "max-u32", pmax));
    } else {
        expect_true(false);
    }

    // Non-inclusion proofs for absent neighbor keys
    if (np2 := tree.generateProof(2)) {
        expect_eq(np2.leaf_present, false);
        expect_true(verifySmtNonInclusionProof(root, 2, np2));
        expect_eq(verifySmtInclusionProof(root, 2, "", np2), false);
    } else {
        expect_true(false);
    }
    if (np_mid := tree.generateProof(0x7FFFFFFF)) {
        expect_true(verifySmtNonInclusionProof(root, 0x7FFFFFFF, np_mid));
    } else {
        expect_true(false);
    }
});

// ============================================================================
// Test 3: Update, Delete Pruning Back to Default Root & Idempotent Delete
// ============================================================================
test("sparseMerkle: update changes root and delete prunes back to exact defaultRootHash", () => {
    let tree = SparseMerkleTree.init(12) ?? return;
    const empty_root = tree.defaultRootHash();

    expect_true(tree.put(7, "v1"));
    expect_true(tree.put(100, "keep"));
    const root_v1 = tree.rootHash();
    const proof_v1 = tree.generateProof(7) ?? return;
    expect_true(verifySmtInclusionProof(root_v1, 7, "v1", proof_v1));

    // Update key 7 -> root changes, old proof rejected, new proof accepted
    expect_true(tree.put(7, "v2"));
    const root_v2 = tree.rootHash();
    expect_eq(hashEq(root_v1, root_v2), false);
    expect_eq(verifySmtInclusionProof(root_v2, 7, "v1", proof_v1), false);
    if (proof_v2 := tree.generateProof(7)) {
        expect_true(verifySmtInclusionProof(root_v2, 7, "v2", proof_v2));
    } else {
        expect_true(false);
    }

    // Delete key 7 -> non-inclusion proof valid; second delete is idempotent (false)
    expect_true(tree.delete(7));
    expect_eq(tree.delete(7), false);
    expect_eq(tree.contains(7), false);
    const root_after_del7 = tree.rootHash();
    if (np7 := tree.generateProof(7)) {
        expect_true(verifySmtNonInclusionProof(root_after_del7, 7, np7));
    } else {
        expect_true(false);
    }

    // Delete last remaining key 100 -> root prunes back to exact defaultRootHash
    expect_true(tree.delete(100));
    expect_eq(tree.stats().leaf_count, 0 as usize);
    expect_true(hashEq(tree.rootHash(), empty_root));
});

// ============================================================================
// Test 4: Strict Tamper Rejection (Sibling, Value, Root, Key, Presence Flag)
// ============================================================================
test("sparseMerkle: tampered sibling, root, value_hash, key or flag strictly rejected", () => {
    let tree = SparseMerkleTree.init(16) ?? return;
    expect_true(tree.put(123, "secret-record"));
    expect_true(tree.put(456, "other-record"));
    const root = tree.rootHash();
    const valid_proof = tree.generateProof(123) ?? return;
    expect_true(verifySmtInclusionProof(root, 123, "secret-record", valid_proof));

    // 1. Wrong value bytes
    expect_eq(verifySmtInclusionProof(root, 123, "tampered-record", valid_proof), false);

    // 2. Wrong key
    expect_eq(verifySmtInclusionProof(root, 124, "secret-record", valid_proof), false);

    // 3. Flipped bit in sibling[0]
    let bad_sib = valid_proof;
    bad_sib.siblings[0][0] ^= 0x01;
    expect_eq(verifySmtInclusionProof(root, 123, "secret-record", bad_sib), false);

    // 4. Flipped bit in top sibling[15]
    let bad_top = valid_proof;
    bad_top.siblings[15][31] ^= 0x80;
    expect_eq(verifySmtInclusionProof(root, 123, "secret-record", bad_top), false);

    // 5. Tampered root
    let bad_root = root;
    bad_root[10] ^= 0xFF;
    expect_eq(verifySmtInclusionProof(bad_root, 123, "secret-record", valid_proof), false);

    // 6. Tampered value_hash in proof
    let bad_vh = valid_proof;
    bad_vh.value_hash[0] ^= 0x01;
    expect_eq(verifySmtInclusionProof(root, 123, "secret-record", bad_vh), false);

    // 7. Flipped leaf_present flag
    let flipped = valid_proof;
    flipped.leaf_present = false;
    expect_eq(verifySmtInclusionProof(root, 123, "secret-record", flipped), false);
    expect_eq(verifySmtNonInclusionProof(root, 123, flipped), false);
});

// ============================================================================
// Test 5: Domain Separation (0x00 Leaf vs 0x01 Internal Node)
// ============================================================================
test("sparseMerkle: domain separation distinguishes 0x00 leaf from 0x01 internal node", () => {
    const left = smtHashLeaf("L");
    const right = smtHashLeaf("R");
    const node_h = smtHashNode(left, right);

    let concat64: [64]u8 = [0; 64];
    let i: usize = 0;
    while (i < 32) : (i += 1) {
        concat64[i] = left[i];
        concat64[32 + i] = right[i];
    }
    const leaf_of_concat = smtHashLeaf(concat64[0..]);
    expect_eq(hashEq(node_h, leaf_of_concat), false);
});

// ============================================================================
// Test 6: 64-Leaf Capacity Guard, In-Place Update While Full & Reclamation
// ============================================================================
test("sparseMerkle: 64-leaf capacity guard allows updates while full and reuses deleted slots", () => {
    let tree = SparseMerkleTree.init(16) ?? return;
    let k: u32 = 0;
    while (k < (SMT_MAX_LEAVES as u32)) : (k += 1) {
        expect_true(tree.put(k * 7, "item"));
    }
    expect_eq(tree.stats().leaf_count, SMT_MAX_LEAVES);

    // 65th distinct key rejected
    expect_eq(tree.put(9999, "overflow"), false);

    // Updating an existing key while full succeeds
    expect_true(tree.put(0, "updated-item"));

    // Deleting one key frees capacity for the new key
    expect_true(tree.delete(7));
    expect_true(tree.put(9999, "now-fits"));
    expect_eq(tree.stats().leaf_count, SMT_MAX_LEAVES);
});

// ============================================================================
// Test 7: Invalid Depth, Out-of-Range Key Guards & cap.crypto Dynamic Facade
// ============================================================================
test("sparseMerkle: invalid depth, out-of-range key guards & cap.crypto dynamic facade", () => {
    expect_eq(SparseMerkleTree.init(0) == null, true);
    expect_eq(SparseMerkleTree.init(33) == null, true);

    // Depth 4 supports keys 0..15 only
    let tree = cap.crypto.SparseMerkleTree.init(4) ?? return;
    expect_eq(tree.put(16, "oob"), false);
    expect_eq(tree.delete(16), false);
    expect_eq(tree.generateProof(16) == null, true);

    expect_true(tree.put(15, "edge-key"));
    const root = tree.rootHash();
    if (p := tree.generateProof(15)) {
        expect_true(cap.crypto.verifySmtInclusionProof(root, 15, "edge-key", p));
    } else {
        expect_true(false);
    }

    tree.clear();
    expect_eq(tree.stats().leaf_count, 0 as usize);
    expect_true(hashEq(tree.rootHash(), tree.defaultRootHash()));
});
