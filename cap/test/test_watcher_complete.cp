// cap/test/test_watcher_complete.cp
// Comprehensive verification of cap:watcher standard library
import std from "std";
import { test, expect_eq } from "cap:test";
import {
    WatchAction,
    WatchEventOp,
    WatchEvent,
    WatchOptions,
    WatchItemKind,
    Watcher,
    DebounceEngine,
    InotifyBackend,
    createWatcher,
    DEFAULT_WATCH_MASK,
    IN_MODIFY,
    IN_CREATE,
    IN_DELETE,
    IN_ISDIR,
} from "cap:watcher";

struct CallbackTracker {
    pub count: u32 = 0,
    pub last_action: WatchAction = WatchAction.modify,
    pub last_is_dir: bool = false,
}

fn onWatchEvent(event: WatchEvent, ctx_ptr: *anyopaque) {
    let tracker: *CallbackTracker = unsafe { @ptrCast(@alignCast(ctx_ptr)) };
    tracker.count += 1;
    tracker.last_action = event.action;
    tracker.last_is_dir = event.is_dir;
}

fn test_events_and_op_merging(): !void {
    // Test WatchEventOp initialization
    let op = WatchEventOp.init();
    expect_eq(op.hasChanges(), false);
    expect_eq(op.create, false);
    expect_eq(op.modify, false);
    expect_eq(op.delete, false);

    // Merge operations: create + modify -> toAction() == .create
    let op_create = WatchEventOp{ create: true };
    let op_modify = WatchEventOp{ modify: true };
    let merged1 = WatchEventOp.merge(op_create, op_modify);
    expect_eq(merged1.hasChanges(), true);
    expect_eq(merged1.create, true);
    expect_eq(merged1.modify, true);
    expect_eq(merged1.toAction() == WatchAction.create, true);

    // modify + delete -> toAction() == .delete
    let op_delete = WatchEventOp{ delete: true };
    let merged2 = WatchEventOp.merge(op_modify, op_delete);
    expect_eq(merged2.toAction() == WatchAction.delete, true);

    // modify + rename -> toAction() == .rename
    let op_rename = WatchEventOp{ rename: true };
    let merged3 = WatchEventOp.merge(op_modify, op_rename);
    expect_eq(merged3.toAction() == WatchAction.rename, true);

    // Single modify -> toAction() == .modify
    expect_eq(op_modify.toAction() == WatchAction.modify, true);

    // WatchEvent fields
    let event = WatchEvent{
        path: "src/main.cp",
        action: WatchAction.modify,
        is_dir: false,
        timestamp: 123456,
        cookie: 99,
    };
    expect_eq(std.mem.eql(u8, event.path, "src/main.cp"), true);
    expect_eq(event.action == WatchAction.modify, true);
    expect_eq(event.is_dir, false);
    expect_eq(event.timestamp, 123456);
    expect_eq(event.cookie, 99);
}

fn test_debounce_coalescing(): !void {
    let engine = DebounceEngine.init(50);
    expect_eq(engine.pendingCount(), 0);

    // Push 3 rapid events for the same file within debounce window
    let p1 = engine.push("src/service.cp", WatchAction.modify, false, 100);
    expect_eq(p1, true);
    expect_eq(engine.pendingCount(), 1);

    let p2 = engine.push("src/service.cp", WatchAction.modify, false, 115);
    expect_eq(p2, true);
    expect_eq(engine.pendingCount(), 1);

    let p3 = engine.push("src/service.cp", WatchAction.modify, false, 130);
    expect_eq(p3, true);
    expect_eq(engine.pendingCount(), 1);

    // Verify debounce entry state
    let idx_opt = engine.findIndex("src/service.cp");
    expect_eq(idx_opt != null, true);
    let idx = idx_opt.?;
    expect_eq(engine.entries[idx].count, 3);
    expect_eq(engine.entries[idx].first_time_ms, 100);
    expect_eq(engine.entries[idx].last_time_ms, 130);

    // At now_ms = 150 (elapsed = 20 < 50), events are NOT ready
    let out_events: [16]WatchEvent = undefined;
    let ready_count1 = engine.drainReady(150, &out_events);
    expect_eq(ready_count1, 0);
    expect_eq(engine.pendingCount(), 1);

    // At now_ms = 185 (elapsed = 55 >= 50), event IS ready
    let ready_count2 = engine.drainReady(185, &out_events);
    expect_eq(ready_count2, 1);
    expect_eq(engine.pendingCount(), 0);
    expect_eq(std.mem.eql(u8, out_events[0].path, "src/service.cp"), true);
    expect_eq(out_events[0].action == WatchAction.modify, true);

    // Push 2 distinct files and test drainAll
    * = engine.push("file_a.txt", WatchAction.create, false, 200);
    * = engine.push("file_b.txt", WatchAction.delete, true, 201);
    expect_eq(engine.pendingCount(), 2);

    let drain_all_count = engine.drainAll(&out_events);
    expect_eq(drain_all_count, 2);
    expect_eq(engine.pendingCount(), 0);
}

