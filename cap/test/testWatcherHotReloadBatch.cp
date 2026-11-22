// Origin: Pure-CP Stdlib Test Suite (lib/watcher/hotReloadBatch.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Allocation HMR HotReloadBatch:
//   1. stripTrailingPathSep & isHotReloadPathSep normalization (POSIX, Windows, root "/")
//   2. appendFile deduplication, empty path ignore, containsFile & getFile
//   3. appendDir directory dedup, slash-normalized subpath join into NUL-delimited extra_arena & root join
//   4. reset() O(1) slot recycling & generation monotonicity
//   5. End-to-end integration with WatcherAtomicRelay<HotReloadBatch>
//   6. Boundary guards: DirsCapacityExceeded (32), FilesCapacityExceeded (64) & IndexOutOfBounds

import { test, expect_eq, expect_true } from "cap:test";
import {
    HOT_RELOAD_MAX_FILES,
    HOT_RELOAD_MAX_DIRS,
    HotReloadBatchError,
    isHotReloadPathSep,
    stripTrailingPathSep,
    HotReloadBatch,
    WatcherAtomicRelay,
    WatcherSubmitOutcome,
} from "cap:watcher";

test("HotReloadBatch: path separator helpers & trailing slash stripping", fn() {
    expect_true(isHotReloadPathSep('/'));
    expect_true(isHotReloadPathSep('\\'));
    expect_true(!isHotReloadPathSep('a'));

    expect_eq(stripTrailingPathSep("/"), "/");
    expect_eq(stripTrailingPathSep(""), "");
    expect_eq(stripTrailingPathSep("src/app/"), "src/app");
    expect_eq(stripTrailingPathSep("src/app///"), "src/app");
    expect_eq(stripTrailingPathSep("C:\\proj\\src\\"), "C:\\proj\\src");
    expect_eq(stripTrailingPathSep("src/app"), "src/app");
});

test("HotReloadBatch: appendFile deduplication & lookup", fn() {
    let batch = HotReloadBatch.init();
    expect_true(batch.isEmpty());
    expect_eq(batch.fileCount(), 0);

    // Empty path ignored
    expect_true(!(batch.appendFile("") catch true));
    expect_true(batch.isEmpty());

    // First insert returns true; duplicate returns false
    expect_true(batch.appendFile("src/index.ts") catch false);
    expect_true(!(batch.appendFile("src/index.ts") catch true));
    expect_true(batch.appendFile("src/utils.ts") catch false);

    expect_true(!batch.isEmpty());
    expect_eq(batch.fileCount(), 2);
    expect_true(batch.containsFile("src/index.ts"));
    expect_true(batch.containsFile("src/utils.ts"));
    expect_true(!batch.containsFile("src/missing.ts"));
    expect_eq(batch.getFile(0) catch "", "src/index.ts");
    expect_eq(batch.getFile(1) catch "", "src/utils.ts");
});

test("HotReloadBatch: appendDir slash normalization, root join & extra_arena dedup", fn() {
    let batch = HotReloadBatch.init();

    // Trailing slash on dir + leading slash on sub -> single "/"
    expect_true(batch.appendDir("src/pages/", "/home.tsx") catch false);
    expect_eq(batch.dirCount(), 1);
    expect_eq(batch.extraFileCount(), 1);
    expect_eq(batch.getDir(0) catch "", "src/pages");
    expect_eq(batch.getExtraFile(0) catch "", "src/pages/home.tsx");
    expect_true(batch.containsDir("src/pages///"));
    expect_true(batch.containsFile("src/pages/home.tsx"));

    // Exact duplicate (even with different slash styles) must not duplicate dir or extra_arena bytes
    const used_before = batch.extraArenaUsed();
    expect_true(!(batch.appendDir("src/pages", "home.tsx") catch true));
    expect_eq(batch.dirCount(), 1);
    expect_eq(batch.extraFileCount(), 1);
    expect_eq(batch.extraArenaUsed(), used_before);

    // Same dir, new sub file -> dir count unchanged, extra file count incremented
    expect_true(batch.appendDir("src/pages", "about.tsx") catch false);
    expect_eq(batch.dirCount(), 1);
    expect_eq(batch.extraFileCount(), 2);
    expect_eq(batch.getExtraFile(1) catch "", "src/pages/about.tsx");

    // Root directory "/" + "main.ts" -> "/main.ts" (no double leading slash)
    expect_true(batch.appendDir("/", "main.ts") catch false);
    expect_eq(batch.dirCount(), 2);
    expect_eq(batch.getDir(1) catch "", "/");
    expect_eq(batch.getExtraFile(2) catch "", "/main.ts");

    // Directory without subpath (null)
    expect_true(batch.appendDir("assets/", null) catch false);
    expect_eq(batch.dirCount(), 3);
    expect_eq(batch.extraFileCount(), 3);
    expect_eq(batch.getDir(2) catch "", "assets");
});

