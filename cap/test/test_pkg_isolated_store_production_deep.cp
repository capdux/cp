// cap/test/test_pkg_isolated_store_production_deep.cp
// Production-deep regression tests for cap:pkg isolated_store module
// Source: /data/dev/bun/src/install/isolated_install/Store.cp & /data/dev/bun/docs/pm/isolated-installs.mdx

import { test, expect } from "cap:test";
import {
    ISOLATED_STORE_MAX_ENTRIES,
    ISOLATED_STORE_MAX_DEPS_PER_ENTRY,
    ISOLATED_STORE_MAX_PEERS,
    IsolatedStoreError,
    IsolatedEntryKind,
    IsolatedPeerRef,
    IsolatedStoreEntry,
    IsolatedStoreGraph,
    formatStorePathSegment,
    formatIsolatedStoreKey,
    formatIsolatedStoreDir,
    formatIsolatedSymlinkTarget,
    formatGlobalStorePath,
    computePeerSetHash,
    computeGlobalEntryClosureHash,
} from "cap:pkg";

fn strEq(a: []const u8, b: []const u8): bool {
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

test("pkg isolated_store: store key, dir & symlink target formatting (scoped, unscoped, root, folder)") {
    let buf: [160]u8 = [0; 160];

    // Unscoped registry package
    const e1 = IsolatedStoreEntry{
        kind: IsolatedEntryKind.Registry,
        name: "lodash",
        resolution: "4.17.21",
    };
    const k1 = formatIsolatedStoreKey(e1, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..k1], "lodash@4.17.21"));

    const d1 = formatIsolatedStoreDir(e1, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..d1], "node_modules/.bun/lodash@4.17.21/node_modules/lodash"));

    const s1 = formatIsolatedSymlinkTarget(e1, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..s1], ".bun/lodash@4.17.21/node_modules/lodash"));

    // Scoped registry package (+ replaces /, symlink uses ../.bun/)
    const e2 = IsolatedStoreEntry{
        kind: IsolatedEntryKind.Registry,
        name: "@scope/pkg",
        resolution: "2.1.0",
    };
    const k2 = formatIsolatedStoreKey(e2, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..k2], "@scope+pkg@2.1.0"));

    const d2 = formatIsolatedStoreDir(e2, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..d2], "node_modules/.bun/@scope+pkg@2.1.0/node_modules/@scope/pkg"));

    const s2 = formatIsolatedSymlinkTarget(e2, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..s2], "../.bun/@scope+pkg@2.1.0/node_modules/@scope/pkg"));

    // Root & Folder entries
    const e_root = IsolatedStoreEntry{
        kind: IsolatedEntryKind.Root,
        name: "my-app",
    };
    const kr = formatIsolatedStoreKey(e_root, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..kr], "my-app@root"));

    const e_anon_root = IsolatedStoreEntry{
        kind: IsolatedEntryKind.Root,
        name: "",
        top_level_dir_name: "monorepo",
    };
    const kar = formatIsolatedStoreKey(e_anon_root, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..kar], "monorepo"));

    const e_folder = IsolatedStoreEntry{
        kind: IsolatedEntryKind.Folder,
        name: "local-util",
        resolution: "packages/util",
    };
    const kf = formatIsolatedStoreKey(e_folder, buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..kf], "local-util@file+packages+util"));
}

test("pkg isolated_store: order-independent peer set hash & deduplication") {
    const empty: [0]IsolatedPeerRef = [];
    const h0 = computePeerSetHash(empty[0..]) catch { expect(false); return; };
    expect(h0 == 0);

    const p_ab: [2]IsolatedPeerRef = [
        IsolatedPeerRef{ name: "react", version: "18.3.1" },
        IsolatedPeerRef{ name: "react-dom", version: "18.3.1" },
    ];
    const p_ba: [2]IsolatedPeerRef = [
        IsolatedPeerRef{ name: "react-dom", version: "18.3.1" },
        IsolatedPeerRef{ name: "react", version: "18.3.1" },
    ];
    const p_dup: [3]IsolatedPeerRef = [
        IsolatedPeerRef{ name: "react", version: "18.3.1" },
        IsolatedPeerRef{ name: "react-dom", version: "18.3.1" },
        IsolatedPeerRef{ name: "react", version: "18.3.1" },
    ];
    const p_diff: [2]IsolatedPeerRef = [
        IsolatedPeerRef{ name: "react", version: "19.0.0" },
        IsolatedPeerRef{ name: "react-dom", version: "19.0.0" },
    ];

    const hab = computePeerSetHash(p_ab[0..]) catch { expect(false); return; };
    const hba = computePeerSetHash(p_ba[0..]) catch { expect(false); return; };
    const hdup = computePeerSetHash(p_dup[0..]) catch { expect(false); return; };
    const hdiff = computePeerSetHash(p_diff[0..]) catch { expect(false); return; };

    expect(hab != 0);
    expect(hab == hba);
    expect(hab == hdup);
    expect(hab != hdiff);

    // Key includes `+<16hex>` when peer_hash != 0
    const entry = IsolatedStoreEntry{
        kind: IsolatedEntryKind.Registry,
        name: "styled-box",
        resolution: "1.0.0",
        peer_hash: hab,
    };
    let buf: [128]u8 = [0; 128];
    const n = formatIsolatedStoreKey(entry, buf[0..]) catch { expect(false); return; };
    expect(n == "styled-box@1.0.0+".len + 16);
}

