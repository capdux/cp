// @cap/tapable — Zero-allocation plugin hook engine
// Clean-room MIT implementation modeled on npm `tapable` v2 (Webpack / Rspack).

pub const TAP_MAX_TAPS: usize = 32;

pub struct TapEntry {
    name: string,
    stage: i32,
    handler_id: u32,
    enabled: bool,
}

pub type TapSyncFn = *const fn (u32, i64) i64;
pub type TapBailFn = *const fn (u32, i64) i64?;
pub type TapWaterfallFn = *const fn (u32, i64) i64;
pub type TapLoopFn = *const fn (u32, i64) bool;

fn tapEmptyEntry(): TapEntry {
    return TapEntry{
        name: "",
        stage: 0,
        handler_id: 0,
        enabled: false,
    };
}

fn tapStrEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

/// Insert entry keeping `entries[0..count]` stably sorted by `stage` ascending.
fn tapInsertSorted(entries: []TapEntry, count: usize, entry: TapEntry): usize {
    if (count >= entries.len) return count;
    let insert_idx = count;
    while (insert_idx > 0 && entries[insert_idx - 1].stage > entry.stage) {
        entries[insert_idx] = entries[insert_idx - 1];
        insert_idx -= 1;
    }
    entries[insert_idx] = entry;
    return count + 1;
}

/// Insert entry immediately before the first tap matching `before_name` (or by stage if not found).
fn tapInsertBefore(entries: []TapEntry, count: usize, name: string, before_name: string, handler_id: u32): usize {
    if (count >= entries.len) return count;
    let target_idx: usize = count;
    let i: usize = 0;
    while (i < count) : (i += 1) {
        if (tapStrEq(entries[i].name, before_name)) {
            target_idx = i;
            break;
        }
    }
    if (target_idx == count) {
        return tapInsertSorted(entries, count, TapEntry{
            name: name,
            stage: 0,
            handler_id: handler_id,
            enabled: true,
        });
    }
    const target_stage = entries[target_idx].stage;
    let shift = count;
    while (shift > target_idx) {
        entries[shift] = entries[shift - 1];
        shift -= 1;
    }
    entries[target_idx] = TapEntry{
        name: name,
        stage: target_stage,
        handler_id: handler_id,
        enabled: true,
    };
    return count + 1;
}

fn tapSetEnabled(entries: []TapEntry, count: usize, name: string, enabled: bool): bool {
    let found = false;
    let i: usize = 0;
    while (i < count) : (i += 1) {
        if (tapStrEq(entries[i].name, name)) {
            let e = entries[i];
            e.enabled = enabled;
            entries[i] = e;
            found = true;
        }
    }
    return found;
}

fn tapCollectOrdered(entries: []const TapEntry, count: usize, out: []u32): usize {
    let written: usize = 0;
    let i: usize = 0;
    while (i < count && written < out.len) : (i += 1) {
        if (entries[i].enabled) {
            out[written] = entries[i].handler_id;
            written += 1;
        }
    }
    return written;
}

/// SyncHook — invokes every enabled tap in stage/before order.
pub struct SyncHook {
    entries: [32]TapEntry,
    count: usize,

    static fn init(): SyncHook {
        let h = SyncHook{ entries: undefined, count: 0 };
        let i: usize = 0;
        while (i < TAP_MAX_TAPS) : (i += 1) {
            h.entries[i] = tapEmptyEntry();
        }
        return h;
    }

    fn tap(*_, name: string, handler_id: u32): bool {
        return _.tapStage(name, 0, handler_id);
    }

    fn tapStage(*_, name: string, stage: i32, handler_id: u32): bool {
        if (_.count >= TAP_MAX_TAPS) return false;
        const entry = TapEntry{
            name: name,
            stage: stage,
            handler_id: handler_id,
            enabled: true,
        };
        _.count = tapInsertSorted(_.entries[0..], _.count, entry);
        return true;
    }

    fn tapBefore(*_, name: string, before_name: string, handler_id: u32): bool {
        if (_.count >= TAP_MAX_TAPS) return false;
        _.count = tapInsertBefore(_.entries[0..], _.count, name, before_name, handler_id);
        return true;
    }

    fn setEnabled(*_, name: string, enabled: bool): bool {
        return tapSetEnabled(_.entries[0..], _.count, name, enabled);
    }

    fn isUsed(): bool {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.entries[i].enabled) return true;
        }
        return false;
    }

    fn tapCount(): usize {
        return _.count;
    }

    fn getOrderedHandlers(out: []u32): usize {
        const local: [32]TapEntry = _.entries;
        return tapCollectOrdered(local[0.._.count], _.count, out);
    }

    fn call(cb: TapSyncFn, arg: i64): usize {
        let invoked: usize = 0;
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.entries[i].enabled) {
                * = cb(_.entries[i].handler_id, arg);
                invoked += 1;
            }
        }
        return invoked;
    }
}

/// SyncBailHook — short-circuits and returns the first non-null callback result.
pub struct SyncBailHook {
    entries: [32]TapEntry,
    count: usize,

    static fn init(): SyncBailHook {
        let h = SyncBailHook{ entries: undefined, count: 0 };
        let i: usize = 0;
        while (i < TAP_MAX_TAPS) : (i += 1) {
            h.entries[i] = tapEmptyEntry();
        }
        return h;
    }

    fn tap(*_, name: string, handler_id: u32): bool {
        return _.tapStage(name, 0, handler_id);
    }

