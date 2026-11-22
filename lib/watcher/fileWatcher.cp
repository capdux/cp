// Source: Absorbed from /data/dev/bun/src/watcher/INotifyWatcher.cp | Synced: 2026-11-22T12:49:17Z
// lib/watcher/file_watcher.cp
// Cross-Platform Filesystem Watcher & Linux inotify / macOS kqueue Event Abstraction
// Absorbed from Bun src/watcher/INotifyWatcher.cp, KEventWatcher.cp & Watcher.cp

const std = @import("std");

import {
    WatchEventKind,
    WatchMask,
    WatchEvent,
} from "./events.cp";

export {
    WatchEventKind,
    WatchMask,
    WatchEvent,
};

pub struct WatchEntry {
    wd: i32 = -1;
    path: []const u8 = "";
    is_dir: bool = false;
    active: bool = false;
}

pub struct FileWatcher {
    entries: [64]WatchEntry = undefined;
    entry_count: usize = 0;
    next_wd: i32 = 1;
    coalesce_interval_ns: i64 = 100000;

    pub static fn init(): FileWatcher {
        return FileWatcher {
            entries: undefined,
            entry_count: 0,
            next_wd: 1,
            coalesce_interval_ns: 100000,
        };
    }

    pub fn addWatch(*_, path: []const u8, is_dir: bool): i32 {
        // Reuse existing entry if already registered
        let i: usize = 0;
        while (i < _.entry_count) {
            if (_.entries[i].active && std.mem.eql(u8, _.entries[i].path, path)) {
                return _.entries[i].wd;
            }
            i += 1;
        }

        if (_.entry_count >= 64) {
            return -1;
        }

        const wd = _.next_wd;
        _.next_wd += 1;
        _.entries[_.entry_count] = WatchEntry {
            wd: wd,
            path: path,
            is_dir: is_dir,
            active: true,
        };
        _.entry_count += 1;
        return wd;
    }

    pub fn removeWatch(*_, wd: i32): bool {
        let i: usize = 0;
        while (i < _.entry_count) {
            if (_.entries[i].active && _.entries[i].wd == wd) {
                _.entries[i].active = false;
                return true;
            }
            i += 1;
        }
        return false;
    }

    pub fn findPath(wd: i32): ([]const u8)? {
        let i: usize = 0;
        while (i < _.entry_count) {
            if (_.entries[i].active && _.entries[i].wd == wd) {
                return _.entries[i].path;
            }
            i += 1;
        }
        return null;
    }

    pub fn activeCount(): usize {
        let cnt: usize = 0;
        let i: usize = 0;
        while (i < _.entry_count) {
            if (_.entries[i].active) {
                cnt += 1;
            }
            i += 1;
        }
        return cnt;
    }
}
