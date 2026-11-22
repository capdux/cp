// Origin: Pure-CP Stdlib Test Suite (lib/attention/guided_trie.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc Guided Decoding Prefix-Trie & Logit Masker:
//   1. Common prefix sharing, node reuse & stats accounting (node_count, sequence_count, max_depth)
//   2. Multi-branch next-token enumeration & caller output buffer capacity cap
//   3. Dual terminal+intermediate prefix nodes (coexisting EOS + child branches, post-hoc prefix terminal, EOS dedup)
//   4. End-to-end apply_logit_mask_f32 + greedy argMax autoregressive decoding with adversarial high-logit distractors
//   5. Invalid token transition -> is_dead interception (advance, allowed_next_tokens, apply_logit_mask_f32)
//   6. Duplicate insertion idempotence & empty sequence rejection
//   7. Out-of-bounds token_id (>= logits.len) & empty logits slice safety guards
//   8. 512-node capacity limit & atomic rollback-free overflow guard

import { test, expect_eq } from "cap:test";
import {
    GUIDED_TRIE_MAX_NODES,
    GUIDED_TRIE_NO_NODE,
    GuidedTrieCursor,
    GuidedTrieStats,
    GuidedTokenTrie,
} from "cap:attention";

fn argMaxF32(logits: []const f32): usize {
    let best_idx: usize = 0;
    let best_val: f32 = logits[0];
    let i: usize = 1;
    while (i < logits.len) : (i += 1) {
        if (logits[i] > best_val) {
            best_val = logits[i];
            best_idx = i;
        }
    }
    return best_idx;
}

fn containsU32(slice: []const u32, target: u32): bool {
    let i: usize = 0;
    while (i < slice.len) : (i += 1) {
        if (slice[i] == target) return true;
    }
    return false;
}

// ============================================================================
// Test 1: Common Prefix Sharing, Node Reuse & Stats Accounting
// ============================================================================
test("guided_trie: common prefix sharing & node reuse", () => {
    let trie = GuidedTokenTrie.init();
    const s0: GuidedTrieStats = trie.stats();
    expect_eq(s0.node_count, 1);
    expect_eq(s0.sequence_count, 0);
    expect_eq(s0.max_depth, 0);

    // Insert [10, 20, 30] -> adds 3 nodes (total 4 including root)
    const seq1: [3]u32 = [10, 20, 30];
    expect_eq(trie.insert_sequence(seq1[0..]), true);
    const s1 = trie.stats();
    expect_eq(s1.node_count, 4);
    expect_eq(s1.sequence_count, 1);
    expect_eq(s1.max_depth, 3);

    // Insert [10, 20, 40] -> reuses [10, 20], adds only 1 node (total 5)
    const seq2: [3]u32 = [10, 20, 40];
    expect_eq(trie.insert_sequence(seq2[0..]), true);
    const s2 = trie.stats();
    expect_eq(s2.node_count, 5);
    expect_eq(s2.sequence_count, 2);
    expect_eq(s2.max_depth, 3);

    // Insert [10, 50, 60, 70] -> reuses [10], adds 3 nodes (total 8), max_depth becomes 4
    const seq3: [4]u32 = [10, 50, 60, 70];
    expect_eq(trie.insert_sequence(seq3[0..]), true);
    const s3 = trie.stats();
    expect_eq(s3.node_count, 8);
    expect_eq(s3.sequence_count, 3);
    expect_eq(s3.max_depth, 4);

    // Verify cursor traversal reaches terminal with exact step_count
    let cur = trie.new_cursor();
    expect_eq(cur.node_idx, 0);
    expect_eq(cur.step_count, 0);
    expect_eq(cur.is_complete, false);
    expect_eq(cur.is_dead, false);

    cur = trie.advance(cur, 10);
    expect_eq(cur.step_count, 1);
    expect_eq(cur.is_complete, false);
    expect_eq(cur.is_dead, false);

    cur = trie.advance(cur, 20);
    expect_eq(cur.step_count, 2);
    expect_eq(cur.is_complete, false);

    cur = trie.advance(cur, 40);
    expect_eq(cur.step_count, 3);
    expect_eq(cur.is_complete, true);
    expect_eq(cur.is_dead, false);
});