test("pkg isolated_store: graph deduplication & ordered dependency insertion") {
    let g = IsolatedStoreGraph.init();
    const id_a1 = g.addOrFindEntry(IsolatedEntryKind.Registry, "a", "1.0.0", 0, false) catch { expect(false); return; };
    const id_a2 = g.addOrFindEntry(IsolatedEntryKind.Registry, "a", "1.0.0", 0, false) catch { expect(false); return; };
    const id_b = g.addOrFindEntry(IsolatedEntryKind.Registry, "b", "2.0.0", 0, false) catch { expect(false); return; };
    const id_c = g.addOrFindEntry(IsolatedEntryKind.Registry, "c", "3.0.0", 0, false) catch { expect(false); return; };
    // Different peer_hash creates distinct entry
    const id_a_peer = g.addOrFindEntry(IsolatedEntryKind.Registry, "a", "1.0.0", 0xdeadbeef, false) catch { expect(false); return; };

    expect(id_a1 == id_a2);
    expect(id_a1 != id_a_peer);
    expect(g.count() == 4);

    // Insert dependencies out of order + duplicate -> stored sorted & deduplicated
    g.addDependency(id_a1, id_c) catch { expect(false); return; };
    g.addDependency(id_a1, id_b) catch { expect(false); return; };
    g.addDependency(id_a1, id_c) catch { expect(false); return; };

    if (ea := g.entryAt(id_a1)) {
        expect(ea.dep_count == 2);
        expect(ea.deps[0] == id_b);
        expect(ea.deps[1] == id_c);
    } else {
        expect(false);
    }
}

test("pkg isolated_store: transitive Global Virtual Store closure hash sensitivity & cycle safety") {
    // Project 1: app -> ui@1.0.0 -> dep@1.0.0
    let g1 = IsolatedStoreGraph.init();
    const g1_dep = g1.addOrFindEntry(IsolatedEntryKind.Registry, "dep", "1.0.0", 0, false) catch { expect(false); return; };
    const g1_ui = g1.addOrFindEntry(IsolatedEntryKind.Registry, "ui", "1.0.0", 0, false) catch { expect(false); return; };
    g1.addDependency(g1_ui, g1_dep) catch { expect(false); return; };
    g1.computeAllEntryHashes() catch { expect(false); return; };

    // Project 2: identical closure -> identical global store hash
    let g2 = IsolatedStoreGraph.init();
    const g2_dep = g2.addOrFindEntry(IsolatedEntryKind.Registry, "dep", "1.0.0", 0, false) catch { expect(false); return; };
    const g2_ui = g2.addOrFindEntry(IsolatedEntryKind.Registry, "ui", "1.0.0", 0, false) catch { expect(false); return; };
    g2.addDependency(g2_ui, g2_dep) catch { expect(false); return; };
    g2.computeAllEntryHashes() catch { expect(false); return; };

    // Project 3: transitive dep bumped to 1.0.1 -> ui's global store hash MUST change
    let g3 = IsolatedStoreGraph.init();
    const g3_dep = g3.addOrFindEntry(IsolatedEntryKind.Registry, "dep", "1.0.1", 0, false) catch { expect(false); return; };
    const g3_ui = g3.addOrFindEntry(IsolatedEntryKind.Registry, "ui", "1.0.0", 0, false) catch { expect(false); return; };
    g3.addDependency(g3_ui, g3_dep) catch { expect(false); return; };
    g3.computeAllEntryHashes() catch { expect(false); return; };

    const e1 = g1.entryAt(g1_ui).?;
    const e2 = g2.entryAt(g2_ui).?;
    const e3 = g3.entryAt(g3_ui).?;
    expect(e1.entry_hash != 0);
    expect(e1.entry_hash == e2.entry_hash);
    expect(e1.entry_hash != e3.entry_hash);

    let gpath: [128]u8 = [0; 128];
    const gn = formatGlobalStorePath(e1, gpath[0..]) catch { expect(false); return; };
    expect(gn == "ui@1.0.0-".len + 16);

    // Cyclic dependencies must terminate safely without infinite loop
    let gc = IsolatedStoreGraph.init();
    const c1 = gc.addOrFindEntry(IsolatedEntryKind.Registry, "cyc-a", "1.0.0", 0, false) catch { expect(false); return; };
    const c2 = gc.addOrFindEntry(IsolatedEntryKind.Registry, "cyc-b", "1.0.0", 0, false) catch { expect(false); return; };
    gc.addDependency(c1, c2) catch { expect(false); return; };
    gc.addDependency(c2, c1) catch { expect(false); return; };
    gc.computeAllEntryHashes() catch { expect(false); return; };
    expect(gc.entryAt(c1).?.entry_hash != 0);
    expect(gc.entryAt(c2).?.entry_hash != 0);
}

