// Source: Absorbed from /data/dev/bun/src/watcher/Watcher.cp | Synced: 2026-11-20T11:19:58Z
import std from "std";
import {
    WatchAction,
    WatchEvent,
    WatchOptions,
    WatchItemKind,
    WatchCallback,
    getNowMs,
} from "./events.cp";
import { DebounceEngine, MAX_DEBOUNCE_ENTRIES } from "./debounce.cp";
import { WatchFilter } from "./filter.cp";
import {
    InotifyBackend,
    InotifyRawEvent,
    DEFAULT_WATCH_MASK,
    IN_MODIFY,
    IN_ATTRIB,
    IN_CLOSE_WRITE,
    IN_MOVED_FROM,
    IN_MOVED_TO,
    IN_CREATE,
    IN_DELETE,
    IN_DELETE_SELF,
    IN_MOVE_SELF,
    IN_ISDIR,
} from "./inotify.cp";

pub const MAX_WATCH_ITEMS: usize = 64;
pub const MAX_PATH_LEN: usize = 256;
pub const MAX_RAW_BATCH: usize = 32;

pub struct WatchItem {
    pub path_buf: [MAX_PATH_LEN]u8 = undefined,
    pub path_len: usize = 0,
    pub wd: i32 = -1,
    pub kind: WatchItemKind = WatchItemKind.file,
    pub active: bool = false,

    pub fn path(*_): []const u8 {
        return _.path_buf[0.._.path_len];
    }
}

pub struct Watcher {
    pub items: [MAX_WATCH_ITEMS]WatchItem = undefined,
    pub item_count: usize = 0,
    pub options: WatchOptions = WatchOptions{},
    pub debounce: DebounceEngine = DebounceEngine.init(50),
    pub backend: InotifyBackend = InotifyBackend{},
    pub is_active: bool = false,
    pub is_paused: bool = false,
    pub callback: (WatchCallback)? = null,
    pub callback_ctx: (*anyopaque)? = null,
    pub path_concat_buf: [512]u8 = undefined,

    pub static fn init(options: WatchOptions): Watcher {
        let watcher = Watcher{
            items: undefined,
            item_count: 0,
            options: options,
            debounce: DebounceEngine.init(options.debounce_ms),
            backend: InotifyBackend.init(),
            is_active: true,
            is_paused: false,
            callback: null,
            callback_ctx: null,
            path_concat_buf: undefined,
        };

        let i: usize = 0;
        while (i < MAX_WATCH_ITEMS) {
            watcher.items[i].active = false;
            watcher.items[i].path_len = 0;
            watcher.items[i].wd = -1;
            watcher.items[i].kind = WatchItemKind.file;
            i += 1;
        }

        return watcher;
    }

