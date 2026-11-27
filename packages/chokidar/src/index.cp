// @cap/chokidar-core — Pure CP zero-allocation file watcher core state machine.
// Clean-room MIT implementation inspired by npm `chokidar` v4.
// Reuses `@cap/fast-glob` (`FastGlobOptions`, `GlobPatternSet`) for ignore/dot/depth filtering.

import { FastGlobOptions, GlobPatternSet } from "../../fast-glob/src/index.cp";

pub const EVT_NONE: u8 = 0;
pub const EVT_ADD: u8 = 1;
pub const EVT_CHANGE: u8 = 2;
pub const EVT_UNLINK: u8 = 3;
pub const EVT_ADD_DIR: u8 = 4;
pub const EVT_UNLINK_DIR: u8 = 5;

pub const CHK_MAX_TRACKED: usize = 32;
pub const CHK_MAX_EVENTS: usize = 32;

fn chkStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

/// A filesystem watch event emitted by `FileWatcherCore`.
pub struct WatchEvent {
    pub kind: u8,
    pub path: string,
    pub mtime_ms: u64,
    pub size: u64,
}

/// Configuration options for `FileWatcherCore` (inspired by `chokidar` v4).
pub struct ChokidarOptions {
    pub ignore_initial: bool,
    pub await_write_finish_ms: u64,
    pub follow_symlinks: bool,
    pub depth: usize,
    pub ignore_dotfiles: bool,

    pub static fn defaults(): ChokidarOptions {
        return ChokidarOptions {
            ignore_initial: false,
            await_write_finish_ms: 0 as u64,
            follow_symlinks: true,
            depth: 0,
            ignore_dotfiles: true
        };
    }
}

/// Internal tracked snapshot entry for a watched file or directory.
pub struct TrackedEntry {
    pub active: bool,
    pub path: string,
    pub is_dir: bool,
    pub mtime_ms: u64,
    pub size: u64,
    pub last_emit_ms: u64,
}

/// Zero-heap file watcher core with 32 tracked snapshot slots and 32 FIFO event slots.
pub struct FileWatcherCore {
    pub opts: ChokidarOptions,
    pub matcher: GlobPatternSet,
    pub tracked: [32]TrackedEntry,
    pub tracked_count: usize,
    pub events: [32]WatchEvent,
    pub event_head: usize,
    pub event_len: usize,

    pub static fn new(opts: ChokidarOptions): FileWatcherCore {
        let fg_opts = FastGlobOptions.defaults();
        fg_opts.dot = !opts.ignore_dotfiles;
        fg_opts.deep = opts.depth;
        fg_opts.only_files = false;
        fg_opts.only_directories = false;
        fg_opts.case_sensitive = true;

        let m = GlobPatternSet.new(fg_opts);
        * = m.addPattern("**/*");

        let blank_entry = TrackedEntry {
            active: false,
            path: "",
            is_dir: false,
            mtime_ms: 0 as u64,
            size: 0 as u64,
            last_emit_ms: 0 as u64
        };
        let tracked_arr: [32]TrackedEntry = [blank_entry; 32];

        let blank_evt = WatchEvent {
            kind: EVT_NONE,
            path: "",
            mtime_ms: 0 as u64,
            size: 0 as u64
        };
        let evt_arr: [32]WatchEvent = [blank_evt; 32];

        return FileWatcherCore {
            opts: opts,
            matcher: m,
            tracked: tracked_arr,
            tracked_count: 0,
            events: evt_arr,
            event_head: 0,
            event_len: 0
        };
    }

    /// Registers an ignore glob pattern via the embedded `@cap/fast-glob` `GlobPatternSet`.
    pub fn addIgnore(*_, pattern: string): bool {
        if (pattern.len == 0) {
            return false;
        }
        if (pattern[0] == 33 && pattern.len > 1) {
            return _.matcher.addIgnore(pattern[1..pattern.len]);
        }
        return _.matcher.addIgnore(pattern);
    }

    /// Returns `true` if `path` should be ignored according to ignore globs, dotfiles, and depth.
    pub fn isIgnored(path: string, is_dir: bool): bool {
        return !_.matcher.matches(path, is_dir);
    }