fn test_watcher_registration_and_lifecycle(): !void {
    let opts = WatchOptions{ recursive: true, debounce_ms: 40 };
    let watcher = createWatcher(opts);

    expect_eq(watcher.isActive(), true);
    expect_eq(watcher.isPaused(), false);
    expect_eq(watcher.count(), 0);

    // Watch files/directories
    let w1 = watcher.watch("src");
    expect_eq(w1, true);
    expect_eq(watcher.count(), 1);
    expect_eq(watcher.isWatched("src"), true);

    let w2 = watcher.watch("build.cp");
    expect_eq(w2, true);
    expect_eq(watcher.count(), 2);
    expect_eq(watcher.isWatched("build.cp"), true);

    // Duplicate watch does not duplicate count
    let w_dup = watcher.watch("src");
    expect_eq(w_dup, true);
    expect_eq(watcher.count(), 2);

    // Non-watched check
    expect_eq(watcher.isWatched("non_existent"), false);

    // Unwatch
    let unw = watcher.unwatch("src");
    expect_eq(unw, true);
    expect_eq(watcher.count(), 1);
    expect_eq(watcher.isWatched("src"), false);

    // Unwatch non-watched
    let unw_fail = watcher.unwatch("src");
    expect_eq(unw_fail, false);

    // Pause / resume
    watcher.pause();
    expect_eq(watcher.isPaused(), true);

    watcher.resumeWatch();
    expect_eq(watcher.isPaused(), false);

    // Close
    watcher.close();
    expect_eq(watcher.isActive(), false);
    expect_eq(watcher.count(), 0);
}

fn test_mock_dispatch_and_callback(): !void {
    let watcher = createWatcher(WatchOptions{ debounce_ms: 30 });
    * = watcher.watch("test_dir");

    let tracker = CallbackTracker{};
    watcher.setCallback(onWatchEvent, &tracker);

    // Dispatch mock event
    let d1 = watcher.dispatchMockEvent("test_dir/file.txt", WatchAction.create, false, 100);
    expect_eq(d1, true);

    // Poll before debounce window (now_ms = 110)
    let events: [8]WatchEvent = undefined;
    let n1 = watcher.poll(110, &events);
    expect_eq(n1, 0);
    expect_eq(tracker.count, 0);

    // Poll after debounce window (now_ms = 135)
    let n2 = watcher.poll(135, &events);
    expect_eq(n2, 1);
    expect_eq(tracker.count, 1);
    expect_eq(tracker.last_action == WatchAction.create, true);
    expect_eq(tracker.last_is_dir, false);

    // Dispatch directory delete event
    * = watcher.dispatchMockEvent("test_dir/sub", WatchAction.delete, true, 200);
    let n3 = watcher.drainEvents(&events);
    expect_eq(n3, 1);
    expect_eq(tracker.count, 2);
    expect_eq(tracker.last_action == WatchAction.delete, true);
    expect_eq(tracker.last_is_dir, true);

    watcher.close();
}

fn test_path_resolution_and_formatting(): !void {
    let watcher = createWatcher(WatchOptions{});
    * = watcher.watch("packages/core");
    * = watcher.watch("packages/utils/");

    let path_no_slash = watcher.resolvePath(0, "index.cp");
    expect_eq(std.mem.eql(u8, path_no_slash, "packages/core/index.cp"), true);

    let path_with_slash = watcher.resolvePath(1, "helper.cp");
    expect_eq(std.mem.eql(u8, path_with_slash, "packages/utils/helper.cp"), true);

    let path_empty_name = watcher.resolvePath(0, "");
    expect_eq(std.mem.eql(u8, path_empty_name, "packages/core"), true);

    watcher.close();
}

fn test_inotify_backend_capabilities(): !void {
    let backend = InotifyBackend.init();

    // Verify constants and mask
    expect_eq((DEFAULT_WATCH_MASK & IN_MODIFY) != 0, true);
    expect_eq((DEFAULT_WATCH_MASK & IN_CREATE) != 0, true);
    expect_eq((DEFAULT_WATCH_MASK & IN_DELETE) != 0, true);

    // On Linux x86_64, inotify is available
    if (backend.isAvailable()) {
        expect_eq(backend.fd >= 0, true);

        // Add watch on current workspace
        let wd = backend.addWatch(".", DEFAULT_WATCH_MASK);
        expect_eq(wd >= 0, true);

        // Remove watch
        let rm_ok = backend.removeWatch(wd);
        expect_eq(rm_ok, true);
    }

    backend.close();
    expect_eq(backend.isAvailable(), false);
    expect_eq(backend.fd, -1);
}

fn main() {
    test("1. cap:watcher — WatchEvent, WatchAction & WatchEventOp coalescing flags", () => {
        test_events_and_op_merging()?;
    });

    test("2. cap:watcher — DebounceEngine time-window event coalescing", () => {
        test_debounce_coalescing()?;
    });

    test("3. cap:watcher — Watcher registration, unwatch & pause/resume lifecycle", () => {
        test_watcher_registration_and_lifecycle()?;
    });

    test("4. cap:watcher — Mock event dispatching & callback invocation", () => {
        test_mock_dispatch_and_callback()?;
    });

    test("5. cap:watcher — Directory path concatenation and resolution", () => {
        test_path_resolution_and_formatting()?;
    });

    test("6. cap:watcher — Linux InotifyBackend native kernel integration", () => {
        test_inotify_backend_capabilities()?;
    });
}