// ============================================================================
// Test 2: Multi-Branch Next-Token Enumeration & Output Buffer Cap
// ============================================================================
test("guided_trie: multi-branch allowed_next_tokens & buffer cap", () => {
    let trie = GuidedTokenTrie.init();
    const eos_id: u32 = 2;

    const a: [2]u32 = [5, 11];
    const b: [2]u32 = [5, 12];
    const c: [2]u32 = [5, 13];
    const d: [2]u32 = [8, 21];
    expect_eq(trie.insert_sequence(a[0..]), true);
    expect_eq(trie.insert_sequence(b[0..]), true);
    expect_eq(trie.insert_sequence(c[0..]), true);
    expect_eq(trie.insert_sequence(d[0..]), true);

    // At root: allowed next tokens are {5, 8} (not terminal -> no EOS)
    let out_buf: [8]u32 = [0; 8];
    let out_s: []u32 = out_buf[0..];
    const root_cur = trie.new_cursor();
    const n_root = trie.allowed_next_tokens(root_cur, eos_id, out_s);
    expect_eq(n_root, 2);
    expect_eq(containsU32(out_buf[0..n_root], 5), true);
    expect_eq(containsU32(out_buf[0..n_root], 8), true);
    expect_eq(containsU32(out_buf[0..n_root], eos_id), false);

    // After advancing by 5: allowed next tokens are {11, 12, 13}
    const c5 = trie.advance(root_cur, 5);
    expect_eq(c5.is_complete, false);
    const n_c5 = trie.allowed_next_tokens(c5, eos_id, out_s);
    expect_eq(n_c5, 3);
    expect_eq(containsU32(out_buf[0..n_c5], 11), true);
    expect_eq(containsU32(out_buf[0..n_c5], 12), true);
    expect_eq(containsU32(out_buf[0..n_c5], 13), true);

    // Small caller buffer (len = 2) caps cleanly without overflow
    let tiny_buf: [2]u32 = [0; 2];
    let tiny_s: []u32 = tiny_buf[0..];
    const n_tiny = trie.allowed_next_tokens(c5, eos_id, tiny_s);
    expect_eq(n_tiny, 2);
    expect_eq(tiny_buf[0], 11);
    expect_eq(tiny_buf[1], 12);
});

// ============================================================================
// Test 3: Prefix Is Both Terminal & Intermediate (EOS + Branches Coexist)
// ============================================================================
test("guided_trie: dual terminal+intermediate prefix coexists with EOS & branches", () => {
    let trie = GuidedTokenTrie.init();
    const eos_id: u32 = 2;

    // Insert longer sequence first, then shorter prefix sequence
    const long_seq: [3]u32 = [10, 20, 30];
    const alt_long: [3]u32 = [10, 20, 35];
    const short_prefix: [2]u32 = [10, 20];
    expect_eq(trie.insert_sequence(long_seq[0..]), true);
    expect_eq(trie.insert_sequence(alt_long[0..]), true);
    const nodes_before = trie.stats().node_count;

    // Inserting prefix [10, 20] reuses existing node, increments sequence_count only
    expect_eq(trie.insert_sequence(short_prefix[0..]), true);
    expect_eq(trie.stats().node_count, nodes_before);
    expect_eq(trie.stats().sequence_count, 3);

    let cur = trie.new_cursor();
    cur = trie.advance(cur, 10);
    cur = trie.advance(cur, 20);
    expect_eq(cur.is_complete, true);
    expect_eq(cur.is_dead, false);

    // Allowed next tokens must contain {30, 35, eos_id(2)}
    let out_buf: [8]u32 = [0; 8];
    let out_s: []u32 = out_buf[0..];
    const n = trie.allowed_next_tokens(cur, eos_id, out_s);
    expect_eq(n, 3);
    expect_eq(containsU32(out_buf[0..n], 30), true);
    expect_eq(containsU32(out_buf[0..n], 35), true);
    expect_eq(containsU32(out_buf[0..n], eos_id), true);

    // At leaf [10, 20, 30]: only eos_id is allowed
    const leaf = trie.advance(cur, 30);
    expect_eq(leaf.is_complete, true);
    const n_leaf = trie.allowed_next_tokens(leaf, eos_id, out_s);
    expect_eq(n_leaf, 1);
    expect_eq(out_buf[0], eos_id);

    // EOS deduplication: if eos_id itself is also a child token, do not duplicate it
    const with_eos_child: [3]u32 = [10, 20, eos_id];
    expect_eq(trie.insert_sequence(with_eos_child[0..]), true);
    const n_dedup = trie.allowed_next_tokens(cur, eos_id, out_s);
    expect_eq(n_dedup, 3); // {30, 35, 2} — no duplicate 2
});

