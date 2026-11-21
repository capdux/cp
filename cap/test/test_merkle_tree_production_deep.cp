// Source: native-cp-stdlib (Cryptographic SHA-256 Merkle Tree)

import { test, expect_eq } from "cap:test";
import {
    MerkleTree,
    MerkleProof,
    ProofStep,
    hashLeaf,
    hashInternal,
    verifyProof,
} from "cap:merkle_tree";
import cap from "cap";

const std = @import("std");

test("empty and single leaf merkle tree root hash") {
    const alloc = std.testing.allocator;

    const empty_leaves: []const []const u8 = [];
    let empty_tree = try MerkleTree.init(alloc, empty_leaves);
    defer empty_tree.deinit();
    expect_eq(empty_tree.leafCount(), 0);
    const zero_root = std.mem.zeroes([32]u8);
    expect_eq(std.mem.eql(u8, &empty_tree.rootHash(), &zero_root), true);

    const single_leaves: [1][]const u8 = ["genesis-block"];
    let single_tree = try MerkleTree.init(alloc, &single_leaves);
    defer single_tree.deinit();
    expect_eq(single_tree.leafCount(), 1);
    const expected_single_root = hashLeaf("genesis-block");
    expect_eq(std.mem.eql(u8, &single_tree.rootHash(), &expected_single_root), true);

    let single_proof = try single_tree.generateProof(alloc, 0);
    defer single_proof.deinit(alloc);
    expect_eq(single_proof.steps.len, 0);
    expect_eq(verifyProof(single_tree.rootHash(), "genesis-block", single_proof), true);
    expect_eq(verifyProof(single_tree.rootHash(), "tampered", single_proof), false);
}

test("domain separation prevents second-preimage collision") {
    const left = hashLeaf("alpha");
    const right = hashLeaf("beta");

    let combined: [64]u8 = undefined;
    @memcpy(combined[0..32], &left);
    @memcpy(combined[32..64], &right);

    const as_leaf = hashLeaf(&combined);
    const as_internal = hashInternal(left, right);
    expect_eq(std.mem.eql(u8, &as_leaf, &as_internal), false);

    // Also verify 0x00 vs 0x01 prefix on identical raw payload
    let prefixed_payload: [65]u8 = undefined;
    prefixed_payload[0] = 0x01;
    @memcpy(prefixed_payload[1..65], &combined);
    const leaf_of_internal_preimage = hashLeaf(&prefixed_payload);
    expect_eq(std.mem.eql(u8, &leaf_of_internal_preimage, &as_internal), false);
}

test("odd leaves (5 and 7) full proof generation and verification") {
    const alloc = std.testing.allocator;

    const leaves5: [5][]const u8 = [ "L0", "L1", "L2", "L3", "L4" ];
    let tree5 = try MerkleTree.init(alloc, &leaves5);
    defer tree5.deinit();
    expect_eq(tree5.leafCount(), 5);
    const root5 = tree5.rootHash();

    let i5: usize = 0;
    while (i5 < leaves5.len) {
        let proof = try tree5.generateProof(alloc, i5);
        defer proof.deinit(alloc);
        expect_eq(verifyProof(root5, leaves5[i5], proof), true);
        i5 = i5 + 1;
    }

    const leaves7: [7][]const u8 = [ "a", "b", "c", "d", "e", "f", "g" ];
    let tree7 = try MerkleTree.init(alloc, &leaves7);
    defer tree7.deinit();
    expect_eq(tree7.leafCount(), 7);
    const root7 = tree7.rootHash();

    let i7: usize = 0;
    while (i7 < leaves7.len) {
        let proof = try tree7.generateProof(alloc, i7);
        defer proof.deinit(alloc);
        expect_eq(verifyProof(root7, leaves7[i7], proof), true);
        i7 = i7 + 1;
    }
}

test("power-of-two leaves (8) and tamper rejection") {
    const alloc = std.testing.allocator;

    const leaves8: [8][]const u8 = [ "0", "1", "2", "3", "4", "5", "6", "7" ];
    let tree = try MerkleTree.init(alloc, &leaves8);
    defer tree.deinit();
    expect_eq(tree.leafCount(), 8);
    const root = tree.rootHash();

    // Verify all 8 leaves
    let i8: usize = 0;
    while (i8 < leaves8.len) {
        let proof = try tree.generateProof(alloc, i8);
        defer proof.deinit(alloc);
        expect_eq(proof.steps.len, 3);
        expect_eq(verifyProof(root, leaves8[i8], proof), true);
        i8 = i8 + 1;
    }

    // Tampered leaf data
    let proof2 = try tree.generateProof(alloc, 2);
    defer proof2.deinit(alloc);
    expect_eq(verifyProof(root, "2-tampered", proof2), false);

    // Wrong leaf index (proof generated for index 3 tested with leaf 2)
    let proof3 = try tree.generateProof(alloc, 3);
    defer proof3.deinit(alloc);
    expect_eq(verifyProof(root, "2", proof3), false);

    // Tampered proof.leaf_index directly
    let bad_index_proof = MerkleProof{
        .leaf_index = 4,
        .leaf_count = proof2.leaf_count,
        .steps = proof2.steps,
    };
    expect_eq(verifyProof(root, "2", bad_index_proof), false);

    // Tampered sibling hash
    let bad_steps = try alloc.dupe(ProofStep, proof2.steps);
    defer alloc.free(bad_steps);
    bad_steps[0].hash[0] = bad_steps[0].hash[0] ^ 0xff;
    let tampered_sib_proof = MerkleProof{
        .leaf_index = proof2.leaf_index,
        .leaf_count = proof2.leaf_count,
        .steps = bad_steps,
    };
    expect_eq(verifyProof(root, "2", tampered_sib_proof), false);
}

test("cap.merkle_tree dynamic mount and out-of-bounds error") {
    const alloc = std.testing.allocator;

    const leaves: [3][]const u8 = [ "alice", "bob", "carol" ];
    let tree = try cap.merkle_tree.MerkleTree.init(alloc, &leaves);
    defer tree.deinit();

    const root = tree.rootHash();
    let proof = try tree.generateProof(alloc, 1);
    defer proof.deinit(alloc);

    expect_eq(cap.merkle_tree.verifyProof(root, "bob", proof), true);

    const oob_res = tree.generateProof(alloc, 3);
    expect_eq(std.meta.isError(oob_res), true);
}