    fn tapStage(*_, name: string, stage: i32, handler_id: u32): bool {
        if (_.count >= TAP_MAX_TAPS) return false;
        const entry = TapEntry{
            name: name,
            stage: stage,
            handler_id: handler_id,
            enabled: true,
        };
        _.count = tapInsertSorted(_.entries[0..], _.count, entry);
        return true;
    }

    fn tapBefore(*_, name: string, before_name: string, handler_id: u32): bool {
        if (_.count >= TAP_MAX_TAPS) return false;
        _.count = tapInsertBefore(_.entries[0..], _.count, name, before_name, handler_id);
        return true;
    }

    fn setEnabled(*_, name: string, enabled: bool): bool {
        return tapSetEnabled(_.entries[0..], _.count, name, enabled);
    }

    fn isUsed(): bool {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.entries[i].enabled) return true;
        }
        return false;
    }

    fn tapCount(): usize {
        return _.count;
    }

    fn getOrderedHandlers(out: []u32): usize {
        const local: [32]TapEntry = _.entries;
        return tapCollectOrdered(local[0.._.count], _.count, out);
    }

    fn call(cb: TapBailFn, arg: i64): i64? {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.entries[i].enabled) {
                if (res := cb(_.entries[i].handler_id, arg)) {
                    return res;
                }
            }
        }
        return null;
    }
}

/// SyncWaterfallHook — threads each callback's return value as the next callback's input.
pub struct SyncWaterfallHook {
    entries: [32]TapEntry,
    count: usize,

    static fn init(): SyncWaterfallHook {
        let h = SyncWaterfallHook{ entries: undefined, count: 0 };
        let i: usize = 0;
        while (i < TAP_MAX_TAPS) : (i += 1) {
            h.entries[i] = tapEmptyEntry();
        }
        return h;
    }

    fn tap(*_, name: string, handler_id: u32): bool {
        return _.tapStage(name, 0, handler_id);
    }

    fn tapStage(*_, name: string, stage: i32, handler_id: u32): bool {
        if (_.count >= TAP_MAX_TAPS) return false;
        const entry = TapEntry{
            name: name,
            stage: stage,
            handler_id: handler_id,
            enabled: true,
        };
        _.count = tapInsertSorted(_.entries[0..], _.count, entry);
        return true;
    }

    fn tapBefore(*_, name: string, before_name: string, handler_id: u32): bool {
        if (_.count >= TAP_MAX_TAPS) return false;
        _.count = tapInsertBefore(_.entries[0..], _.count, name, before_name, handler_id);
        return true;
    }

    fn setEnabled(*_, name: string, enabled: bool): bool {
        return tapSetEnabled(_.entries[0..], _.count, name, enabled);
    }

    fn isUsed(): bool {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.entries[i].enabled) return true;
        }
        return false;
    }

    fn tapCount(): usize {
        return _.count;
    }

    fn getOrderedHandlers(out: []u32): usize {
        const local: [32]TapEntry = _.entries;
        return tapCollectOrdered(local[0.._.count], _.count, out);
    }

    fn call(cb: TapWaterfallFn, initial: i64): i64 {
        let val = initial;
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.entries[i].enabled) {
                val = cb(_.entries[i].handler_id, val);
            }
        }
        return val;
    }
}

/// SyncLoopHook — restarts from the first tap whenever any callback returns `true`,
/// stopping once a full pass returns `false` or `max_rounds` is exceeded.
pub struct SyncLoopHook {
    entries: [32]TapEntry,
    count: usize,

    static fn init(): SyncLoopHook {
        let h = SyncLoopHook{ entries: undefined, count: 0 };
        let i: usize = 0;
        while (i < TAP_MAX_TAPS) : (i += 1) {
            h.entries[i] = tapEmptyEntry();
        }
        return h;
    }

    fn tap(*_, name: string, handler_id: u32): bool {
        return _.tapStage(name, 0, handler_id);
    }

    fn tapStage(*_, name: string, stage: i32, handler_id: u32): bool {
        if (_.count >= TAP_MAX_TAPS) return false;
        const entry = TapEntry{
            name: name,
            stage: stage,
            handler_id: handler_id,
            enabled: true,
        };
        _.count = tapInsertSorted(_.entries[0..], _.count, entry);
        return true;
    }

    fn tapBefore(*_, name: string, before_name: string, handler_id: u32): bool {
        if (_.count >= TAP_MAX_TAPS) return false;
        _.count = tapInsertBefore(_.entries[0..], _.count, name, before_name, handler_id);
        return true;
    }

    fn setEnabled(*_, name: string, enabled: bool): bool {
        return tapSetEnabled(_.entries[0..], _.count, name, enabled);
    }

    fn isUsed(): bool {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.entries[i].enabled) return true;
        }
        return false;
    }

    fn tapCount(): usize {
        return _.count;
    }

    fn getOrderedHandlers(out: []u32): usize {
        const local: [32]TapEntry = _.entries;
        return tapCollectOrdered(local[0.._.count], _.count, out);
    }

    fn call(cb: TapLoopFn, arg: i64, max_rounds: usize): bool {
        let rounds: usize = 0;
        while (rounds < max_rounds) {
            rounds += 1;
            let restarted = false;
            let i: usize = 0;
            while (i < _.count) : (i += 1) {
                if (_.entries[i].enabled) {
                    if (cb(_.entries[i].handler_id, arg)) {
                        restarted = true;
                        break;
                    }
                }
            }
            if (!restarted) return true;
        }
        return false;
    }
}
