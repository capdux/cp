// Type definitions for CP

import { FastGlobOptions, GlobPatternSet } from "../../fast-glob/src/index.cp";
export fn chkStrEq(a: string, b: string): bool;
export class WatchEvent {
    kind: u8,
    path: string,
    mtime_ms: u64,
    size: u64,
}

export class ChokidarOptions {
    ignore_initial: bool,
    await_write_finish_ms: u64,
    follow_symlinks: bool,
    depth: usize,
    ignore_dotfiles: bool,
    fn defaults(): ChokidarOptions;
}

export class TrackedEntry {
    active: bool,
    path: string,
    is_dir: bool,
    mtime_ms: u64,
    size: u64,
    last_emit_ms: u64,
}

export class FileWatcherCore {
    opts: ChokidarOptions,
    matcher: GlobPatternSet,
    tracked: [32]TrackedEntry,
    tracked_count: usize,
    events: [32]WatchEvent,
    event_head: usize,
    event_len: usize,
    fn new(opts: ChokidarOptions): FileWatcherCore;
    fn addIgnore(_: auto, pattern: string): bool;
    fn isIgnored(path: string, is_dir: bool): bool;
    fn chkEnqueue(_: auto, kind: u8, path: string, mtime_ms: u64, size: u64);
    fn recordSnapshot(_: auto, path: string, is_dir: bool, mtime_ms: u64, size: u64, is_initial_scan: bool): u8;
    fn recordUnlink(_: auto, path: string, is_dir: bool): u8;
    fn eventCount(): usize;
    fn trackedCount(): usize;
    fn popEvent(_: auto): WatchEvent?;
}