    /// Enqueues a watch event into the fixed 32-slot FIFO ring buffer.
    fn chkEnqueue(*_, kind: u8, path: string, mtime_ms: u64, size: u64) {
        if (_.event_len >= CHK_MAX_EVENTS) {
            return;
        }
        let tail = (_.event_head + _.event_len) % CHK_MAX_EVENTS;
        _.events[tail] = WatchEvent {
            kind: kind,
            path: path,
            mtime_ms: mtime_ms,
            size: size
        };
        _.event_len = _.event_len + 1;
    }

    /// Records a file/directory stat snapshot, diffing against previous state to emit
    /// `EVT_ADD`, `EVT_ADD_DIR`, or `EVT_CHANGE` while honoring `ignore_initial` and debounce.
    pub fn recordSnapshot(
        *_,
        path: string,
        is_dir: bool,
        mtime_ms: u64,
        size: u64,
        is_initial_scan: bool
    ): u8 {
        if (_.isIgnored(path, is_dir)) {
            return EVT_NONE;
        }

        // Check if already tracked.
        let i: usize = 0;
        let free_slot: usize = CHK_MAX_TRACKED;
        while (i < CHK_MAX_TRACKED) {
            if (_.tracked[i].active) {
                if (chkStrEq(_.tracked[i].path, path)) {
                    if (is_dir) {
                        return EVT_NONE;
                    }
                    if (_.tracked[i].mtime_ms == mtime_ms && _.tracked[i].size == size) {
                        return EVT_NONE;
                    }
                    _.tracked[i].mtime_ms = mtime_ms;
                    _.tracked[i].size = size;

                    if (_.opts.await_write_finish_ms > (0 as u64) &&
                        mtime_ms >= _.tracked[i].last_emit_ms &&
                        (mtime_ms - _.tracked[i].last_emit_ms) < _.opts.await_write_finish_ms) {
                        return EVT_NONE;
                    }

                    _.tracked[i].last_emit_ms = mtime_ms;
                    _.chkEnqueue(EVT_CHANGE, path, mtime_ms, size);
                    return EVT_CHANGE;
                }
            } else if (free_slot == CHK_MAX_TRACKED) {
                free_slot = i;
            }
            i = i + 1;
        }

        // New entry.
        if (free_slot >= CHK_MAX_TRACKED) {
            return EVT_NONE;
        }
        _.tracked[free_slot] = TrackedEntry {
            active: true,
            path: path,
            is_dir: is_dir,
            mtime_ms: mtime_ms,
            size: size,
            last_emit_ms: mtime_ms
        };
        _.tracked_count = _.tracked_count + 1;

        if (is_initial_scan && _.opts.ignore_initial) {
            return EVT_NONE;
        }

        let kind = EVT_ADD;
        if (is_dir) {
            kind = EVT_ADD_DIR;
        }
        _.chkEnqueue(kind, path, mtime_ms, size);
        return kind;
    }

    /// Records removal of a tracked file or directory, emitting `EVT_UNLINK` or `EVT_UNLINK_DIR`.
    pub fn recordUnlink(*_, path: string, is_dir: bool): u8 {
        if (_.isIgnored(path, is_dir)) {
            return EVT_NONE;
        }
        let i: usize = 0;
        while (i < CHK_MAX_TRACKED) {
            if (_.tracked[i].active && chkStrEq(_.tracked[i].path, path)) {
                let was_dir = _.tracked[i].is_dir || is_dir;
                let prev_mtime = _.tracked[i].mtime_ms;
                _.tracked[i].active = false;
                _.tracked_count = _.tracked_count - 1;
                let kind = EVT_UNLINK;
                if (was_dir) {
                    kind = EVT_UNLINK_DIR;
                }
                _.chkEnqueue(kind, path, prev_mtime, 0 as u64);
                return kind;
            }
            i = i + 1;
        }
        return EVT_NONE;
    }

    /// Returns the number of queued watch events waiting to be consumed.
    pub fn eventCount(): usize {
        return _.event_len;
    }

    /// Returns the number of currently tracked files and directories.
    pub fn trackedCount(): usize {
        return _.tracked_count;
    }

    /// Pops the oldest pending `WatchEvent`, or `null` if the queue is empty.
    pub fn popEvent(*_): WatchEvent? {
        if (_.event_len == 0) {
            return null;
        }
        let ev = _.events[_.event_head];
        _.event_head = (_.event_head + 1) % CHK_MAX_EVENTS;
        _.event_len = _.event_len - 1;
        return ev;
    }
}
