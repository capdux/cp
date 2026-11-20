// Source: cp/cap/test/test_intern_production_deep.cp
// Deep Production Verification Suite for cap:intern & cap.intern
// Covers:
//   1. StringInterner deduplication (1000x repeats over 10 words -> 10 unique bytes), SymbolId equality & read-only lookup
//   2. resolve / resolveOpt exact restoration, out-of-bounds InvalidSymbolId, large >4KB strings & std.testing.allocator zero-leak
//   3. StringBiMap.insert strict 1:1 bijection enforcement (KeyAlreadyExists & ValueAlreadyExists)
//   4. StringBiMap.forcePut atomic replacement & bidirectional removeByKey/removeByVal cleanup
//   5. `import cap from "cap"; cap.intern` dynamic mounting

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    InternError,
    SymbolId,
    StringInterner,
    StringBiMap,
} from "cap:intern";

test("cap:intern - StringInterner 1000x deduplication, SymbolId equality & read-only lookup") {
    const alloc = std.testing.allocator;
    let pool = StringInterner.init(alloc);
    defer pool.deinit();

    const words = [
        "alpha",
        "beta",
        "gamma",
        "delta",
        "epsilon",
        "zeta",
        "eta",
        "theta",
        "iota",
        "kappa",
    ];

    let expected_bytes: usize = 0;
    for (w of words) {
        expected_bytes += w.len;
    }

    // Read-only lookup before interning must return null and not pollute pool
    expect_eq(pool.lookup("alpha") == null, true);
    expect_eq(pool.count(), 0);
    expect_eq(pool.totalBytes(), 0);

    let first_ids: [10]SymbolId = undefined;
    let wi: usize = 0;
    while (wi < 10) : (wi += 1) {
        first_ids[wi] = pool.intern(words[wi]) catch unreachable;
        expect_eq(first_ids[wi], u32(wi));
    }

    // Repeat 1000 times across the 10 words: zero extra unique entries or bytes
    let rep: usize = 0;
    while (rep < 1000) : (rep += 1) {
        const idx = rep % 10;
        const id = pool.intern(words[idx]) catch unreachable;
        expect_eq(id, first_ids[idx]);
    }

    expect_eq(pool.count(), 10);
    expect_eq(pool.totalBytes(), expected_bytes);
    expect_eq(pool.lookup("gamma").?, first_ids[2]);
    expect_eq(pool.lookup("nonexistent") == null, true);
    expect_eq(pool.count(), 10);
}

test("cap:intern - resolve, resolveOpt, InvalidSymbolId & std.testing.allocator zero-leak") {
    const alloc = std.testing.allocator;
    let pool = StringInterner.init(alloc);
    defer pool.deinit();

    // Empty string interning
    const empty_id = pool.intern("") catch unreachable;
    expect_eq(pool.count(), 1);
    expect_eq(pool.totalBytes(), 0);
    expect_eq(std.mem.eql(u8, pool.resolve(empty_id) catch unreachable, ""), true);

    // Oversized string (> 4096 bytes) forces dedicated chunk allocation
    const big_buf = alloc.alloc(u8, 5000) catch unreachable;
    defer alloc.free(big_buf);
    @memset(big_buf, 'Z');

    const big_id = pool.intern(big_buf) catch unreachable;
    const short_id = pool.intern("compact_sym") catch unreachable;

    expect_eq(std.mem.eql(u8, pool.resolve(big_id) catch unreachable, big_buf), true);
    expect_eq(std.mem.eql(u8, pool.resolve(short_id) catch unreachable, "compact_sym"), true);
    expect_eq(std.mem.eql(u8, pool.resolveOpt(short_id).?, "compact_sym"), true);
    expect_eq(pool.totalBytes(), 5000 + 11);

    // Out-of-bounds SymbolId checks
    expect_eq(pool.resolveOpt(999) == null, true);
    let caught_invalid = false;
    * = pool.resolve(999) catch |e| {
        if (e == InternError.InvalidSymbolId) {
            caught_invalid = true;
        }
        break "";
    };
    expect_eq(caught_invalid, true);
}