    pub fn isWatched(*_, path: []const u8): bool {
        let i: usize = 0;
        while (i < MAX_WATCH_ITEMS) {
            if (_.items[i].active && std.mem.eql(u8, _.items[i].path_buf[0.._.items[i].path_len], path)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    pub fn count(): usize {
        return _.item_count;
    }

    pub fn watch(*_, path: []const u8): bool {
        if (!_.is_active || path.len == 0 || path.len >= MAX_PATH_LEN) {
            return false;
        }

        if (_.isWatched(path)) {
            return true;
        }

        let free_slot: (usize)? = null;
        let i: usize = 0;
        while (i < MAX_WATCH_ITEMS) {
            if (!_.items[i].active) {
                free_slot = i;
                break;
            }
            i += 1;
        }

        if (free_slot == null) {
            return false;
        }

        let slot = free_slot.?;
        let wd: i32 = -1;
        if (_.backend.is_available) {
            wd = _.backend.addWatch(path, DEFAULT_WATCH_MASK);
        }

        @memcpy(_.items[slot].path_buf[0..path.len], path);
        _.items[slot].path_len = path.len;
        _.items[slot].wd = wd;
        _.items[slot].kind = WatchItemKind.file;
        _.items[slot].active = true;
        _.item_count += 1;
        return true;
    }

    pub fn unwatch(*_, path: []const u8): bool {
        if (!_.is_active || path.len == 0) {
            return false;
        }

        let i: usize = 0;
        while (i < MAX_WATCH_ITEMS) {
            if (_.items[i].active && std.mem.eql(u8, _.items[i].path(), path)) {
                if (_.backend.is_available && _.items[i].wd >= 0) {
                    * = _.backend.removeWatch(_.items[i].wd);
                }
                _.items[i].active = false;
                _.items[i].path_len = 0;
                _.items[i].wd = -1;
                if (_.item_count > 0) {
                    _.item_count -= 1;
                }
                return true;
            }
            i += 1;
        }
        return false;
    }

    pub fn pause(*_) {
        _.is_paused = true;
    }

    pub fn @"resume"(*_) {
        _.is_paused = false;
    }

    pub fn resumeWatch(*_) {
        _.is_paused = false;
    }

    pub fn unpause(*_) {
        _.is_paused = false;
    }

    pub fn isPaused(): bool {
        return _.is_paused;
    }

    pub fn isActive(): bool {
        return _.is_active;
    }

    pub fn setCallback(*_, cb: WatchCallback, ctx: *anyopaque) {
        _.callback = cb;
        _.callback_ctx = ctx;
    }

    pub fn shouldIgnore(*_, path: []const u8): bool {
        return WatchFilter.shouldIgnoreWith(
            path,
            _.options.ignore_defaults,
            _.options.ignore_hidden,
            _.options.ignore_patterns,
        );
    }

    pub fn dispatchMockEvent(*_, path: []const u8, action: WatchAction, is_dir: bool, now_ms: i64): bool {
        if (!_.is_active || _.is_paused) {
            return false;
        }
        if (_.shouldIgnore(path)) {
            return false;
        }
        return _.debounce.push(path, action, is_dir, now_ms);
    }

    pub fn resolvePath(*_, item_idx: usize, name: []const u8): []const u8 {
        let item_path = _.items[item_idx].path();
        if (name.len == 0) {
            @memcpy(_.path_concat_buf[0..item_path.len], item_path);
            return _.path_concat_buf[0..item_path.len];
        }

        let has_slash = (item_path.len > 0 && item_path[item_path.len - 1] == 47);
        if (has_slash) {
            let total = item_path.len + name.len;
            if (total > 512) return item_path;
            @memcpy(_.path_concat_buf[0..item_path.len], item_path);
            @memcpy(_.path_concat_buf[item_path.len..total], name);
            return _.path_concat_buf[0..total];
        } else {
            let total = item_path.len + 1 + name.len;
            if (total > 512) return item_path;
            @memcpy(_.path_concat_buf[0..item_path.len], item_path);
            _.path_concat_buf[item_path.len] = 47;
            @memcpy(_.path_concat_buf[item_path.len + 1..total], name);
            return _.path_concat_buf[0..total];
        }
    }

    pub fn poll(*_, now_ms: i64, out_events: *[]WatchEvent): usize {
        if (!_.is_active || _.is_paused) {
            return 0;
        }

        if (_.backend.is_available) {
            let raw_batch: [MAX_RAW_BATCH]InotifyRawEvent = undefined;
            let raw_count = _.backend.readEvents(&raw_batch);
            let r: usize = 0;
            while (r < raw_count) {
                let matched_idx: (usize)? = null;
                let i: usize = 0;
                while (i < MAX_WATCH_ITEMS) {
                    if (_.items[i].active && _.items[i].wd == raw_batch[r].wd) {
                        matched_idx = i;
                        break;
                    }
                    i += 1;
                }

                if (matched_idx != null) {
                    let idx = matched_idx.?;
                    let item_path = _.items[idx].path();
                    let name = raw_batch[r].name();
                    let full_path: []const u8 = item_path;
                    if (name.len > 0) {
                        full_path = _.resolvePath(idx, name);
                    }

                    let mask = raw_batch[r].mask;
                    let action = WatchAction.modify;
                    if ((mask & (IN_DELETE | IN_DELETE_SELF)) != 0) {
                        action = WatchAction.delete;
                    } else if ((mask & (IN_MOVE_SELF | IN_MOVED_FROM | IN_MOVED_TO)) != 0) {
                        action = WatchAction.rename;
                    } else if ((mask & IN_CREATE) != 0) {
                        action = WatchAction.create;
                    } else {
                        action = WatchAction.modify;
                    }
                    let is_dir = (mask & IN_ISDIR) != 0;
                    if (!_.shouldIgnore(full_path)) {
                        * = _.debounce.push(full_path, action, is_dir, now_ms);
                    }
                }
                r += 1;
            }
        }

        let drained_count = _.debounce.drainReady(now_ms, out_events);
        if (cb := _.callback) {
            if (ctx := _.callback_ctx) {
                let d: usize = 0;
                while (d < drained_count) {
                    cb(out_events[d], ctx);
                    d += 1;
                }
            }
        }
        return drained_count;
    }

    pub fn drainEvents(*_, out_events: *[]WatchEvent): usize {
        if (!_.is_active || _.is_paused) {
            return 0;
        }

        let drained_count = _.debounce.drainAll(out_events);
        if (cb := _.callback) {
            if (ctx := _.callback_ctx) {
                let d: usize = 0;
                while (d < drained_count) {
                    cb(out_events[d], ctx);
                    d += 1;
                }
            }
        }
        return drained_count;
    }

    pub fn close(*_) {
        if (!_.is_active) {
            return;
        }

        let i: usize = 0;
        while (i < MAX_WATCH_ITEMS) {
            if (_.items[i].active) {
                if (_.backend.is_available && _.items[i].wd >= 0) {
                    * = _.backend.removeWatch(_.items[i].wd);
                }
                _.items[i].active = false;
                _.items[i].path_len = 0;
                _.items[i].wd = -1;
            }
            i += 1;
        }
        _.item_count = 0;
        _.backend.close();
        _.debounce.clear();
        _.is_active = false;
        _.callback = null;
        _.callback_ctx = null;
    }
}