// ============================================================================
// Test 4: apply_logit_mask_f32 + End-to-End Greedy argMax Guided Decoding
// ============================================================================
test("guided_trie: apply_logit_mask_f32 keeps greedy argMax 100% inside valid sequences", () => {
    let trie = GuidedTokenTrie.init();
    const eos_id: u32 = 1;

    // Two valid tool-call token sequences: [4, 7, 12] and [4, 9, 15]
    const seq_a: [3]u32 = [4, 7, 12];
    const seq_b: [3]u32 = [4, 9, 15];
    expect_eq(trie.insert_sequence(seq_a[0..]), true);
    expect_eq(trie.insert_sequence(seq_b[0..]), true);

    let cur = trie.new_cursor();
    let generated: [4]u32 = [0; 4];
    let gen_len: usize = 0;

    // Step 0: distractor token 0 has logit +100.0, valid token 4 has logit -2.5
    {
        let logits_buf: [32]f32 = [0.0; 32];
        logits_buf[0] = 100.0;
        logits_buf[4] = -2.5;
        logits_buf[31] = 50.0;
        let logits: []f32 = logits_buf[0..];
        const allowed = trie.apply_logit_mask_f32(cur, eos_id, logits);
        expect_eq(allowed, 1);
        expect_eq(logits_buf[4], -2.5);
        expect_eq(logits_buf[0] < -1.0e29, true);
        expect_eq(logits_buf[31] < -1.0e29, true);
        const picked: u32 = @intCast(argMaxF32(logits_buf[0..]));
        expect_eq(picked, 4);
        generated[gen_len] = picked;
        gen_len += 1;
        cur = trie.advance(cur, picked);
    }

    // Step 1: valid branches {7, 9}; token 9 has higher logit than 7, distractor 20 has +999.0
    {
        let logits_buf: [32]f32 = [1.0; 32];
        logits_buf[7] = 1.25;
        logits_buf[9] = 3.75;
        logits_buf[20] = 999.0;
        let logits: []f32 = logits_buf[0..];
        const allowed = trie.apply_logit_mask_f32(cur, eos_id, logits);
        expect_eq(allowed, 2);
        expect_eq(logits_buf[7], 1.25);
        expect_eq(logits_buf[9], 3.75);
        expect_eq(logits_buf[20] < -1.0e29, true);
        const picked: u32 = @intCast(argMaxF32(logits_buf[0..]));
        expect_eq(picked, 9);
        generated[gen_len] = picked;
        gen_len += 1;
        cur = trie.advance(cur, picked);
    }

    // Step 2: only token 15 is valid
    {
        let logits_buf: [32]f32 = [5.0; 32];
        logits_buf[15] = -0.5;
        let logits: []f32 = logits_buf[0..];
        const allowed = trie.apply_logit_mask_f32(cur, eos_id, logits);
        expect_eq(allowed, 1);
        expect_eq(logits_buf[15], -0.5);
        const picked: u32 = @intCast(argMaxF32(logits_buf[0..]));
        expect_eq(picked, 15);
        generated[gen_len] = picked;
        gen_len += 1;
        cur = trie.advance(cur, picked);
        expect_eq(cur.is_complete, true);
    }

    // Step 3: terminal leaf -> only eos_id (1) is preserved
    {
        let logits_buf: [32]f32 = [10.0; 32];
        logits_buf[1] = 0.25;
        let logits: []f32 = logits_buf[0..];
        const allowed = trie.apply_logit_mask_f32(cur, eos_id, logits);
        expect_eq(allowed, 1);
        expect_eq(logits_buf[1], 0.25);
        expect_eq(logits_buf[0] < -1.0e29, true);
        const picked: u32 = @intCast(argMaxF32(logits_buf[0..]));
        expect_eq(picked, eos_id);
        generated[gen_len] = picked;
        gen_len += 1;
    }

    // Final decoded sequence is exactly [4, 9, 15, EOS(1)]
    expect_eq(gen_len, 4);
    expect_eq(generated[0], 4);
    expect_eq(generated[1], 9);
    expect_eq(generated[2], 15);
    expect_eq(generated[3], eos_id);
});

