// Origin: Pure-CP Stdlib Test Suite (lib/db/memTable.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc LSM-Tree Sorted MemTable:
//   1. Unsorted inserts, O(log N) lookup, same-key in-place value reuse & larger value update
//   2. Tombstone deletion (existing & SSTable-shadowing absent keys) & key resurrection via put
//   3. Lexicographical scanRange (half-open [start, end), with/without tombstones & inverted range)
//   4. Lexicographical scanPrefix (live-only filtering & boundary exclusion)
//   5. Immutable freeze() rejects subsequent writes while preserving full read/flush access
//   6. Full boundary guards: empty_key, stale_seq, 128-slot index_full & atomic 8 KiB arena_full
//   7. copyKey/copyValue undersized buffer guards & cap.db dynamic facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    MEMTABLE_MAX_ENTRIES,
    MEMTABLE_ARENA_BYTES,
    MemTableWriteStatus,
    MemTableGetKind,
    MemTableEntryRef,
    SortedMemTable,
} from "cap:db";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// Test 1: Unsorted Inserts, Binary-Search Get & In-Place Value Reuse
// ============================================================================
test("memTable: unsorted inserts, O(log N) get & same-key in-place value reuse", () => {
    let mt = SortedMemTable.init();

    expect_eq(mt.put("k3", "val3", 10), MemTableWriteStatus.ok);
    expect_eq(mt.put("k1", "val1", 20), MemTableWriteStatus.ok);
    expect_eq(mt.put("k4", "val4", 30), MemTableWriteStatus.ok);
    expect_eq(mt.put("k2", "val2_long", 40), MemTableWriteStatus.ok);

    const arena_before = mt.stats().arena_used_bytes;

    // Update k2 with shorter value -> reuses both key and value bytes in-place (zero arena growth!)
    expect_eq(mt.put("k2", "v2", 50), MemTableWriteStatus.ok);
    expect_eq(mt.stats().arena_used_bytes, arena_before);

    const g2 = mt.get("k2");
    expect_eq(g2.kind, MemTableGetKind.found_value);
    expect_eq(g2.seq_num, 50 as u64);
    expect_true(mt.valueEquals(g2.entry, "v2"));

    // Update k2 with larger value -> appends new value bytes and updates pointer
    expect_eq(mt.put("k2", "much_longer_value_2", 60), MemTableWriteStatus.ok);
    const g2b = mt.get("k2");
    expect_eq(g2b.kind, MemTableGetKind.found_value);
    expect_eq(g2b.seq_num, 60 as u64);
    expect_true(mt.valueEquals(g2b.entry, "much_longer_value_2"));

    // Absent key lookup
    expect_eq(mt.get("missing").kind, MemTableGetKind.not_found);
});

// ============================================================================
// Test 2: Tombstone Deletion (Existing & Absent Keys) & Resurrection
// ============================================================================
test("memTable: tombstone deletion shadows lower SSTables and put resurrects cleanly", () => {
    let mt = SortedMemTable.init();
    expect_eq(mt.put("alpha", "1", 1), MemTableWriteStatus.ok);
    expect_eq(mt.put("beta", "2", 2), MemTableWriteStatus.ok);

    // Delete existing key "alpha" -> found_tombstone
    expect_eq(mt.deleteKey("alpha", 3), MemTableWriteStatus.ok);
    const ga = mt.get("alpha");
    expect_eq(ga.kind, MemTableGetKind.found_tombstone);
    expect_eq(ga.seq_num, 3 as u64);

    // Delete absent key "sst_key" -> inserts tombstone to shadow lower SSTable
    expect_eq(mt.deleteKey("sst_key", 4), MemTableWriteStatus.ok);
    const gs = mt.get("sst_key");
    expect_eq(gs.kind, MemTableGetKind.found_tombstone);
    expect_eq(gs.seq_num, 4 as u64);

    const st1 = mt.stats();
    expect_eq(st1.entry_count, 3 as usize);
    expect_eq(st1.live_count, 1 as usize);
    expect_eq(st1.tombstone_count, 2 as usize);

    // Resurrect "alpha" with higher seq_num
    expect_eq(mt.put("alpha", "reborn", 5), MemTableWriteStatus.ok);
    const ga2 = mt.get("alpha");
    expect_eq(ga2.kind, MemTableGetKind.found_value);
    expect_true(mt.valueEquals(ga2.entry, "reborn"));
    expect_eq(mt.stats().live_count, 2 as usize);
    expect_eq(mt.stats().tombstone_count, 1 as usize);
});

