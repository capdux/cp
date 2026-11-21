// Source: Absorbed from /data/dev/bun/src/watcher/Watcher.cp | Synced: 2026-11-21T21:12:37Z
import std from "std";
import { WatchAction, WatchEvent, WatchEventOp } from "./events.cp";

pub const MAX_DEBOUNCE_ENTRIES: usize = 64;
pub const DEBOUNCE_PATH_MAX: usize = 256;

pub struct DebounceEntry {
    pub path_buf: [DEBOUNCE_PATH_MAX]u8 = undefined,
    pub path_len: usize = 0,
    pub op: WatchEventOp = WatchEventOp{},
    pub is_dir: bool = false,
    pub first_time_ms: i64 = 0,
    pub last_time_ms: i64 = 0,
    pub active: bool = false,
    pub count: u32 = 0,

    pub fn path(*_): []const u8 {
        return _.path_buf[0.._.path_len];
    }
}

pub struct DebounceEngine {
    pub entries: [MAX_DEBOUNCE_ENTRIES]DebounceEntry = undefined,
    pub count: usize = 0,
    pub debounce_ms: i64 = 50,

    pub static fn init(debounce_ms: i64): DebounceEngine {
        let engine = DebounceEngine{
            entries: undefined,
            count: 0,
            debounce_ms: debounce_ms,
        };
        let i: usize = 0;
        while (i < MAX_DEBOUNCE_ENTRIES) {
            engine.entries[i].active = false;
            engine.entries[i].count = 0;
            engine.entries[i].path_len = 0;
            engine.entries[i].op = WatchEventOp{};
            i += 1;
        }
        return engine;
    }

    pub fn findIndex(path: []const u8): (usize)? {
        let i: usize = 0;
        while (i < MAX_DEBOUNCE_ENTRIES) {
            if (_.entries[i].active) {
                let entry_path = _.entries[i].path_buf[0.._.entries[i].path_len];
                if (std.mem.eql(u8, entry_path, path)) {
                    return i;
                }
            }
            i += 1;
        }
        return null;
    }

    pub fn push(*_, path: []const u8, action: WatchAction, is_dir: bool, now_ms: i64): bool {
        if (path.len > DEBOUNCE_PATH_MAX) {
            return false;
        }

        if (idx_opt := _.findIndex(path)) {
            let idx = idx_opt;
            let new_op = WatchEventOp{};
            switch (action) {
                WatchAction.create => new_op.create = true,
                WatchAction.modify => new_op.modify = true,
                WatchAction.delete => new_op.delete = true,
                WatchAction.rename => new_op.rename = true,
            }
            _.entries[idx].op = WatchEventOp.merge(_.entries[idx].op, new_op);
            _.entries[idx].last_time_ms = now_ms;
            _.entries[idx].count += 1;
            _.entries[idx].is_dir = is_dir;
            return true;
        }

        let free_idx: (usize)? = null;
        let i: usize = 0;
        while (i < MAX_DEBOUNCE_ENTRIES) {
            if (!_.entries[i].active) {
                free_idx = i;
                break;
            }
            i += 1;
        }

        if (free_idx == null) {
            return false;
        }

        let idx = free_idx.?;
        let op = WatchEventOp{};
        switch (action) {
            WatchAction.create => op.create = true,
            WatchAction.modify => op.modify = true,
            WatchAction.delete => op.delete = true,
            WatchAction.rename => op.rename = true,
        }

        @memcpy(_.entries[idx].path_buf[0..path.len], path);
        _.entries[idx].path_len = path.len;
        _.entries[idx].op = op;
        _.entries[idx].is_dir = is_dir;
        _.entries[idx].first_time_ms = now_ms;
        _.entries[idx].last_time_ms = now_ms;
        _.entries[idx].active = true;
        _.entries[idx].count = 1;
        _.count += 1;
        return true;
    }

    pub fn drainReady(*_, now_ms: i64, out_events: *[]WatchEvent): usize {
        let out_idx: usize = 0;
        let i: usize = 0;
        while (i < MAX_DEBOUNCE_ENTRIES && out_idx < out_events.len) {
            if (_.entries[i].active) {
                let elapsed = now_ms - _.entries[i].last_time_ms;
                if (elapsed >= _.debounce_ms) {
                    out_events[out_idx] = WatchEvent{
                        path: _.entries[i].path_buf[0.._.entries[i].path_len],
                        action: _.entries[i].op.toAction(),
                        is_dir: _.entries[i].is_dir,
                        timestamp: _.entries[i].last_time_ms,
                        cookie: 0,
                    };
                    out_idx += 1;
                    _.entries[i].active = false;
                    if (_.count > 0) {
                        _.count -= 1;
                    }
                }
            }
            i += 1;
        }
        return out_idx;
    }

    pub fn drainAll(*_, out_events: *[]WatchEvent): usize {
        let out_idx: usize = 0;
        let i: usize = 0;
        while (i < MAX_DEBOUNCE_ENTRIES && out_idx < out_events.len) {
            if (_.entries[i].active) {
                out_events[out_idx] = WatchEvent{
                    path: _.entries[i].path_buf[0.._.entries[i].path_len],
                    action: _.entries[i].op.toAction(),
                    is_dir: _.entries[i].is_dir,
                    timestamp: _.entries[i].last_time_ms,
                    cookie: 0,
                };
                out_idx += 1;
                _.entries[i].active = false;
                if (_.count > 0) {
                    _.count -= 1;
                }
            }
            i += 1;
        }
        return out_idx;
    }

    pub fn clear(*_) {
        let i: usize = 0;
        while (i < MAX_DEBOUNCE_ENTRIES) {
            _.entries[i].active = false;
            _.entries[i].count = 0;
            _.entries[i].path_len = 0;
            i += 1;
        }
        _.count = 0;
    }

    pub fn pendingCount(): usize {
        return _.count;
    }
}
