// Origin: Pure-CP Stdlib Test Suite (lib/crypto/merkleMmr.cp) | Created: 2026-11-23
// Deep production regression tests for cap:crypto Merkle Mountain Range (`lib/crypto/merkleMmr.cp`):
//   1. Empty, single-leaf, and power-of-two exact domain-separated SHA-256 root checks
//   2. Non-power-of-two multi-peak topology (7 leaves -> 3 peaks heights 2/1/0) & right-to-left bagging
//   3. Exhaustive O(log N) inclusion proofs for every single leaf in an 11-leaf multi-mountain MMR
//   4. Tamper detection: modified leaf payload, sibling hash, sibling side, peak bag & leaf_pos
//   5. Historical prefix snapshot consistency (`rootAtLeafCount` / `verifyPrefixRoot`) across 1..15 leaves
//   6. Full 128-leaf capacity (255 nodes, height 7), overflow guard, and depth-7 proofs
//   7. Zero-heap `MmrStats`, `clear()`, and `cap.crypto` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    MMR_MAX_LEAVES,
    mmrConstantTimeEq,
    mmrHashEmpty,
    mmrHashLeaf,
    mmrHashParent,
    mmrHashBag,
    MmrPeak,
    MerkleMmr,
    verifyMmrProof,
} from "cap:crypto";

test("merkleMmr: empty, single-leaf, and power-of-two exact domain-separated SHA-256 roots") {
    let mmr = MerkleMmr.init();
    expect_true(mmrConstantTimeEq(mmr.bagPeaks(), mmrHashEmpty()));
    expect_eq(mmr.leafCount(), @as(usize, 0));
    expect_eq(mmr.nodeCount(), @as(usize, 0));
    expect_eq(mmr.peakCount(), @as(usize, 0));

    // 1 leaf -> pos 0, single peak == leaf hash
    const p0 = mmr.appendLeaf("leaf-0") ?? 999;
    expect_eq(p0, @as(u64, 0));
    expect_eq(mmr.peakCount(), @as(usize, 1));
    const h0 = mmrHashLeaf(0, "leaf-0");
    expect_true(mmrConstantTimeEq(mmr.bagPeaks(), h0));

    // 2 leaves -> pos 1, merges into parent pos 2 (single peak)
    const p1 = mmr.appendLeaf("leaf-1") ?? 999;
    expect_eq(p1, @as(u64, 1));
    expect_eq(mmr.nodeCount(), @as(usize, 3));
    expect_eq(mmr.peakCount(), @as(usize, 1));
    const h1 = mmrHashLeaf(1, "leaf-1");
    const h2 = mmrHashParent(2, h0, h1);
    expect_true(mmrConstantTimeEq(mmr.bagPeaks(), h2));

    // 4 leaves -> full binary tree of 7 nodes, single peak at pos 6
    expect_eq(mmr.appendLeaf("leaf-2") ?? 999, @as(u64, 3));
    expect_eq(mmr.appendLeaf("leaf-3") ?? 999, @as(u64, 4));
    expect_eq(mmr.nodeCount(), @as(usize, 7));
    expect_eq(mmr.peakCount(), @as(usize, 1));
    const h5 = mmrHashParent(5, mmrHashLeaf(3, "leaf-2"), mmrHashLeaf(4, "leaf-3"));
    const h6 = mmrHashParent(6, h2, h5);
    expect_true(mmrConstantTimeEq(mmr.bagPeaks(), h6));
}

