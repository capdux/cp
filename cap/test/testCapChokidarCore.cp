import { test, expect_eq } from "cap:test";
import {
    EVT_NONE,
    EVT_ADD,
    EVT_CHANGE,
    EVT_UNLINK,
    EVT_ADD_DIR,
    EVT_UNLINK_DIR,
    ChokidarOptions,
    FileWatcherCore
} from "../../packages/@cap/chokidar-core/src/index.cp";

fn main() {
    test("chokidar-core: emits ADD, CHANGE, UNLINK and ADD_DIR, UNLINK_DIR in FIFO order", () => {
        let watcher = FileWatcherCore.new(ChokidarOptions.defaults());

        expect_eq(watcher.recordSnapshot("src", true, 100 as u64, 0 as u64, false), EVT_ADD_DIR);
        expect_eq(watcher.recordSnapshot("src/app.cp", false, 200 as u64, 50 as u64, false), EVT_ADD);
        // Same mtime and size produces no event
        expect_eq(watcher.recordSnapshot("src/app.cp", false, 200 as u64, 50 as u64, false), EVT_NONE);
        // Updated mtime/size emits CHANGE
        expect_eq(watcher.recordSnapshot("src/app.cp", false, 300 as u64, 64 as u64, false), EVT_CHANGE);
        expect_eq(watcher.trackedCount(), 2 as usize);

        expect_eq(watcher.recordUnlink("src/app.cp", false), EVT_UNLINK);
        expect_eq(watcher.recordUnlink("src", true), EVT_UNLINK_DIR);
        expect_eq(watcher.trackedCount(), 0 as usize);
        expect_eq(watcher.eventCount(), 5 as usize);

        let e1 = watcher.popEvent().?;
        expect_eq(e1.kind, EVT_ADD_DIR);
        expect_eq(e1.path, "src");

        let e2 = watcher.popEvent().?;
        expect_eq(e2.kind, EVT_ADD);
        expect_eq(e2.size, 50 as u64);

        let e3 = watcher.popEvent().?;
        expect_eq(e3.kind, EVT_CHANGE);
        expect_eq(e3.size, 64 as u64);

        let e4 = watcher.popEvent().?;
        expect_eq(e4.kind, EVT_UNLINK);

        let e5 = watcher.popEvent().?;
        expect_eq(e5.kind, EVT_UNLINK_DIR);

        expect_eq(watcher.popEvent() == null, true);
    });

    test("chokidar-core: delegates ignore globs, dotfiles, and depth to @cap/fast-glob", () => {
        let opts = ChokidarOptions.defaults();
        opts.depth = 2;
        opts.ignore_dotfiles = true;
        let watcher = FileWatcherCore.new(opts);
        expect_eq(watcher.addIgnore("dist/**"), true);
        expect_eq(watcher.addIgnore("*.log"), true);

        expect_eq(watcher.isIgnored("src/index.cp", false), false);
        expect_eq(watcher.isIgnored("dist/bundle.js", false), true);
        expect_eq(watcher.isIgnored("error.log", false), true);
        expect_eq(watcher.isIgnored(".env", false), true);
        expect_eq(watcher.isIgnored("a/b/deep.cp", false), true);

        expect_eq(watcher.recordSnapshot("dist/bundle.js", false, 100 as u64, 10 as u64, false), EVT_NONE);
        expect_eq(watcher.trackedCount(), 0 as usize);
    });

    test("chokidar-core: ignore_initial suppresses initial scan events while still tracking", () => {
        let opts = ChokidarOptions.defaults();
        opts.ignore_initial = true;
        let watcher = FileWatcherCore.new(opts);

        expect_eq(watcher.recordSnapshot("src/a.cp", false, 1000 as u64, 10 as u64, true), EVT_NONE);
        expect_eq(watcher.trackedCount(), 1 as usize);
        expect_eq(watcher.eventCount(), 0 as usize);

        // Subsequent modification emits CHANGE
        expect_eq(watcher.recordSnapshot("src/a.cp", false, 1200 as u64, 20 as u64, false), EVT_CHANGE);
        expect_eq(watcher.eventCount(), 1 as usize);
        let ev = watcher.popEvent().?;
        expect_eq(ev.kind, EVT_CHANGE);
        expect_eq(ev.path, "src/a.cp");
    });

    test("chokidar-core: await_write_finish_ms debounces rapid consecutive writes", () => {
        let opts = ChokidarOptions.defaults();
        opts.await_write_finish_ms = 500 as u64;
        let watcher = FileWatcherCore.new(opts);

        expect_eq(watcher.recordSnapshot("data.bin", false, 1000 as u64, 10 as u64, false), EVT_ADD);
        // Within 500ms window -> suppressed
        expect_eq(watcher.recordSnapshot("data.bin", false, 1200 as u64, 20 as u64, false), EVT_NONE);
        expect_eq(watcher.recordSnapshot("data.bin", false, 1499 as u64, 30 as u64, false), EVT_NONE);
        // At 1500ms (>= 500ms since last emit) -> emits CHANGE
        expect_eq(watcher.recordSnapshot("data.bin", false, 1500 as u64, 40 as u64, false), EVT_CHANGE);
        expect_eq(watcher.eventCount(), 2 as usize);
    });

    test("chokidar-core: allow dotfiles when ignore_dotfiles is false", () => {
        let opts = ChokidarOptions.defaults();
        opts.ignore_dotfiles = false;
        let watcher = FileWatcherCore.new(opts);

        expect_eq(watcher.isIgnored(".env.local", false), false);
        expect_eq(watcher.recordSnapshot(".env.local", false, 500 as u64, 12 as u64, false), EVT_ADD);
        expect_eq(watcher.trackedCount(), 1 as usize);
    });

    test("chokidar-core: unlinking untracked path returns EVT_NONE and reuses freed slots", () => {
        let watcher = FileWatcherCore.new(ChokidarOptions.defaults());
        expect_eq(watcher.recordUnlink("never-added.cp", false), EVT_NONE);

        expect_eq(watcher.recordSnapshot("temp.cp", false, 10 as u64, 5 as u64, false), EVT_ADD);
        expect_eq(watcher.recordUnlink("temp.cp", false), EVT_UNLINK);
        expect_eq(watcher.trackedCount(), 0 as usize);

        // Re-adding same path emits ADD again
        expect_eq(watcher.recordSnapshot("temp.cp", false, 20 as u64, 8 as u64, false), EVT_ADD);
        expect_eq(watcher.trackedCount(), 1 as usize);
    });
}
