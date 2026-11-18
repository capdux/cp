// Source: bun/src/watcher/Watcher.cp
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

pub struct WatchEvent {
    pub path: []const u8,
    pub action: WatchAction,
    pub is_dir: bool = false,
    pub timestamp: i64 = 0,
    pub cookie: u32 = 0,
}

pub enum WatchItemKind {
    file,
    directory,
}

pub struct WatchOptions {
    pub recursive: bool = true,
    pub debounce_ms: i64 = 50,
}

pub type WatchCallback = *const fn (event: WatchEvent, ctx: *anyopaque) void;

pub fn getNowMs(): i64 {
    let ts: std.os.linux.timespec = .{ .sec = 0, .nsec = 0 };
    __zig__ {
        _ = std.os.linux.clock_gettime(.MONOTONIC, &ts);
    }
    return (ts.sec * 1000) + @divTrunc(ts.nsec, 1_000_000);
}