// ============================================================================
// Test 3: Lexicographical scanRange (With & Without Tombstones)
// ============================================================================
test("memTable: scanRange outputs strict lexicographical order with optional tombstones", () => {
    let mt = SortedMemTable.init();
    expect_eq(mt.put("e", "5", 1), MemTableWriteStatus.ok);
    expect_eq(mt.put("b", "2", 2), MemTableWriteStatus.ok);
    expect_eq(mt.put("d", "4", 3), MemTableWriteStatus.ok);
    expect_eq(mt.put("a", "1", 4), MemTableWriteStatus.ok);
    expect_eq(mt.put("c", "3", 5), MemTableWriteStatus.ok);
    expect_eq(mt.deleteKey("c", 6), MemTableWriteStatus.ok);

    let out: [8]MemTableEntryRef = [MemTableEntryRef{}; 8];
    let out_s: []MemTableEntryRef = out[0..];

    // [b, e) excluding tombstones -> ["b", "d"]
    const n_live = mt.scanRange("b", "e", false, *out_s);
    expect_eq(n_live, 2 as usize);
    expect_true(mt.keyEquals(out[0], "b"));
    expect_true(mt.keyEquals(out[1], "d"));

    // [b, e) including tombstones (SSTable flush path) -> ["b", "c"(tombstone), "d"]
    const n_all = mt.scanRange("b", "e", true, *out_s);
    expect_eq(n_all, 3 as usize);
    expect_true(mt.keyEquals(out[0], "b"));
    expect_eq(out[0].is_tombstone, false);
    expect_true(mt.keyEquals(out[1], "c"));
    expect_eq(out[1].is_tombstone, true);
    expect_true(mt.keyEquals(out[2], "d"));

    // Open bounds ("", "") with tombstones -> all 5 in sorted order a, b, c, d, e
    const n_full = mt.scanRange("", "", true, *out_s);
    expect_eq(n_full, 5 as usize);
    expect_true(mt.keyEquals(out[0], "a"));
    expect_true(mt.keyEquals(out[4], "e"));

    // Inverted range [e, b) -> 0
    expect_eq(mt.scanRange("e", "b", true, *out_s), 0 as usize);
});

// ============================================================================
// Test 4: Lexicographical scanPrefix
// ============================================================================
test("memTable: scanPrefix matches only live keys with exact prefix in sorted order", () => {
    let mt = SortedMemTable.init();
    expect_eq(mt.put("user:300", "c", 1), MemTableWriteStatus.ok);
    expect_eq(mt.put("order:1", "o", 2), MemTableWriteStatus.ok);
    expect_eq(mt.put("user:100", "a", 3), MemTableWriteStatus.ok);
    expect_eq(mt.put("user_meta:1", "m", 4), MemTableWriteStatus.ok);
    expect_eq(mt.put("user:200", "b", 5), MemTableWriteStatus.ok);
    expect_eq(mt.deleteKey("user:200", 6), MemTableWriteStatus.ok);

    let out: [8]MemTableEntryRef = [MemTableEntryRef{}; 8];
    let out_s: []MemTableEntryRef = out[0..];
    const n = mt.scanPrefix("user:", *out_s);
    expect_eq(n, 2 as usize);
    expect_true(mt.keyEquals(out[0], "user:100"));
    expect_true(mt.valueEquals(out[0], "a"));
    expect_true(mt.keyEquals(out[1], "user:300"));
    expect_true(mt.valueEquals(out[1], "c"));
});

// ============================================================================
// Test 5: Immutable Freeze Prior to SSTable Flush
// ============================================================================
test("memTable: freeze rejects subsequent put/deleteKey while keeping reads intact", () => {
    let mt = SortedMemTable.init();
    expect_eq(mt.put("k1", "v1", 10), MemTableWriteStatus.ok);
    expect_eq(mt.isFrozen(), false);

    mt.freeze();
    expect_eq(mt.isFrozen(), true);
    expect_eq(mt.stats().frozen, true);

    expect_eq(mt.put("k2", "v2", 11), MemTableWriteStatus.frozen);
    expect_eq(mt.deleteKey("k1", 12), MemTableWriteStatus.frozen);

    // Reads still succeed
    const g1 = mt.get("k1");
    expect_eq(g1.kind, MemTableGetKind.found_value);
    expect_true(mt.valueEquals(g1.entry, "v1"));
    expect_eq(mt.stats().entry_count, 1 as usize);
});

