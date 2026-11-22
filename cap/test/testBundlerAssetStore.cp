// Origin: Pure-CP Stdlib Test Suite (lib/bundler/assetStore.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Content-Addressable Ref-Counted AssetStore:
//   1. formatAssetUrl 16-hex formatting (with dot, without dot, empty ext, leading zeros, BufferTooSmall)
//   2. Content-addressable deduplication across multiple paths & memory accounting
//   3. replacePath sole-owner in-place replacement (old hash revocation) & identical-hash refresh
//   4. replacePath shared-ref detach & convergence onto existing hash
//   5. swapRemove path-index fixup correctness via removePath & unrefByHash
//   6. Error guards: AssetNotFound, DoubleUnref, InvalidDecrement & putOrIncrementRefCount

import { test, expect_eq, expect_true } from "cap:test";
import {
    AssetStoreError,
    formatAssetUrl,
    AssetStore,
} from "cap:bundler";

test("AssetStore: formatAssetUrl hex encoding & BufferTooSmall guard", fn() {
    let buf: [64]u8 = [0; 64];

    const u1 = formatAssetUrl(0x0123456789abcdef, ".png", buf[0..]) catch "";
    expect_eq(u1, "/_bun/asset/0123456789abcdef.png");

    const u2 = formatAssetUrl(0x00000000000000ff, "css", buf[0..]) catch "";
    expect_eq(u2, "/_bun/asset/00000000000000ff.css");

    const u3 = formatAssetUrl(0, "", buf[0..]) catch "";
    expect_eq(u3, "/_bun/asset/0000000000000000");

    let tiny: [16]u8 = [0; 16];
    let got_small = false;
    formatAssetUrl(0xdeadbeef, ".js", tiny[0..]) catch |e| {
        if (e == AssetStoreError.BufferTooSmall) got_small = true;
    };
    expect_true(got_small);
});

test("AssetStore: content-addressable deduplication & memory accounting", fn() {
    let store = AssetStore.init();
    const h: u64 = 0x1122334455667788;

    const r1 = store.replacePath("/app/logo.png", h, 4096, "image/png") catch unreachable;
    expect_true(!r1.shared_existing);
    expect_true(!r1.replaced_in_place);
    expect_eq(store.pathCount(), 1);
    expect_eq(store.uniqueAssetCount(), 1);
    expect_eq(store.getRefCount(h), 1);
    expect_eq(store.totalMemoryBytes(), 4096);

    // Second path with identical content hash shares the same entry
    const r2 = store.replacePath("/app/favicon.png", h, 4096, "image/png") catch unreachable;
    expect_true(r2.shared_existing);
    expect_eq(r2.entry_index, r1.entry_index);
    expect_eq(store.pathCount(), 2);
    expect_eq(store.uniqueAssetCount(), 1);
    expect_eq(store.getRefCount(h), 2);
    expect_eq(store.totalMemoryBytes(), 4096);

    if (got := store.getHash("/app/favicon.png")) {
        expect_eq(got, h);
    } else {
        expect_true(false);
    }
});

test("AssetStore: sole-owner in-place replacePath & stale hash revocation", fn() {
    let store = AssetStore.init();
    const h1: u64 = 0xaaaa000000000001;
    const h2: u64 = 0xbbbb000000000002;

    const r1 = store.replacePath("/app/style.css", h1, 1024, "text/css") catch unreachable;
    expect_eq(store.uniqueAssetCount(), 1);

    // Overwrite with same hash -> refreshes metadata in-place, no eviction
    const r_same = store.replacePath("/app/style.css", h1, 1024, "text/css; charset=utf-8") catch unreachable;
    expect_true(r_same.replaced_in_place);
    expect_true(r_same.evicted_hash == null);
    if (entry := store.getByHash(h1)) {
        expect_eq(entry.mime_type, "text/css; charset=utf-8");
        expect_eq(entry.refs, 1);
    } else {
        expect_true(false);
    }

    // Overwrite with brand-new hash while refs == 1 -> in-place replace & revoke h1
    const r2 = store.replacePath("/app/style.css", h2, 2048, "text/css") catch unreachable;
    expect_true(r2.replaced_in_place);
    expect_eq(r2.entry_index, r1.entry_index);
    if (ev := r2.evicted_hash) {
        expect_eq(ev, h1);
    } else {
        expect_true(false);
    }
    expect_true(store.getByHash(h1) == null);
    expect_eq(store.getRefCount(h2), 1);
    expect_eq(store.uniqueAssetCount(), 1);
    expect_eq(store.totalMemoryBytes(), 2048);
});