test("HotReloadBatch: reset() O(1) reuse & generation counter", fn() {
    let batch = HotReloadBatch.init();
    expect_eq(batch.generation, 0);

    * = batch.appendFile("a.ts") catch false;
    * = batch.appendDir("dir", "b.ts") catch false;
    expect_true(!batch.isEmpty());
    expect_true(batch.extraArenaUsed() > 0);

    batch.reset();
    expect_eq(batch.generation, 1);
    expect_true(batch.isEmpty());
    expect_eq(batch.fileCount(), 0);
    expect_eq(batch.dirCount(), 0);
    expect_eq(batch.extraFileCount(), 0);
    expect_eq(batch.extraArenaUsed(), 0);
    expect_true(!batch.containsFile("a.ts"));

    // Reusable immediately after reset
    expect_true(batch.appendFile("c.ts") catch false);
    expect_eq(batch.fileCount(), 1);
    expect_eq(batch.getFile(0) catch "", "c.ts");
});

test("HotReloadBatch: integration with WatcherAtomicRelay<HotReloadBatch>", fn() {
    let relay = WatcherAtomicRelay<HotReloadBatch>.init(HotReloadBatch.init());

    // Producer acquires slot, fills deduplicated HMR events, submits
    const slot = relay.watcherAcquire() catch unreachable;
    let b = relay.slotMut(slot);
    b.reset();
    * = b.appendFile("src/App.tsx") catch false;
    * = b.appendDir("src/styles/", "main.css") catch false;
    const outcome = relay.watcherReleaseAndSubmit(slot, true) catch unreachable;
    expect_eq(outcome, WatcherSubmitOutcome.ScheduledImmediate);

    // Consumer inspects payload and recycles slot
    const view = relay.slotMut(slot);
    expect_eq(view.fileCount(), 1);
    expect_eq(view.dirCount(), 1);
    expect_eq(view.getExtraFile(0) catch "", "src/styles/main.css");
    const next_slot = relay.serverRecycle(slot) catch unreachable;
    expect_true(next_slot == null);
});

test("HotReloadBatch: capacity limits & IndexOutOfBounds guards", fn() {
    let batch = HotReloadBatch.init();

    // IndexOutOfBounds on empty batch
    let oob = false;
    batch.getFile(0) catch |e| {
        if (e == HotReloadBatchError.IndexOutOfBounds) oob = true;
    };
    expect_true(oob);

    // Fill 32 unique directories using distinct slices from a static string
    const pool = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-_";
    let d: usize = 0;
    while (d < HOT_RELOAD_MAX_DIRS) {
        const name = pool[d .. d + 1];
        expect_true(batch.appendDir(name, null) catch false);
        d += 1;
    }
    expect_eq(batch.dirCount(), HOT_RELOAD_MAX_DIRS);

    // 33rd unique dir -> DirsCapacityExceeded
    let dir_full = false;
    batch.appendDir("overflow_dir", null) catch |e| {
        if (e == HotReloadBatchError.DirsCapacityExceeded) dir_full = true;
    };
    expect_true(dir_full);

    // Fill 64 unique files
    let f: usize = 0;
    while (f < HOT_RELOAD_MAX_FILES) {
        const fname = pool[f .. f + 1];
        expect_true(batch.appendFile(fname) catch false);
        f += 1;
    }
    expect_eq(batch.fileCount(), HOT_RELOAD_MAX_FILES);

    // 65th unique file -> FilesCapacityExceeded
    let file_full = false;
    batch.appendFile("overflow_file.ts") catch |e| {
        if (e == HotReloadBatchError.FilesCapacityExceeded) file_full = true;
    };
    expect_true(file_full);
});
