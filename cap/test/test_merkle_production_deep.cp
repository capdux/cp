// Source: cp/cap/test/test_merkle_production_deep.cp
// Deep Production Verification Suite for cap:merkle & cap.merkle
// Covers:
//   1. Non-power-of-two tree builds (1, 2, 3, 5, 7, 17), deterministic rootHex & second-preimage resistance
//   2. Exhaustive generateProof & verifyProof across all 17 leaves
//   3. Tamper resistance (leaf_data, sibling_hash, sibling_side, root_hash) & EmptyTree/IndexOutOfBounds
//   4. diffTrees dual-tree fast diffing (100 blocks with 2 edits + cross-size diff) & zero memory leaks
//   5. `import cap from "cap"; cap.merkle` dynamic mounting

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MerkleError,
    ProofSide,
    ProofStep,
    MerkleProof,
    MerkleTree,
    hashLeaf,
    hashNode,
    verifyProof,
    diffTrees,
} from "cap:merkle";

fn expectBuildEmpty(alloc: std.mem.Allocator): bool {
    const empty_slice: []const string = [];
    let t = MerkleTree.build(alloc, empty_slice) catch e => {
        return e == MerkleError.EmptyTree;
    };
    t.deinit();
    return false;
}

fn expectOobProof(alloc: std.mem.Allocator, tree: &MerkleTree, idx: usize): bool {
    let p = tree.generateProof(alloc, idx) catch e => {
        return e == MerkleError.IndexOutOfBounds;
    };
    p.deinit();
    return false;
}

test("cap:merkle - non-power-of-two builds, deterministic rootHex & second-preimage resistance") {
    const alloc = std.testing.allocator;
    const counts: [6]usize = [1, 2, 3, 5, 7, 17];

    for (cnt of counts) {
        let items = alloc.alloc(string, cnt) catch unreachable;
        defer alloc.free(items);
        let bufs = alloc.alloc([16]u8, cnt) catch unreachable;
        defer alloc.free(bufs);

        for (i of 0..cnt) {
            const s = std.fmt.bufPrint(&bufs[i], "leaf-{d}", .{i}) catch unreachable;
            items[i] = s;
        }

        let t1 = MerkleTree.build(alloc, items) catch unreachable;
        defer t1.deinit();
        let t2 = MerkleTree.build(alloc, items) catch unreachable;
        defer t2.deinit();

        expect_eq(t1.leafCount(), cnt);
        expect_eq(std.mem.eql(u8, &t1.rootHash(), &t2.rootHash()), true);

        const hex1 = t1.rootHex();
        const hex2 = t2.rootHex();
        expect_eq(hex1.len, 64);
        expect_eq(std.mem.eql(u8, &hex1, &hex2), true);
    }

    // Domain separation anti-second-preimage test:
    // 2-leaf tree root = H(0x01 || L0 || L1)
    // 1-leaf tree with 64-byte payload (L0 || L1) root = H(0x00 || L0 || L1)
    const two_items: [2]string = ["alpha", "beta"];
    let two_tree = MerkleTree.build(alloc, &two_items) catch unreachable;
    defer two_tree.deinit();

    const l0 = hashLeaf("alpha");
    const l1 = hashLeaf("beta");
    let forged_leaf: [64]u8 = undefined;
    @memcpy(forged_leaf[0..32], &l0);
    @memcpy(forged_leaf[32..64], &l1);

    const one_item: [1]string = [forged_leaf[0..64]];
    let one_tree = MerkleTree.build(alloc, &one_item) catch unreachable;
    defer one_tree.deinit();

    expect_eq(std.mem.eql(u8, &two_tree.rootHash(), &one_tree.rootHash()), false);
}

test("cap:merkle - generateProof & verifyProof exhaustive traversal (N=17)") {
    const alloc = std.testing.allocator;
    const n: usize = 17;

    let items = alloc.alloc(string, n) catch unreachable;
    defer alloc.free(items);
    let bufs = alloc.alloc([24]u8, n) catch unreachable;
    defer alloc.free(bufs);

    for (i of 0..n) {
        items[i] = std.fmt.bufPrint(&bufs[i], "chunk-payload-{d}", .{i}) catch unreachable;
    }

    let tree = MerkleTree.build(alloc, items) catch unreachable;
    defer tree.deinit();
    const root = tree.rootHash();

    for (i of 0..n) {
        let proof = tree.generateProof(alloc, i) catch unreachable;
        defer proof.deinit();

        expect_eq(proof.leaf_index, i);
        expect_eq(proof.leaf_count, n);
        expect_eq(MerkleTree.verifyProof(root, items[i], &proof), true);
        expect_eq(verifyProof(root, items[i], &proof), true);
    }

    // Also verify single-leaf tree proof (0 steps)
    const single: [1]string = ["solo"];
    let solo_tree = MerkleTree.build(alloc, &single) catch unreachable;
    defer solo_tree.deinit();
    let solo_proof = solo_tree.generateProof(alloc, 0) catch unreachable;
    defer solo_proof.deinit();
    expect_eq(solo_proof.steps.len, 0);
    expect_eq(MerkleTree.verifyProof(solo_tree.rootHash(), "solo", &solo_proof), true);
}