test("AssetStore: shared-ref detach & convergence onto existing hash", fn() {
    let store = AssetStore.init();
    const h_shared: u64 = 0x1001;
    const h_new: u64 = 0x2002;

    * = store.replacePath("/a.svg", h_shared, 500, "image/svg+xml") catch unreachable;
    * = store.replacePath("/b.svg", h_shared, 500, "image/svg+xml") catch unreachable;
    expect_eq(store.getRefCount(h_shared), 2);

    // Overwriting /a.svg must NOT evict h_shared because /b.svg still references it
    const r_detach = store.replacePath("/a.svg", h_new, 700, "image/svg+xml") catch unreachable;
    expect_true(!r_detach.replaced_in_place);
    expect_true(r_detach.evicted_hash == null);
    expect_eq(store.getRefCount(h_shared), 1);
    expect_eq(store.getRefCount(h_new), 1);
    expect_eq(store.uniqueAssetCount(), 2);

    // Now overwrite /a.svg back to h_shared -> converges onto h_shared and evicts h_new!
    const r_conv = store.replacePath("/a.svg", h_shared, 500, "image/svg+xml") catch unreachable;
    expect_true(r_conv.shared_existing);
    if (ev := r_conv.evicted_hash) {
        expect_eq(ev, h_new);
    } else {
        expect_true(false);
    }
    expect_eq(store.getRefCount(h_shared), 2);
    expect_eq(store.getRefCount(h_new), 0);
    expect_eq(store.uniqueAssetCount(), 1);
});

test("AssetStore: swapRemove path-index fixup on removePath", fn() {
    let store = AssetStore.init();
    const h0: u64 = 0xa0;
    const h1: u64 = 0xb1;
    const h2: u64 = 0xc2;

    * = store.replacePath("/p0.png", h0, 100, "image/png") catch unreachable;
    * = store.replacePath("/p1.png", h1, 200, "image/png") catch unreachable;
    * = store.replacePath("/p2.png", h2, 300, "image/png") catch unreachable;
    expect_eq(store.uniqueAssetCount(), 3);

    // Removing /p0.png evicts index 0 and swaps index 2 (h2) into index 0.
    // The path mapping for /p2.png must be fixed up to still resolve to h2!
    const evicted = store.removePath("/p0.png") catch false;
    expect_true(evicted);
    expect_eq(store.pathCount(), 2);
    expect_eq(store.uniqueAssetCount(), 2);
    expect_true(store.getHash("/p0.png") == null);

    if (got1 := store.getHash("/p1.png")) {
        expect_eq(got1, h1);
    } else {
        expect_true(false);
    }
    if (got2 := store.getHash("/p2.png")) {
        expect_eq(got2, h2);
    } else {
        expect_true(false);
    }
});

test("AssetStore: putOrIncrementRefCount & error guards (NotFound, DoubleUnref, InvalidDecrement)", fn() {
    let store = AssetStore.init();
    const h: u64 = 0xf00d;

    expect_true(store.putOrIncrementRefCount(h, 512, "application/wasm", 2) catch false);
    expect_true(!(store.putOrIncrementRefCount(h, 512, "application/wasm", 1) catch true));
    expect_eq(store.getRefCount(h), 3);

    // InvalidDecrement (0)
    let inv = false;
    store.unrefByHash(h, 0) catch |e| {
        if (e == AssetStoreError.InvalidDecrement) inv = true;
    };
    expect_true(inv);

    // DoubleUnref (dec_count > refs)
    let dbl = false;
    store.unrefByHash(h, 4) catch |e| {
        if (e == AssetStoreError.DoubleUnref) dbl = true;
    };
    expect_true(dbl);
    expect_eq(store.getRefCount(h), 3);

    // Unref by 2 -> still alive (refs == 1)
    expect_true(!(store.unrefByHash(h, 2) catch true));
    expect_eq(store.getRefCount(h), 1);

    // Final unref by 1 -> evicted
    expect_true(store.unrefByHash(h, 1) catch false);
    expect_eq(store.uniqueAssetCount(), 0);

    // Unref again -> AssetNotFound
    let nf = false;
    store.unrefByHash(h, 1) catch |e| {
        if (e == AssetStoreError.AssetNotFound) nf = true;
    };
    expect_true(nf);
});
