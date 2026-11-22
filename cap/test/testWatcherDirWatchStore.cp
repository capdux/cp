// Origin: Pure-CP Stdlib Test Suite (lib/watcher/dirWatchStore.cp) | Created: 2026-11-22
// Production Deep Verification Suite for DirectoryWatchStore:
//   1. shouldTrackResolutionFailure loader/path guard rules (Script vs CSS/HTML, POSIX & Windows abs paths)
//   2. insert directory dedup, trailing-slash normalization, intrusive chain & duplicate-dep guard
//   3. removeDep head/middle/tail unlink & automatic empty-dir watch teardown
//   4. O(1) free_list slot recycling without invalidating live dep indices
//   5. removeSourceFile cross-directory cleanup preserving sibling source dependencies
//   6. Error guards: EmptyDirPath, EmptySpecifier, DirsCapacityExceeded (32) & DepsCapacityExceeded (128)

import { test, expect_eq, expect_true } from "cap:test";
import {
    DIR_WATCH_MAX_DIRS,
    DIR_WATCH_MAX_DEPS,
    DirWatchStoreError,
    DirWatchLoaderKind,
    shouldTrackResolutionFailure,
    DirWatchDep,
    DirWatchStore,
} from "cap:watcher";

test("DirWatchStore: shouldTrackResolutionFailure rules", fn() {
    // Script loader: only relative ./ or ../ from absolute source path
    expect_true(shouldTrackResolutionFailure("/app/src/index.ts", "./missing.ts", .Script));
    expect_true(shouldTrackResolutionFailure("/app/src/index.ts", "../shared/a.tsx", .Script));
    expect_true(shouldTrackResolutionFailure("C:\\proj\\index.ts", "./x.js", .Script));
    expect_true(!shouldTrackResolutionFailure("/app/src/index.ts", "lodash", .Script));
    expect_true(!shouldTrackResolutionFailure("relative/index.ts", "./missing.ts", .Script));
    expect_true(!shouldTrackResolutionFailure("/app/src/index.ts", "", .Script));

    // CSS / HTML loaders: allow non-relative non-empty specifiers
    expect_true(shouldTrackResolutionFailure("/app/styles.css", "theme.css", .Css));
    expect_true(shouldTrackResolutionFailure("/app/index.html", "favicon.ico", .Html));
    expect_true(!shouldTrackResolutionFailure("/app/index.html", "", .Html));
});

test("DirWatchStore: insert dedup, trailing-slash normalization & collectDepsForDir", fn() {
    let store = DirWatchStore.init();
    expect_eq(store.watchCount(), 0);

    const r1 = store.insert("/app/src/ui/", "/app/src/a.tsx", "./ui/Btn.tsx", 10) catch unreachable;
    expect_true(r1.dir_created);
    expect_true(r1.dep_inserted);
    expect_eq(r1.watch_id, 10);

    // Same directory with different trailing slash + new dep -> reuses existing watch_id 10
    const r2 = store.insert("/app/src/ui", "/app/src/b.tsx", "./ui/Card.tsx", 99) catch unreachable;
    expect_true(!r2.dir_created);
    expect_true(r2.dep_inserted);
    expect_eq(r2.watch_id, 10);

    // Exact duplicate (source, specifier) -> no-op
    const r_dup = store.insert("/app/src/ui///", "/app/src/a.tsx", "./ui/Btn.tsx", 77) catch unreachable;
    expect_true(!r_dup.dir_created);
    expect_true(!r_dup.dep_inserted);
    expect_eq(r_dup.dep_index, r1.dep_index);

    expect_eq(store.watchCount(), 1);
    expect_eq(store.activeDepCount(), 2);

    let collected: [8]DirWatchDep = [DirWatchDep{}; 8];
    const n = store.collectDepsForDir("/app/src/ui/", collected[0..]);
    expect_eq(n, 2);
    // Head-inserted: r2 is first, r1 is second
    expect_eq(collected[0].source_file_path, "/app/src/b.tsx");
    expect_eq(collected[1].source_file_path, "/app/src/a.tsx");
});

test("DirWatchStore: removeDep unlink & automatic empty-dir teardown", fn() {
    let store = DirWatchStore.init();
    * = store.insert("/dir", "/s1.ts", "./x.ts", 5) catch unreachable;
    * = store.insert("/dir", "/s2.ts", "./y.ts", 5) catch unreachable;
    * = store.insert("/dir", "/s3.ts", "./z.ts", 5) catch unreachable;
    expect_eq(store.activeDepCount(), 3);

    // Remove middle node (/s2.ts)
    const rm_mid = store.removeDep("/dir", "/s2.ts", "./y.ts");
    expect_true(rm_mid.dep_removed);
    expect_true(!rm_mid.dir_removed);
    expect_eq(store.activeDepCount(), 2);

    // Remove head (/s3.ts)
    const rm_head = store.removeDep("/dir", "/s3.ts", "./z.ts");
    expect_true(rm_head.dep_removed);
    expect_true(!rm_head.dir_removed);
    expect_eq(store.activeDepCount(), 1);

    // Remove final remaining dep (/s1.ts) -> directory watch automatically removed with watch_id 5
    const rm_last = store.removeDep("/dir", "/s1.ts", "./x.ts");
    expect_true(rm_last.dep_removed);
    expect_true(rm_last.dir_removed);
    expect_eq(rm_last.removed_watch_id, 5);
    expect_eq(store.watchCount(), 0);
    expect_eq(store.activeDepCount(), 0);
    expect_true(store.findDir("/dir") == null);
});