test("pkg isolated_store: non-registry entries excluded from Global Virtual Store") {
    let g = IsolatedStoreGraph.init();
    const ws_id = g.addOrFindEntry(IsolatedEntryKind.Workspace, "@mono/core", "workspace:packages/core", 0, false) catch { expect(false); return; };
    const patch_id = g.addOrFindEntry(IsolatedEntryKind.Patched, "react", "18.3.1", 0, false) catch { expect(false); return; };
    g.computeAllEntryHashes() catch { expect(false); return; };

    const ws = g.entryAt(ws_id).?;
    const pt = g.entryAt(patch_id).?;
    expect(!ws.supportsGlobalStore());
    expect(!pt.supportsGlobalStore());
    expect(ws.entry_hash == 0);
    expect(pt.entry_hash == 0);

    let buf: [128]u8 = [0; 128];
    let err_ws = false;
    * = formatGlobalStorePath(ws, buf[0..]) catch |e| {
        expect(e == IsolatedStoreError.NotEligibleForGlobalStore);
        err_ws = true;
    };
    expect(err_ws);

    let err_pt = false;
    * = formatGlobalStorePath(pt, buf[0..]) catch |e| {
        expect(e == IsolatedStoreError.NotEligibleForGlobalStore);
        err_pt = true;
    };
    expect(err_pt);
}

test("pkg isolated_store: malformed inputs, invalid IDs & buffer overflow guards") {
    let g = IsolatedStoreGraph.init();

    let err_name = false;
    * = g.addOrFindEntry(IsolatedEntryKind.Registry, "", "1.0.0", 0, false) catch |e| {
        expect(e == IsolatedStoreError.EmptyPackageName);
        err_name = true;
    };
    expect(err_name);

    let err_res = false;
    * = g.addOrFindEntry(IsolatedEntryKind.Registry, "foo", "", 0, false) catch |e| {
        expect(e == IsolatedStoreError.EmptyResolution);
        err_res = true;
    };
    expect(err_res);

    let err_id = false;
    g.addDependency(0, 99) catch |e| {
        expect(e == IsolatedStoreError.InvalidEntryId);
        err_id = true;
    };
    expect(err_id);

    let tiny: [4]u8 = [0; 4];
    let err_buf = false;
    * = formatStorePathSegment("@scope/package", tiny[0..]) catch |e| {
        expect(e == IsolatedStoreError.BufferTooSmall);
        err_buf = true;
    };
    expect(err_buf);

    // Closure hash helper is order-independent
    const d1: [2]u64 = [111, 222];
    const d2: [2]u64 = [222, 111];
    expect(computeGlobalEntryClosureHash("pkg@1.0.0", d1[0..]) == computeGlobalEntryClosureHash("pkg@1.0.0", d2[0..]));

    expect(ISOLATED_STORE_MAX_ENTRIES == 64);
    expect(ISOLATED_STORE_MAX_DEPS_PER_ENTRY == 32);
    expect(ISOLATED_STORE_MAX_PEERS == 32);
}
