// Source: Absorbed from /data/dev/bun/src/watcher/Watcher.cp | Synced: 2026-11-22T12:49:17Z
import std from "std";

pub enum WatchAction {
    create,
    modify,
    delete,
    rename,
}

pub struct WatchEventOp {
    pub create: bool = false,
    pub modify: bool = false,
    pub delete: bool = false,
    pub rename: bool = false,
    pub metadata: bool = false,
    pub move_to: bool = false,

    pub static fn init(): WatchEventOp {
        return WatchEventOp{};
    }

    pub static fn merge(before: WatchEventOp, after: WatchEventOp): WatchEventOp {
        return WatchEventOp{
            create: before.create || after.create,
            modify: before.modify || after.modify,
            delete: before.delete || after.delete,
            rename: before.rename || after.rename,
            metadata: before.metadata || after.metadata,
            move_to: before.move_to || after.move_to,
        };
    }

    pub fn toAction(): WatchAction {
        if (_.delete) {
            return WatchAction.delete;
        }
        if (_.rename || _.move_to) {
            return WatchAction.rename;
        }
        if (_.create) {
            return WatchAction.create;
        }
        return WatchAction.modify;
    }

    pub fn hasChanges(): bool {
        return _.create || _.modify || _.delete || _.rename || _.metadata || _.move_to;
    }
}

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
    pub path: []const u8 = "",
    pub action: WatchAction = WatchAction.modify,
    pub is_dir: bool = false,
    pub timestamp: i64 = 0,
    pub cookie: u32 = 0,
    pub wd: i32 = -1,
    pub mask: u32 = 0,
    pub name: []const u8 = "",

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
        if (_.is_dir) return true;
        return (_.mask & WatchMask.IN_ISDIR) != 0;
    }
}

pub enum WatchItemKind {
    file,
    directory,
}

pub struct WatchOptions {
    pub recursive: bool = true,
    pub debounce_ms: i64 = 50,
    pub ignore_defaults: bool = true,
    pub ignore_hidden: bool = false,
    pub ignore_patterns: []const []const u8 = [],
}

pub type WatchCallback = *const fn (event: WatchEvent, ctx: *anyopaque) void;

pub fn getNowMs(): i64 {
    let ts: std.os.linux.timespec = .{ .sec = 0, .nsec = 0 };
    __zig__ {
        _ = std.os.linux.clock_gettime(.MONOTONIC, &ts);
    }
    return (ts.sec * 1000) + @divTrunc(ts.nsec, 1_000_000);
}
