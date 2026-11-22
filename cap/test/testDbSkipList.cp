// Origin: Pure-CP Stdlib Test Suite (lib/db/skipList.cp) | Created: 2026-11-23
// Deep production regression tests for cap:db deterministic Arena SkipList (`lib/db/skipList.cp`):
//   1. Lexicographic put/get, monotonic version bumps, in-place overwrite (0 arena growth) & realloc update
//   2. Tombstone deletion (existing + standalone missing key) and resurrection via subsequent put
//   3. O(log N) lower-bound `seek` cursor & `[start_key, end_key)` `rangeScan` with tombstone toggle
//   4. Deterministic xorshift64 seed reproducibility & multi-level tower promotion (`current_max_level > 1`)
//   5. Atomic pre-checks on 8 KiB arena exhaustion and 256-node pool saturation (zero partial corruption)
//   6. `shouldFlush(threshold_permille)` trigger & empty/inverted-range boundary guards
//   7. Zero-heap `SkipListStats` telemetry, `clear()` seed restore, and `cap.db` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    SKIPLIST_MAX_NODES,
    SKIPLIST_ARENA_BYTES,
    SKIPLIST_NO_NODE,
    SkipListWriteStatus,
    SkipListGetKind,
    SkipListEntryView,
    ArenaSkipList,
} from "cap:db";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("skipList: lexicographic put/get, in-place overwrite, and monotonic versions") {
    let sl = ArenaSkipList.initWithSeed(12345);

    expect_true(sl.put("delta", "400") == SkipListWriteStatus.ok);
    expect_true(sl.put("alpha", "10000") == SkipListWriteStatus.ok);
    expect_true(sl.put("charlie", "300") == SkipListWriteStatus.ok);
    expect_true(sl.put("bravo", "200") == SkipListWriteStatus.ok);

    const g1 = sl.get("alpha");
    expect_true(g1.kind == SkipListGetKind.found);
    expect_eq(g1.version, @as(u64, 2));
    expect_true(bytesEq(sl.valueSlice(g1.entry), "10000"));

    // In-place overwrite with shorter value consumes 0 extra arena bytes
    const used_before = sl.stats().arena_used_bytes;
    expect_true(sl.put("alpha", "10") == SkipListWriteStatus.ok);
    expect_eq(sl.stats().arena_used_bytes, used_before);
    const g2 = sl.get("alpha");
    expect_true(g2.kind == SkipListGetKind.found);
    expect_eq(g2.version, @as(u64, 5));
    expect_true(bytesEq(sl.valueSlice(g2.entry), "10"));

    // Larger overwrite allocates only the new value bytes (reusing key bytes)
    expect_true(sl.put("alpha", "10000000") == SkipListWriteStatus.ok);
    expect_eq(sl.stats().arena_used_bytes, used_before + 8);
    const g3 = sl.getConst("alpha");
    expect_true(g3.kind == SkipListGetKind.found);
    expect_true(bytesEq(sl.valueSlice(g3.entry), "10000000"));
    expect_eq(sl.stats().live_keys, @as(usize, 4));
}

test("skipList: tombstone delete masking (existing + missing key) and resurrection") {
    let sl = ArenaSkipList.initWithSeed(77);

    expect_true(sl.put("k1", "v1") == SkipListWriteStatus.ok);
    expect_true(sl.put("k2", "v2") == SkipListWriteStatus.ok);

    // Delete existing key k1 -> becomes tombstone, live_keys 2 -> 1, tombstone_keys 0 -> 1
    expect_true(sl.delete("k1") == SkipListWriteStatus.ok);
    const r1 = sl.get("k1");
    expect_true(r1.kind == SkipListGetKind.tombstone);
    expect_true(bytesEq(sl.valueSlice(r1.entry), ""));
    expect_eq(sl.stats().live_keys, @as(usize, 1));
    expect_eq(sl.stats().tombstone_keys, @as(usize, 1));

    // Delete missing key k_absent -> inserts standalone tombstone for LSM lower-level masking
    expect_true(sl.delete("k_absent") == SkipListWriteStatus.ok);
    const ra = sl.get("k_absent");
    expect_true(ra.kind == SkipListGetKind.tombstone);
    expect_eq(sl.stats().tombstone_keys, @as(usize, 2));

    // Resurrect k1 with a new value -> live_keys 1 -> 2, tombstone_keys 2 -> 1
    expect_true(sl.put("k1", "v1_reborn") == SkipListWriteStatus.ok);
    const r1_new = sl.get("k1");
    expect_true(r1_new.kind == SkipListGetKind.found);
    expect_true(bytesEq(sl.valueSlice(r1_new.entry), "v1_reborn"));
    expect_eq(sl.stats().live_keys, @as(usize, 2));
    expect_eq(sl.stats().tombstone_keys, @as(usize, 1));
}