test("DirWatchStore: O(1) free_list slot recycling", fn() {
    let store = DirWatchStore.init();
    const r0 = store.insert("/d", "/a.ts", "./1.ts", 1) catch unreachable;
    const r1 = store.insert("/d", "/b.ts", "./2.ts", 1) catch unreachable;
    expect_eq(r0.dep_index, 0);
    expect_eq(r1.dep_index, 1);
    expect_eq(store.freeSlotCount(), 0);

    * = store.removeDep("/d", "/a.ts", "./1.ts");
    expect_eq(store.freeSlotCount(), 1);

    // Next insertion must recycle freed slot 0
    const r2 = store.insert("/d", "/c.ts", "./3.ts", 1) catch unreachable;
    expect_eq(r2.dep_index, 0);
    expect_eq(store.freeSlotCount(), 0);
    expect_eq(store.activeDepCount(), 2);
});

test("DirWatchStore: removeSourceFile cross-directory cleanup", fn() {
    let store = DirWatchStore.init();
    // /src/App.tsx has failed imports in /d1 (sole dep) and /d2 (shared with /src/Other.tsx)
    * = store.insert("/d1", "/src/App.tsx", "./m1.ts", 101) catch unreachable;
    * = store.insert("/d2", "/src/App.tsx", "./m2.ts", 102) catch unreachable;
    * = store.insert("/d2", "/src/Other.tsx", "./m3.ts", 102) catch unreachable;
    expect_eq(store.watchCount(), 2);
    expect_eq(store.activeDepCount(), 3);

    let removed_wids: [8]u16 = [0; 8];
    const n_rm = store.removeSourceFile("/src/App.tsx", removed_wids[0..]);
    // Only /d1 (watch_id 101) became empty; /d2 still has /src/Other.tsx
    expect_eq(n_rm, 1);
    expect_eq(removed_wids[0], 101);
    expect_eq(store.watchCount(), 1);
    expect_eq(store.activeDepCount(), 1);
    expect_true(store.findDir("/d1") == null);
    expect_true(store.findDir("/d2") != null);
});

test("DirWatchStore: error guards (EmptyDirPath, EmptySpecifier, DirsCapacityExceeded, DepsCapacityExceeded)", fn() {
    let store = DirWatchStore.init();

    let empty_dir = false;
    store.insert("", "/a.ts", "./x.ts", 1) catch |e| {
        if (e == DirWatchStoreError.EmptyDirPath) empty_dir = true;
    };
    expect_true(empty_dir);

    let empty_spec = false;
    store.insert("/d", "/a.ts", "", 1) catch |e| {
        if (e == DirWatchStoreError.EmptySpecifier) empty_spec = true;
    };
    expect_true(empty_spec);

    // Fill 32 unique directories
    const pool = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-_";
    let d: usize = 0;
    while (d < DIR_WATCH_MAX_DIRS) {
        * = store.insert(pool[d .. d + 1], "/s.ts", "./x.ts", @as(u16, d + 1)) catch unreachable;
        d += 1;
    }
    expect_eq(store.watchCount(), DIR_WATCH_MAX_DIRS);

    // 33rd unique dir -> DirsCapacityExceeded
    let dir_full = false;
    store.insert("overflow_dir", "/s.ts", "./x.ts", 999) catch |e| {
        if (e == DirWatchStoreError.DirsCapacityExceeded) dir_full = true;
    };
    expect_true(dir_full);

    // Fill remaining dep slots up to 128 on an existing dir ("0")
    let store2 = DirWatchStore.init();
    const spec_pool = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-_";
    let k: usize = 0;
    while (k < 64) {
        * = store2.insert("/root", "/s1.ts", spec_pool[k .. k + 1], 1) catch unreachable;
        * = store2.insert("/root", "/s2.ts", spec_pool[k .. k + 1], 1) catch unreachable;
        k += 1;
    }
    expect_eq(store2.activeDepCount(), DIR_WATCH_MAX_DEPS);

    let dep_full = false;
    store2.insert("/root", "/s3.ts", "./overflow.ts", 1) catch |e| {
        if (e == DirWatchStoreError.DepsCapacityExceeded) dep_full = true;
    };
    expect_true(dep_full);
});