// ============================================================================
// Test 5: Invalid Token Triggers is_dead Interception
// ============================================================================
test("guided_trie: invalid token triggers is_dead state & blocks all logits", () => {
    let trie = GuidedTokenTrie.init();
    const eos_id: u32 = 2;
    const seq: [2]u32 = [10, 20];
    expect_eq(trie.insert_sequence(seq[0..]), true);

    const root = trie.new_cursor();
    const c1 = trie.advance(root, 10);
    expect_eq(c1.is_dead, false);

    // Advance with invalid token 99 -> dead cursor
    const dead = trie.advance(c1, 99);
    expect_eq(dead.is_dead, true);
    expect_eq(dead.is_complete, false);
    expect_eq(dead.node_idx, GUIDED_TRIE_NO_NODE);
    expect_eq(dead.step_count, 1);

    // Advancing a dead cursor stays dead even with a valid root token
    const still_dead = trie.advance(dead, 10);
    expect_eq(still_dead.is_dead, true);
    expect_eq(still_dead.node_idx, GUIDED_TRIE_NO_NODE);

    // allowed_next_tokens returns 0 on dead cursor
    let out_buf: [4]u32 = [77; 4];
    let out_s: []u32 = out_buf[0..];
    expect_eq(trie.allowed_next_tokens(dead, eos_id, out_s), 0);
    expect_eq(out_buf[0], 77); // untouched

    // apply_logit_mask_f32 masks every single vocab entry and returns 0
    let logits_buf: [16]f32 = [3.5; 16];
    let logits: []f32 = logits_buf[0..];
    const allowed = trie.apply_logit_mask_f32(dead, eos_id, logits);
    expect_eq(allowed, 0);
    let i: usize = 0;
    while (i < logits_buf.len) : (i += 1) {
        expect_eq(logits_buf[i] < -1.0e29, true);
    }
});

// ============================================================================
// Test 6: Duplicate Insertion Idempotence & Empty Sequence Rejection
// ============================================================================
test("guided_trie: duplicate insertion idempotence & empty sequence guard", () => {
    let trie = GuidedTokenTrie.init();

    // Empty sequence must be rejected without mutating trie
    const dummy: [1]u32 = [42];
    const empty_seq: []const u32 = dummy[0..0];
    expect_eq(trie.insert_sequence(empty_seq), false);
    expect_eq(trie.stats().node_count, 1);
    expect_eq(trie.stats().sequence_count, 0);

    // Insert [7, 8, 9] twice -> idempotent
    const seq: [3]u32 = [7, 8, 9];
    expect_eq(trie.insert_sequence(seq[0..]), true);
    expect_eq(trie.stats().node_count, 4);
    expect_eq(trie.stats().sequence_count, 1);

    expect_eq(trie.insert_sequence(seq[0..]), true);
    expect_eq(trie.stats().node_count, 4);
    expect_eq(trie.stats().sequence_count, 1);
    expect_eq(trie.stats().max_depth, 3);
});