test("skipList: seek forward cursor and lexicographic rangeScan with tombstone toggle") {
    let sl = ArenaSkipList.initWithSeed(99);
    * = sl.put("k05", "5");
    * = sl.put("k01", "1");
    * = sl.put("k03", "3");
    * = sl.put("k04", "4");
    * = sl.put("k02", "2");
    * = sl.delete("k03");

    // Forward cursor via seek("k02") -> k02, k03(tombstone), k04, k05
    let cur: u16 = sl.seek("k02");
    expect_true(cur != SKIPLIST_NO_NODE);
    if (e0 := sl.entryAt(cur)) {
        expect_true(bytesEq(sl.keySlice(e0), "k02"));
    } else {
        expect_true(false);
    }
    cur = sl.nextNode(cur);
    if (e1 := sl.entryAt(cur)) {
        expect_true(bytesEq(sl.keySlice(e1), "k03"));
        expect_true(e1.tombstone);
    } else {
        expect_true(false);
    }

    // rangeScan ["k02", "k05") excluding tombstones => ["k02", "k04"]
    let out: [8]SkipListEntryView = [SkipListEntryView{}; 8];
    let out_s: []SkipListEntryView = out[0..];
    const n_live = sl.rangeScan("k02", "k05", false, *out_s);
    expect_eq(n_live, @as(usize, 2));
    expect_true(bytesEq(sl.keySlice(out[0]), "k02"));
    expect_true(bytesEq(sl.keySlice(out[1]), "k04"));

    // rangeScan ["k02", "k05") including tombstones => ["k02", "k03", "k04"]
    const n_all = sl.rangeScan("k02", "k05", true, *out_s);
    expect_eq(n_all, @as(usize, 3));
    expect_true(bytesEq(sl.keySlice(out[1]), "k03"));
    expect_true(out[1].tombstone);

    // Unbounded scan ("", "") returns all 5 nodes in sorted order
    const n_full = sl.rangeScan("", "", true, *out_s);
    expect_eq(n_full, @as(usize, 5));
    expect_true(bytesEq(sl.keySlice(out[0]), "k01"));
    expect_true(bytesEq(sl.keySlice(out[4]), "k05"));
}

test("skipList: deterministic xorshift64 seed reproducibility and multi-level promotion") {
    let a = ArenaSkipList.initWithSeed(0xDEADBEEF);
    let b = ArenaSkipList.initWithSeed(0xDEADBEEF);

    let key_buf: [4]u8 = ['k', '0', '0', '0'];
    let i: usize = 0;
    while (i < 64) {
        key_buf[1] = @as(u8, '0') + @as(u8, @intCast((i / 100) % 10));
        key_buf[2] = @as(u8, '0') + @as(u8, @intCast((i / 10) % 10));
        key_buf[3] = @as(u8, '0') + @as(u8, @intCast(i % 10));
        expect_true(a.put(key_buf[0..], "val") == SkipListWriteStatus.ok);
        expect_true(b.put(key_buf[0..], "val") == SkipListWriteStatus.ok);
        i += 1;
    }

    const sa = a.stats();
    const sb = b.stats();
    expect_eq(sa.total_nodes, @as(usize, 64));
    expect_eq(sa.current_max_level, sb.current_max_level);
    expect_eq(sa.total_comparisons, sb.total_comparisons);
    expect_eq(sa.arena_used_bytes, sb.arena_used_bytes);
    expect_true(sa.current_max_level > 1);
}

