// Origin: Pure-CP Stdlib Test Suite (lib/sourcemap/sourceMapStore.cp) | Created: 2026-11-22
// Production Deep Verification Suite for DevServer SourceMapStore:
//   1. SourceMapStoreId 64-bit kind/payload packing & formatUrl() / BufferTooSmall guard
//   2. Strong ref-counting, 10-second weak-ref grace period serving & expiry
//   3. In-place revival from weak-ref pool via putOrRef()
//   4. Bounded 16-entry weak-ref FIFO eviction when 17th weak entry arrives
//   5. pruneExpired() mixed-batch cleanup & removeImmediate()
//   6. Error guards: NotFound, DoubleUnref, CapacityExceeded (128 strong) & weak-evict-on-full

import { test, expect_eq, expect_true } from "cap:test";
import {
    SOURCEMAP_WEAK_REF_EXPIRY_SECS,
    SOURCEMAP_WEAK_REF_MAX,
    SOURCEMAP_STORE_MAX_ENTRIES,
    SourceMapStoreError,
    SourceMapChunkKind,
    SourceMapStoreId,
    SourceMapStore,
} from "cap:sourcemap";

test("SourceMapStoreId: 64-bit kind tagging, masking & formatUrl", fn() {
    const id_init = SourceMapStoreId.forInitialResponse(0x00123456);
    expect_eq(id_init.kind(), SourceMapChunkKind.InitialResponse);
    expect_eq(id_init.generationId(), 0x00123456);

    // High bit of content_hash must be masked off, then bit 63 set for HmrChunk
    const id_hmr = SourceMapStoreId.forHmrChunk(0xFEDCBA9876543210);
    expect_eq(id_hmr.kind(), SourceMapChunkKind.HmrChunk);
    expect_eq(id_hmr.contentHash(), 0x7EDCBA9876543210);

    let buf: [64]u8 = [0; 64];
    const url_init = id_init.formatUrl(buf[0..]) catch "";
    expect_eq(url_init, "/_bun/client/0000000000123456.js.map");

    const url_hmr = id_hmr.formatUrl(buf[0..]) catch "";
    expect_eq(url_hmr, "/_bun/client/fedcba9876543210.js.map");

    let tiny: [16]u8 = [0; 16];
    let got_small = false;
    id_init.formatUrl(tiny[0..]) catch |e| {
        if (e == SourceMapStoreError.BufferTooSmall) got_small = true;
    };
    expect_true(got_small);
});

test("SourceMapStore: strong ref-counting & 10-second weak-ref grace window", fn() {
    let store = SourceMapStore.init();
    const id = SourceMapStoreId.forInitialResponse(7);

    expect_true(store.putOrRef(id, 5, 4096) catch false);
    expect_true(!(store.putOrRef(id, 5, 4096) catch true)); // ref_count -> 2
    expect_eq(store.strongCount(), 1);
    expect_eq(store.weakCount(), 0);
    expect_eq(store.totalMemoryBytes(), 4096);

    // First unref: ref_count 2 -> 1 (still strong)
    expect_true(!(store.unref(id, 100) catch true));
    expect_eq(store.strongCount(), 1);
    expect_eq(store.weakCount(), 0);

    // Second unref at t=100s: ref_count 1 -> 0 (enters weak grace pool)
    expect_true(store.unref(id, 100) catch false);
    expect_eq(store.strongCount(), 0);
    expect_eq(store.weakCount(), 1);

    // Still resolvable at t=109s (within 10s grace period)
    if (e := store.get(id, 100 + SOURCEMAP_WEAK_REF_EXPIRY_SECS - 1)) {
        expect_true(e.isWeak());
        expect_eq(e.file_count, 5);
    } else {
        expect_true(false);
    }

    // Expired at t=110s
    expect_true(store.get(id, 100 + SOURCEMAP_WEAK_REF_EXPIRY_SECS) == null);
});

test("SourceMapStore: weak-ref revival via putOrRef", fn() {
    let store = SourceMapStore.init();
    const id = SourceMapStoreId.forHmrChunk(0x4242);

    * = store.putOrRef(id, 3, 2048) catch false;
    * = store.unref(id, 50) catch false;
    expect_eq(store.weakCount(), 1);

    // Revive before expiry
    expect_true(!(store.putOrRef(id, 4, 2500) catch true));
    expect_eq(store.strongCount(), 1);
    expect_eq(store.weakCount(), 0);

    // Must NOT be pruned even far in the future
    expect_eq(store.pruneExpired(9999), 0);
    if (e := store.get(id, 9999)) {
        expect_eq(e.ref_count, 1);
        expect_eq(e.memory_bytes, 2500);
    } else {
        expect_true(false);
    }
});