// ============================================================================
// Test 7: Out-of-Bounds Token IDs & Empty Logits Slice Safety Guards
// ============================================================================
test("guided_trie: out-of-bounds token_id & empty logits guards", () => {
    let trie = GuidedTokenTrie.init();
    const eos_id: u32 = 99999; // deliberately larger than vocab_size

    const in_bounds: [1]u32 = [3];
    const out_of_bounds: [1]u32 = [50000];
    expect_eq(trie.insert_sequence(in_bounds[0..]), true);
    expect_eq(trie.insert_sequence(out_of_bounds[0..]), true);

    // Vocab size = 8: token 50000 is ignored safely, token 3 is preserved
    let logits_buf: [8]f32 = [2.0; 8];
    logits_buf[3] = 4.25;
    let logits: []f32 = logits_buf[0..];
    const allowed = trie.apply_logit_mask_f32(trie.new_cursor(), eos_id, logits);
    expect_eq(allowed, 1);
    expect_eq(logits_buf[3], 4.25);
    expect_eq(logits_buf[0] < -1.0e29, true);
    expect_eq(logits_buf[7] < -1.0e29, true);

    // Complete cursor with out-of-bounds eos_id: safely returns 0 and masks all
    const term_cur = trie.advance(trie.new_cursor(), 3);
    expect_eq(term_cur.is_complete, true);
    let logits2_buf: [8]f32 = [1.0; 8];
    let logits2: []f32 = logits2_buf[0..];
    expect_eq(trie.apply_logit_mask_f32(term_cur, eos_id, logits2), 0);

    // Empty logits slice -> returns 0 safely
    let empty_logits: []f32 = logits_buf[0..0];
    expect_eq(trie.apply_logit_mask_f32(trie.new_cursor(), 1, empty_logits), 0);
});

// ============================================================================
// Test 8: 512-Node Capacity Limit & Atomic Overflow Rollback-Free Guard
// ============================================================================
test("guided_trie: 512-node capacity limit & atomic overflow guard", () => {
    let trie = GuidedTokenTrie.init();
    expect_eq(GUIDED_TRIE_MAX_NODES, 512);

    // Root uses 1 node. Insert 509 single-token sequences -> node_count = 510 (2 slots left).
    let tok: u32 = 1;
    while (tok <= 509) : (tok += 1) {
        const single: [1]u32 = [tok];
        expect_eq(trie.insert_sequence(single[0..]), true);
    }
    expect_eq(trie.stats().node_count, 510);
    expect_eq(trie.stats().sequence_count, 509);

    // Attempt to insert a 3-node sequence [1, 1001, 1002, 1003] (reuses [1], needs 3 > 2 left).
    // Must fail atomically without leaving partial nodes!
    const too_long: [4]u32 = [1, 1001, 1002, 1003];
    expect_eq(trie.insert_sequence(too_long[0..]), false);
    expect_eq(trie.stats().node_count, 510);
    expect_eq(trie.stats().sequence_count, 509);

    // Verify partial child 1001 was NOT left attached under token 1
    const c1 = trie.advance(trie.new_cursor(), 1);
    const ghost = trie.advance(c1, 1001);
    expect_eq(ghost.is_dead, true);

    // Exact-fit 2-node suffix [1, 2001, 2002] -> fills node_count to exactly 512
    const exact_fit: [3]u32 = [1, 2001, 2002];
    expect_eq(trie.insert_sequence(exact_fit[0..]), true);
    expect_eq(trie.stats().node_count, GUIDED_TRIE_MAX_NODES);
    expect_eq(trie.stats().sequence_count, 510);

    // Any further sequence requiring new nodes must fail safely
    const overflow_seq: [1]u32 = [9999];
    expect_eq(trie.insert_sequence(overflow_seq[0..]), false);
    expect_eq(trie.stats().node_count, GUIDED_TRIE_MAX_NODES);

    // Marking an existing non-terminal prefix [1, 2001] as terminal requires 0 new nodes -> succeeds!
    const existing_prefix: [2]u32 = [1, 2001];
    expect_eq(trie.insert_sequence(existing_prefix[0..]), true);
    expect_eq(trie.stats().node_count, GUIDED_TRIE_MAX_NODES);
    expect_eq(trie.stats().sequence_count, 511);
});