// ============================================================================
// Test 6: Boundary Guards (empty_key, stale_seq, index_full & atomic arena_full)
// ============================================================================
test("memTable: rejects empty_key, stale_seq, 128-slot index_full & atomic 8KiB arena_full", () => {
    let mt = SortedMemTable.init();

    // Empty key rejected
    expect_eq(mt.put("", "v", 1), MemTableWriteStatus.empty_key);
    expect_eq(mt.deleteKey("", 1), MemTableWriteStatus.empty_key);

    // Monotonic seq_num guard
    expect_eq(mt.put("k0", "v0", 10), MemTableWriteStatus.ok);
    expect_eq(mt.put("k1", "v1", 10), MemTableWriteStatus.stale_seq);
    expect_eq(mt.put("k1", "v1", 5), MemTableWriteStatus.stale_seq);
    expect_eq(mt.deleteKey("k0", 10), MemTableWriteStatus.stale_seq);

    // Fill remaining 127 slots -> total 128 entries
    let i: usize = 1;
    while (i < MEMTABLE_MAX_ENTRIES) : (i += 1) {
        const d0: u8 = ('0' as u8) + ((i / 100) as u8);
        const d1: u8 = ('0' as u8) + (((i / 10) % 10) as u8);
        const d2: u8 = ('0' as u8) + ((i % 10) as u8);
        const kbuf: [4]u8 = ['k', d0, d1, d2];
        expect_eq(mt.put(kbuf[0..], "x", (10 + i) as u64), MemTableWriteStatus.ok);
    }
    expect_eq(mt.stats().entry_count, MEMTABLE_MAX_ENTRIES);

    // 129th distinct key -> index_full; updating existing key "k0" still succeeds!
    expect_eq(mt.put("overflow_key", "v", 500), MemTableWriteStatus.index_full);
    expect_eq(mt.put("k0", "u0", 501), MemTableWriteStatus.ok);

    // Arena-full atomic guard on a fresh table
    let mt2 = SortedMemTable.init();
    const big_val: [4096]u8 = ['z'; 4096];
    expect_eq(mt2.put("a", big_val[0..], 1), MemTableWriteStatus.ok);
    const used_after_a = mt2.stats().arena_used_bytes;
    // Second 4096-byte value + 1-byte key exceeds 8192 (4097 + 4097 = 8194 > 8192) -> atomic arena_full!
    expect_eq(mt2.put("b", big_val[0..], 2), MemTableWriteStatus.arena_full);
    expect_eq(mt2.stats().entry_count, 1 as usize);
    expect_eq(mt2.stats().arena_used_bytes, used_after_a);
    expect_eq(mt2.stats().arena_capacity_bytes, MEMTABLE_ARENA_BYTES);
});

// ============================================================================
// Test 7: copyKey / copyValue Guards & cap.db Dynamic Facade Parity
// ============================================================================
test("memTable: copyKey/copyValue buffer guards & cap.db dynamic facade", () => {
    let mt = cap.db.SortedMemTable.init();
    expect_eq(mt.put("cfg:mode", "production", 100), MemTableWriteStatus.ok);

    const g = mt.get("cfg:mode");
    expect_eq(g.kind, MemTableGetKind.found_value);

    // Undersized buffers return null safely
    let tiny: [3]u8 = [0; 3];
    let tiny_s: []u8 = tiny[0..];
    expect_eq(mt.copyKey(g.entry, *tiny_s) == null, true);
    expect_eq(mt.copyValue(g.entry, *tiny_s) == null, true);

    // Adequate buffers copy exact bytes
    let buf: [32]u8 = [0; 32];
    let buf_s: []u8 = buf[0..];
    const kl = mt.copyKey(g.entry, *buf_s) ?? 0;
    expect_true(strEq(buf[0..kl], "cfg:mode"));
    const vl = mt.copyValue(g.entry, *buf_s) ?? 0;
    expect_true(strEq(buf[0..vl], "production"));

    const st = mt.stats();
    expect_eq(st.min_seq_num, 100 as u64);
    expect_eq(st.max_seq_num, 100 as u64);
});