test("SourceMapStore: bounded 16-entry weak-ref FIFO eviction", fn() {
    let store = SourceMapStore.init();

    // Insert 17 entries and transition all 17 to weak state in order 1..17
    let i: u32 = 1;
    while (i <= @as(u32, SOURCEMAP_WEAK_REF_MAX) + 1) {
        const id = SourceMapStoreId.forInitialResponse(i);
        * = store.putOrRef(id, 1, 100) catch false;
        * = store.unref(id, 1000) catch false;
        i += 1;
    }

    // Pool capped at 16; entry #1 (oldest) must have been evicted, #2 and #17 remain
    expect_eq(store.weakCount(), SOURCEMAP_WEAK_REF_MAX);
    expect_true(store.get(SourceMapStoreId.forInitialResponse(1), 1001) == null);
    expect_true(store.get(SourceMapStoreId.forInitialResponse(2), 1001) != null);
    expect_true(store.get(SourceMapStoreId.forInitialResponse(17), 1001) != null);
});

test("SourceMapStore: pruneExpired mixed batch & removeImmediate", fn() {
    let store = SourceMapStore.init();
    const id_strong = SourceMapStoreId.forInitialResponse(1);
    const id_old_weak = SourceMapStoreId.forInitialResponse(2);
    const id_fresh_weak = SourceMapStoreId.forInitialResponse(3);

    * = store.putOrRef(id_strong, 1, 100) catch false;
    * = store.putOrRef(id_old_weak, 1, 200) catch false;
    * = store.putOrRef(id_fresh_weak, 1, 300) catch false;

    * = store.unref(id_old_weak, 100) catch false;   // expires at 110
    * = store.unref(id_fresh_weak, 108) catch false; // expires at 118

    // Prune at t=112: only id_old_weak is removed
    expect_eq(store.pruneExpired(112), 1);
    expect_eq(store.totalCount(), 2);
    expect_true(store.get(id_old_weak, 112) == null);
    expect_true(store.get(id_fresh_weak, 112) != null);

    // removeImmediate removes strong entry cleanly
    expect_true(store.removeImmediate(id_strong));
    expect_true(!store.removeImmediate(id_strong));
    expect_eq(store.totalCount(), 1);
});

test("SourceMapStore: error guards (NotFound, DoubleUnref, CapacityExceeded & weak eviction on full)", fn() {
    let store = SourceMapStore.init();
    const missing = SourceMapStoreId.forInitialResponse(999);

    // NotFound on unref
    let nf = false;
    store.unref(missing, 0) catch |e| {
        if (e == SourceMapStoreError.NotFound) nf = true;
    };
    expect_true(nf);

    // DoubleUnref when already weak
    const w_id = SourceMapStoreId.forInitialResponse(1);
    * = store.putOrRef(w_id, 1, 64) catch false;
    * = store.unref(w_id, 10) catch false;
    let dbl = false;
    store.unref(w_id, 11) catch |e| {
        if (e == SourceMapStoreError.DoubleUnref) dbl = true;
    };
    expect_true(dbl);

    // Fill remaining 127 slots with strong entries (total 128: 1 weak + 127 strong)
    let k: u32 = 2;
    while (k <= @as(u32, SOURCEMAP_STORE_MAX_ENTRIES)) {
        * = store.putOrRef(SourceMapStoreId.forInitialResponse(k), 1, 64) catch false;
        k += 1;
    }
    expect_eq(store.totalCount(), SOURCEMAP_STORE_MAX_ENTRIES);

    // Inserting 129th must succeed by evicting the 1 weak entry (w_id)!
    const id_129 = SourceMapStoreId.forInitialResponse(129);
    expect_true(store.putOrRef(id_129, 1, 64) catch false);
    expect_true(store.get(w_id, 12) == null);
    expect_eq(store.strongCount(), SOURCEMAP_STORE_MAX_ENTRIES);

    // Now all 128 are strong -> 130th must return CapacityExceeded
    let cap_err = false;
    store.putOrRef(SourceMapStoreId.forInitialResponse(130), 1, 64) catch |e| {
        if (e == SourceMapStoreError.CapacityExceeded) cap_err = true;
    };
    expect_true(cap_err);
});