test("merkleMmr: non-power-of-two multi-peak topology (7 leaves -> 3 peaks) and right-to-left bagging") {
    let mmr = MerkleMmr.init();
    let buf: [2]u8 = ['L', '0'];
    let k: usize = 0;
    while (k < 7) {
        buf[1] = @as(u8, '0') + @as(u8, @intCast(k));
        expect_true(mmr.appendLeaf(buf[0..]) != null);
        k += 1;
    }

    // 7 = 4 + 2 + 1 => 3 peaks at pos 6 (h=2), pos 9 (h=1), pos 10 (h=0); total nodes = 11
    expect_eq(mmr.leafCount(), @as(usize, 7));
    expect_eq(mmr.nodeCount(), @as(usize, 11));
    expect_eq(mmr.peakCount(), @as(usize, 3));

    let pk_arr: [4]MmrPeak = [MmrPeak{}; 4];
    let pk_s: []MmrPeak = pk_arr[0..];
    const pk_n = mmr.peaks(*pk_s);
    expect_eq(pk_n, @as(usize, 3));
    expect_eq(pk_arr[0].pos, @as(u64, 6));
    expect_eq(pk_arr[0].height, @as(u8, 2));
    expect_eq(pk_arr[1].pos, @as(u64, 9));
    expect_eq(pk_arr[1].height, @as(u8, 1));
    expect_eq(pk_arr[2].pos, @as(u64, 10));
    expect_eq(pk_arr[2].height, @as(u8, 0));

    // Manual right-to-left bagging must match bagPeaks()
    const bag_right = mmrHashBag(11, pk_arr[1].hash, pk_arr[2].hash);
    const expected_root = mmrHashBag(11, pk_arr[0].hash, bag_right);
    expect_true(mmrConstantTimeEq(mmr.bagPeaks(), expected_root));
}

test("merkleMmr: exhaustive O(log N) inclusion proofs for all 11 leaves across 3 mountains") {
    let mmr = MerkleMmr.init();
    let k: usize = 0;
    while (k < 11) {
        let payload: [3]u8 = ['e', @as(u8, '0') + @as(u8, @intCast(k / 10)), @as(u8, '0') + @as(u8, @intCast(k % 10))];
        expect_true(mmr.appendLeaf(payload[0..]) != null);
        k += 1;
    }

    // 11 = 8 + 2 + 1 => 3 peaks (heights 3, 1, 0)
    expect_eq(mmr.peakCount(), @as(usize, 3));
    const root = mmr.bagPeaks();

    let idx: usize = 0;
    while (idx < 11) {
        let payload: [3]u8 = ['e', @as(u8, '0') + @as(u8, @intCast(idx / 10)), @as(u8, '0') + @as(u8, @intCast(idx % 10))];
        if (proof := mmr.generateProof(idx)) {
            expect_true(verifyMmrProof(root, proof, payload[0..]));
        } else {
            expect_true(false);
        }
        idx += 1;
    }

    // Out-of-bounds leaf index returns null
    expect_true(mmr.generateProof(11) == null);
}

test("merkleMmr: tamper detection rejects modified leaf, sibling hash, direction, peak bag & position") {
    let mmr = MerkleMmr.init();
    * = mmr.appendLeaf("alpha");
    * = mmr.appendLeaf("beta");
    * = mmr.appendLeaf("gamma");
    * = mmr.appendLeaf("delta");
    * = mmr.appendLeaf("epsilon");
    const root = mmr.bagPeaks();

    // Leaf 1 ("beta") is inside the left height-2 mountain and has a right peak ("epsilon")
    const base_proof = mmr.generateProof(1) ?? return;
    expect_true(verifyMmrProof(root, base_proof, "beta"));

    // 1. Tampered leaf payload
    expect_eq(verifyMmrProof(root, base_proof, "betA"), false);

    // 2. Tampered sibling hash
    let bad_sib = base_proof;
    bad_sib.sibling_hashes[0][0] ^= 0x01;
    expect_eq(verifyMmrProof(root, bad_sib, "beta"), false);

    // 3. Flipped sibling side
    let bad_side = base_proof;
    bad_side.sibling_is_right[0] = !bad_side.sibling_is_right[0];
    expect_eq(verifyMmrProof(root, bad_side, "beta"), false);

    // 4. Tampered right peak bag
    let bad_bag = base_proof;
    bad_bag.right_bag_hash[31] ^= 0x80;
    expect_eq(verifyMmrProof(root, bad_bag, "beta"), false);

    // 5. Tampered leaf_pos
    let bad_pos = base_proof;
    bad_pos.leaf_pos += 1;
    expect_eq(verifyMmrProof(root, bad_pos, "beta"), false);
}