test("skipList: atomic pre-checks on arena exhaustion and 256-node pool saturation") {
    // Part A: Arena exhaustion (8 KiB) leaves no partial state
    let sl_arena = ArenaSkipList.initWithSeed(1);
    let big_val: [1024]u8 = ['X'; 1024];
    let k: u8 = 0;
    while (k < 7) {
        let kb: [2]u8 = ['a', @as(u8, '0') + k];
        expect_true(sl_arena.put(kb[0..], big_val[0..]) == SkipListWriteStatus.ok);
        k += 1;
    }
    // 7 * 1026 = 7182 bytes used; remaining = 1010 < 1026 -> must fail atomically with arena_full
    const used_snap = sl_arena.stats().arena_used_bytes;
    const nodes_snap = sl_arena.stats().total_nodes;
    expect_true(sl_arena.put("z9", big_val[0..]) == SkipListWriteStatus.arena_full);
    expect_eq(sl_arena.stats().arena_used_bytes, used_snap);
    expect_eq(sl_arena.stats().total_nodes, nodes_snap);
    expect_true(sl_arena.get("z9").kind == SkipListGetKind.not_found);

    // Part B: 256-node pool saturation
    let sl_nodes = ArenaSkipList.initWithSeed(2);
    let key3: [3]u8 = ['0', '0', '0'];
    let n: usize = 0;
    while (n < SKIPLIST_MAX_NODES) {
        key3[0] = @as(u8, '0') + @as(u8, @intCast((n / 100) % 10));
        key3[1] = @as(u8, '0') + @as(u8, @intCast((n / 10) % 10));
        key3[2] = @as(u8, '0') + @as(u8, @intCast(n % 10));
        expect_true(sl_nodes.put(key3[0..], "v") == SkipListWriteStatus.ok);
        n += 1;
    }
    expect_eq(sl_nodes.stats().total_nodes, SKIPLIST_MAX_NODES);
    expect_true(sl_nodes.put("999", "overflow") == SkipListWriteStatus.node_pool_full);

    // Existing key can still be overwritten in-place or tombstoned even when node pool is full
    expect_true(sl_nodes.put("000", "w") == SkipListWriteStatus.ok);
    expect_true(sl_nodes.delete("001") == SkipListWriteStatus.ok);
    expect_true(sl_nodes.get("001").kind == SkipListGetKind.tombstone);
}

test("skipList: shouldFlush thresholds and empty/inverted boundary guards") {
    let sl = ArenaSkipList.init();
    expect_true(sl.put("", "bad") == SkipListWriteStatus.empty_key);
    expect_true(sl.delete("") == SkipListWriteStatus.empty_key);
    expect_true(sl.get("missing").kind == SkipListGetKind.not_found);
    expect_eq(sl.seek("a"), SKIPLIST_NO_NODE);

    let out: [4]SkipListEntryView = [SkipListEntryView{}; 4];
    let out_s: []SkipListEntryView = out[0..];
    expect_eq(sl.rangeScan("", "", true, *out_s), @as(usize, 0));

    // Write 4096 bytes (~500 permille of 8192 arena)
    let chunk: [1024]u8 = ['A'; 1024];
    * = sl.put("b1", chunk[0..]);
    * = sl.put("b2", chunk[0..]);
    * = sl.put("b3", chunk[0..]);
    * = sl.put("b4", chunk[0..]);

    expect_true(sl.shouldFlush(500));
    expect_eq(sl.shouldFlush(600), false);
    expect_true(sl.shouldFlush(0));

    // Inverted range [b4, b1) safely returns 0
    expect_eq(sl.rangeScan("b4", "b1", true, *out_s), @as(usize, 0));
}

test("skipList: telemetry snapshot, clear() seed restore, and cap.db facade parity") {
    let sl = cap.db.ArenaSkipList.initWithSeed(2026);
    * = sl.put("m", "1");
    * = sl.put("a", "2");
    * = sl.put("z", "3");
    * = sl.get("z");

    const st = sl.stats();
    expect_eq(st.node_capacity, SKIPLIST_MAX_NODES);
    expect_eq(st.arena_capacity, SKIPLIST_ARENA_BYTES);
    expect_eq(st.total_nodes, @as(usize, 3));
    expect_eq(st.live_keys, @as(usize, 3));
    expect_eq(st.tombstone_keys, @as(usize, 0));
    expect_true(st.total_comparisons > 0);
    expect_eq(st.next_version, @as(u64, 4));

    sl.clear();
    const st2 = sl.stats();
    expect_eq(st2.total_nodes, @as(usize, 0));
    expect_eq(st2.arena_used_bytes, @as(usize, 0));
    expect_eq(st2.total_comparisons, @as(u64, 0));
    expect_eq(st2.next_version, @as(u64, 1));
    expect_true(sl.get("m").kind == cap.db.SkipListGetKind.not_found);
}