test("cap:merkle - tamper resistance & EmptyTree / IndexOutOfBounds bounds") {
    const alloc = std.testing.allocator;

    // EmptyTree check
    expect_eq(expectBuildEmpty(alloc), true);

    const items: [5]string = ["d0", "d1", "d2", "d3", "d4"];
    let tree = MerkleTree.build(alloc, &items) catch unreachable;
    defer tree.deinit();
    const root = tree.rootHash();

    // IndexOutOfBounds check
    expect_eq(expectOobProof(alloc, &tree, 5), true);

    // Tamper 1: wrong leaf_data
    let proof = tree.generateProof(alloc, 2) catch unreachable;
    defer proof.deinit();
    expect_eq(MerkleTree.verifyProof(root, "d2-tampered", &proof), false);

    // Tamper 2: wrong root_hash
    let bad_root = root;
    bad_root[0] ^= 0xFF;
    expect_eq(MerkleTree.verifyProof(bad_root, "d2", &proof), false);

    // Tamper 3: mutated sibling_hash
    if (proof.steps.len > 0) {
        const orig_byte = proof.steps[0].sibling_hash[0];
        proof.steps[0].sibling_hash[0] = orig_byte ^ 0x01;
        expect_eq(MerkleTree.verifyProof(root, "d2", &proof), false);
        proof.steps[0].sibling_hash[0] = orig_byte;
    }

    // Tamper 4: forged sibling_side
    if (proof.steps.len > 0) {
        const orig_side = proof.steps[0].sibling_side;
        proof.steps[0].sibling_side = if (orig_side == ProofSide.Left) ProofSide.Right else ProofSide.Left;
        expect_eq(MerkleTree.verifyProof(root, "d2", &proof), false);
        proof.steps[0].sibling_side = orig_side;
    }

    // Restored proof still passes
    expect_eq(MerkleTree.verifyProof(root, "d2", &proof), true);
}

test("cap:merkle - diffTrees dual-tree fast diffing & zero memory leaks") {
    const alloc = std.testing.allocator;
    const n: usize = 100;

    let items_a = alloc.alloc(string, n) catch unreachable;
    defer alloc.free(items_a);
    let items_b = alloc.alloc(string, n) catch unreachable;
    defer alloc.free(items_b);
    let bufs_a = alloc.alloc([24]u8, n) catch unreachable;
    defer alloc.free(bufs_a);
    let bufs_b = alloc.alloc([24]u8, n) catch unreachable;
    defer alloc.free(bufs_b);

    for (i of 0..n) {
        items_a[i] = std.fmt.bufPrint(&bufs_a[i], "block-{d}", .{i}) catch unreachable;
        items_b[i] = std.fmt.bufPrint(&bufs_b[i], "block-{d}", .{i}) catch unreachable;
    }

    // Modify only index 13 and 77
    items_b[13] = "block-13-modified";
    items_b[77] = "block-77-modified";

    let tree_a = MerkleTree.build(alloc, items_a) catch unreachable;
    defer tree_a.deinit();
    let tree_b = MerkleTree.build(alloc, items_b) catch unreachable;
    defer tree_b.deinit();

    // Same-tree diff should be empty
    const same_diff = MerkleTree.diffTrees(alloc, &tree_a, &tree_a) catch unreachable;
    defer alloc.free(same_diff);
    expect_eq(same_diff.len, 0);

    // 100-block diff with 2 edits should return exactly [13, 77]
    const diffs = diffTrees(alloc, &tree_a, &tree_b) catch unreachable;
    defer alloc.free(diffs);
    expect_eq(diffs.len, 2);
    expect_eq(diffs[0], 13);
    expect_eq(diffs[1], 77);

    // Cross-size diff: 100 blocks vs 103 blocks (index 5 changed + 100, 101, 102 added)
    let items_c = alloc.alloc(string, 103) catch unreachable;
    defer alloc.free(items_c);
    for (i of 0..100) {
        items_c[i] = items_a[i];
    }
    items_c[5] = "block-5-edited";
    items_c[100] = "block-100-new";
    items_c[101] = "block-101-new";
    items_c[102] = "block-102-new";

    let tree_c = MerkleTree.build(alloc, items_c) catch unreachable;
    defer tree_c.deinit();

    const cross_diffs = MerkleTree.diffTrees(alloc, &tree_a, &tree_c) catch unreachable;
    defer alloc.free(cross_diffs);
    expect_eq(cross_diffs.len, 4);
    expect_eq(cross_diffs[0], 5);
    expect_eq(cross_diffs[1], 100);
    expect_eq(cross_diffs[2], 101);
    expect_eq(cross_diffs[3], 102);
}

test("cap:merkle - import cap from \"cap\" dynamic mounting (cap.merkle)") {
    const alloc = std.testing.allocator;
    const items: [4]string = ["m0", "m1", "m2", "m3"];

    let tree = cap.merkle.MerkleTree.build(alloc, &items) catch unreachable;
    defer tree.deinit();

    expect_eq(tree.leafCount(), 4);
    let proof = tree.generateProof(alloc, 3) catch unreachable;
    defer proof.deinit();

    expect_eq(cap.merkle.MerkleTree.verifyProof(tree.rootHash(), "m3", &proof), true);
}