test("merkleMmr: historical prefix root consistency across 1..15 appended leaves") {
    let mmr = MerkleMmr.init();
    let hist_roots: [15][32]u8 = [[0; 32]; 15];

    let k: usize = 0;
    while (k < 15) {
        let msg: [3]u8 = ['h', @as(u8, '0') + @as(u8, @intCast(k / 10)), @as(u8, '0') + @as(u8, @intCast(k % 10))];
        * = mmr.appendLeaf(msg[0..]);
        hist_roots[k] = mmr.bagPeaks();
        k += 1;
    }

    // Verify every single historical prefix root from 1 to 15 leaves
    let p: usize = 1;
    while (p <= 15) {
        expect_true(mmr.verifyPrefixRoot(p, hist_roots[p - 1]));
        p += 1;
    }

    // Tampered historical root fails
    let forged = hist_roots[6];
    forged[0] ^= 0xFF;
    expect_eq(mmr.verifyPrefixRoot(7, forged), false);

    // Out-of-bounds prefix returns null / false
    expect_true(mmr.rootAtLeafCount(16) == null);
    expect_eq(mmr.verifyPrefixRoot(16, hist_roots[14]), false);
}

test("merkleMmr: full 128-leaf capacity (255 nodes, height 7), overflow guard & depth-7 proofs") {
    let mmr = MerkleMmr.init();
    let k: usize = 0;
    while (k < MMR_MAX_LEAVES) {
        let tag: [3]u8 = [
            @as(u8, '0') + @as(u8, @intCast((k / 100) % 10)),
            @as(u8, '0') + @as(u8, @intCast((k / 10) % 10)),
            @as(u8, '0') + @as(u8, @intCast(k % 10)),
        ];
        expect_true(mmr.appendLeaf(tag[0..]) != null);
        k += 1;
    }

    // 128th leaf completes a single height-7 mountain of 255 nodes; 129th rejected
    expect_eq(mmr.leafCount(), MMR_MAX_LEAVES);
    expect_eq(mmr.nodeCount(), @as(usize, 255));
    expect_eq(mmr.peakCount(), @as(usize, 1));
    expect_eq(mmr.stats().max_height, @as(u8, 7));
    expect_true(mmr.appendLeaf("overflow") == null);

    // Depth-7 proofs for first, middle, and last leaves all verify cleanly
    const root = mmr.bagPeaks();
    const p_first = mmr.generateProof(0) ?? return;
    const p_mid = mmr.generateProof(63) ?? return;
    const p_last = mmr.generateProof(127) ?? return;
    expect_eq(p_first.sibling_count, @as(usize, 7));
    expect_true(verifyMmrProof(root, p_first, "000"));
    expect_true(verifyMmrProof(root, p_mid, "063"));
    expect_true(verifyMmrProof(root, p_last, "127"));
}

test("merkleMmr: zero-heap stats snapshot, clear, and cap.crypto facade parity") {
    let mmr = cap.crypto.MerkleMmr.init();
    * = mmr.appendLeaf("doc-a");
    * = mmr.appendLeaf("doc-b");
    * = mmr.appendLeaf("doc-c");

    const st = mmr.stats();
    expect_eq(st.leaf_count, @as(usize, 3));
    expect_eq(st.node_count, @as(usize, 4));
    expect_eq(st.peak_count, @as(usize, 2));
    expect_eq(st.max_height, @as(u8, 1));

    const root = mmr.bagPeaks();
    if (proof := mmr.generateProof(2)) {
        expect_true(cap.crypto.verifyMmrProof(root, proof, "doc-c"));
    } else {
        expect_true(false);
    }

    mmr.clear();
    expect_eq(mmr.leafCount(), @as(usize, 0));
    expect_true(cap.crypto.mmrConstantTimeEq(mmr.bagPeaks(), cap.crypto.mmrHashEmpty()));
}