test("cap:intern - StringBiMap.insert strict 1:1 bijection enforcement") {
    const alloc = std.testing.allocator;
    let bm = StringBiMap.init(alloc);
    defer bm.deinit();

    bm.insert("k1", "v1") catch unreachable;
    bm.insert("k2", "v2") catch unreachable;
    expect_eq(bm.len(), 2);
    expect_eq(bm.containsKey("k1"), true);
    expect_eq(bm.containsVal("v2"), true);
    expect_eq(std.mem.eql(u8, bm.getByKey("k1").?, "v1"), true);
    expect_eq(std.mem.eql(u8, bm.getByVal("v2").?, "k2"), true);

    // Idempotent insert of exact same pair succeeds cleanly
    bm.insert("k1", "v1") catch unreachable;
    expect_eq(bm.len(), 2);

    // Same key bound to different value -> KeyAlreadyExists
    let key_err = false;
    bm.insert("k1", "v_other") catch |e| {
        if (e == InternError.KeyAlreadyExists) {
            key_err = true;
        }
    };
    expect_eq(key_err, true);

    // Different key bound to existing value -> ValueAlreadyExists
    let val_err = false;
    bm.insert("k_other", "v2") catch |e| {
        if (e == InternError.ValueAlreadyExists) {
            val_err = true;
        }
    };
    expect_eq(val_err, true);

    expect_eq(bm.len(), 2);
    expect_eq(bm.containsKey("k_other"), false);
    expect_eq(bm.containsVal("v_other"), false);
}

test("cap:intern - StringBiMap.forcePut atomic replacement & bidirectional removal") {
    const alloc = std.testing.allocator;
    let bm = StringBiMap.init(alloc);
    defer bm.deinit();

    bm.insert("A", "1") catch unreachable;
    bm.insert("B", "2") catch unreachable;
    bm.insert("C", "3") catch unreachable;
    expect_eq(bm.len(), 3);

    // forcePut("A", "2") must atomically unlink A<->1 and B<->2, then establish A<->2
    bm.forcePut("A", "2") catch unreachable;
    expect_eq(bm.len(), 2);
    expect_eq(std.mem.eql(u8, bm.getByKey("A").?, "2"), true);
    expect_eq(std.mem.eql(u8, bm.getByVal("2").?, "A"), true);
    expect_eq(bm.getByKey("B") == null, true);
    expect_eq(bm.getByVal("1") == null, true);

    // removeByKey clears both forward and reverse
    expect_eq(bm.removeByKey("A"), true);
    expect_eq(bm.removeByKey("A"), false);
    expect_eq(bm.getByKey("A") == null, true);
    expect_eq(bm.getByVal("2") == null, true);
    expect_eq(bm.len(), 1);

    // removeByVal clears both reverse and forward
    expect_eq(bm.removeByVal("3"), true);
    expect_eq(bm.removeByVal("3"), false);
    expect_eq(bm.getByKey("C") == null, true);
    expect_eq(bm.getByVal("3") == null, true);
    expect_eq(bm.len(), 0);
}

test("cap.intern - dynamic mounting via import cap from cap") {
    const alloc = std.testing.allocator;

    let pool = cap.intern.StringInterner.init(alloc);
    defer pool.deinit();
    const id_a = pool.intern("dyn_symbol") catch unreachable;
    const id_b = pool.intern("dyn_symbol") catch unreachable;
    expect_eq(id_a, id_b);
    expect_eq(std.mem.eql(u8, pool.resolveOpt(id_a).?, "dyn_symbol"), true);

    let bm = cap.intern.StringBiMap.init(alloc);
    defer bm.deinit();
    bm.insert("alias", "target") catch unreachable;
    expect_eq(std.mem.eql(u8, bm.getByVal("target").?, "alias"), true);
}
