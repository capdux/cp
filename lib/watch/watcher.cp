// Source: Bun src/watcher/INotifyWatcher.cp
// lib/watch/watcher.cp
// Cross-Platform Filesystem Watcher & Linux inotify / macOS kqueue Event Abstraction
// Absorbed from Bun src/watcher/INotifyWatcher.cp, KEventWatcher.cp & Watcher.cp

const std = @import("std");

pub enum WatchEventKind: u8 {
    Unknown = 0,
    Created = 1,
    Modified = 2,
    Deleted = 3,
    Renamed = 4,
    MovedSelf = 5,
    Attrib = 6,
}

pub struct WatchMask {
    pub const IN_ACCESS: u32 = 0x00000001;
    pub const IN_MODIFY: u32 = 0x00000002;
    pub const IN_ATTRIB: u32 = 0x00000004;
    pub const IN_CLOSE_WRITE: u32 = 0x00000008;
    pub const IN_MOVED_FROM: u32 = 0x00000040;
    pub const IN_MOVED_TO: u32 = 0x00000080;
    pub const IN_CREATE: u32 = 0x00000100;
    pub const IN_DELETE: u32 = 0x00000200;
    pub const IN_DELETE_SELF: u32 = 0x00000400;
    pub const IN_MOVE_SELF: u32 = 0x00000800;
    pub const IN_ISDIR: u32 = 0x40000000;

    pub const DEFAULT_FILE_MASK: u32 = 0x00000002 | 0x00000080 | 0x00000400 | 0x00000800;
    pub const DEFAULT_DIR_MASK: u32 = 0x00000002 | 0x00000040 | 0x00000080 | 0x00000100 | 0x00000200 | 0x00000400 | 0x00000800;
}

pub struct WatchEvent {
    wd: i32 = -1;
    mask: u32 = 0;
    cookie: u32 = 0;
    name: []const u8 = "";

    pub fn kind(): WatchEventKind {
        if ((_.mask & WatchMask.IN_CREATE) != 0) {
            return WatchEventKind.Created;
        }
        if ((_.mask & (WatchMask.IN_DELETE | WatchMask.IN_DELETE_SELF)) != 0) {
            return WatchEventKind.Deleted;
        }
        if ((_.mask & (WatchMask.IN_MOVED_FROM | WatchMask.IN_MOVED_TO)) != 0) {
            return WatchEventKind.Renamed;
        }
        if ((_.mask & WatchMask.IN_MOVE_SELF) != 0) {
            return WatchEventKind.MovedSelf;
        }
        if ((_.mask & (WatchMask.IN_MODIFY | WatchMask.IN_CLOSE_WRITE)) != 0) {
            return WatchEventKind.Modified;
        }
        if ((_.mask & WatchMask.IN_ATTRIB) != 0) {
            return WatchEventKind.Attrib;
        }
        return WatchEventKind.Unknown;
    }

    pub fn isDirectory(): bool {
        return (_.mask & WatchMask.IN_ISDIR) != 0;
    }
}

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
