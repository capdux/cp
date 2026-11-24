// ============================================================================
// cap:progress — Thread-Safe Flicker-Free Hierarchical Progress Bar & Nodes
// ============================================================================

const std = @import("std");

pub const MAX_NAME_LEN: usize = 120;
pub const MAX_NODES: usize = 64;

const SYNC_START: []const u8 = "\x1b[?2026h";
const SYNC_END: []const u8 = "\x1b[?2026l";

pub struct ProgressOptions {
    root_name: []const u8 = "",
    estimated_total: usize = 0,
    disable_printing: bool = true,
}

struct NodeSlot {
    active: std.atomic.Value(bool),
    parent_index: i32,
    completed: std.atomic.Value(u32),
    estimated_total: std.atomic.Value(u32),
    name_len: std.atomic.Value(u8),
    name_buf: [MAX_NAME_LEN]u8,

    fn reset(*_, parent_idx: i32, name: []const u8, total: usize) {
        _.parent_index = parent_idx;
        _.completed.store(0, .monotonic);
        const clamped_total: u32 = if (total > std.math.maxInt(u32)) std.math.maxInt(u32) else @as(u32, @intCast(total));
        _.estimated_total.store(clamped_total, .monotonic);
        const n: usize = if (name.len > MAX_NAME_LEN) MAX_NAME_LEN else name.len;
        if (n > 0) {
            @memcpy(_.name_buf[0 .. n], name[0 .. n]);
        }
        _.name_len.store(@as(u8, @intCast(n)), .release);
        _.active.store(true, .release);
    }

    fn setName(*_, name: []const u8) {
        const n: usize = if (name.len > MAX_NAME_LEN) MAX_NAME_LEN else name.len;
        if (n > 0) {
            @memcpy(_.name_buf[0 .. n], name[0 .. n]);
        }
        _.name_len.store(@as(u8, @intCast(n)), .release);
    }

    fn nameSlice(): []const u8 {
        const n: usize = @as(usize, _.name_len.load(.acquire));
        return _.name_buf[0 .. n];
    }
}

struct ProgressState {
    slots: [MAX_NODES]NodeSlot,
    disable_printing: bool,

    fn initEmpty(*_, disable_printing: bool) {
        _.disable_printing = disable_printing;
        let i: usize = 0;
        while (i < MAX_NODES) : (i += 1) {
            _.slots[i] = NodeSlot{
                active: std.atomic.Value(bool).init(false),
                parent_index: -1,
                completed: std.atomic.Value(u32).init(0),
                estimated_total: std.atomic.Value(u32).init(0),
                name_len: std.atomic.Value(u8).init(0),
                name_buf: undefined,
            };
        }
    }

    fn allocChild(*_, parent_idx: u32, name: []const u8, total: usize): u32? {
        let i: usize = 1;
        while (i < MAX_NODES) : (i += 1) {
            if (_.slots[i].active.cmpxchgStrong(false, true, .acq_rel, .monotonic) == null) {
                _.slots[i].reset(@as(i32, @intCast(parent_idx)), name, total);
                return @as(u32, @intCast(i));
            }
        }
        return null;
    }
}

fn appendBytes(dst: []u8, pos: *usize, src: []const u8) {
    if (pos.* >= dst.len) return;
    const avail = dst.len - pos.*;
    const n = if (src.len < avail) src.len else avail;
    if (n > 0) {
        @memcpy(dst[pos.* .. pos.* + n], src[0 .. n]);
        pos.* += n;
    }
}

fn appendU32(dst: []u8, pos: *usize, val: u32) {
    let tmp: [16]u8 = undefined;
    let v: u32 = val;
    let len: usize = 0;
    if (v == 0) {
        tmp[0] = '0';
        len = 1;
    } else {
        while (v > 0) {
            tmp[len] = @as(u8, @intCast('0' + (v % 10)));
            v /= 10;
            len += 1;
        }
        let lo: usize = 0;
        let hi: usize = len - 1;
        while (lo < hi) {
            const c = tmp[lo];
            tmp[lo] = tmp[hi];
            tmp[hi] = c;
            lo += 1;
            hi -= 1;
        }
    }
    appendBytes(dst, pos, tmp[0 .. len]);
}

/// Hierarchical child progress node with thread-safe atomic counters and `~init()` RAII.
pub struct ProgressNode {
    state: (*ProgressState)?,
    index: u32,
    ended: bool,

    pub static fn none(): ProgressNode {
        return ProgressNode{
            state: null,
            index: 0,
            ended: true,
        };
    }

    /// Spawns a nested child node under this node. Thread-safe.
    pub fn start(name: []const u8, estimated_total: usize): ProgressNode {
        if (_.ended) return ProgressNode.none();
        const st = _.state orelse return ProgressNode.none();
        const child_idx = st.allocChild(_.index, name, estimated_total) orelse return ProgressNode.none();
        return ProgressNode{
            state: st,
            index: child_idx,
            ended: false,
        };
    }

    /// Sets the absolute completed item count. Thread-safe.
    pub fn setCompleted(completed_items: usize) {
        if (_.ended) return;
        const st = _.state orelse return;
        const clamped: u32 = if (completed_items > std.math.maxInt(u32)) std.math.maxInt(u32) else @as(u32, @intCast(completed_items));
        st.slots[_.index].completed.store(clamped, .monotonic);
    }

    /// Atomically increments the completed item count by 1. Thread-safe.
    pub fn completeOne() {
        if (_.ended) return;
        const st = _.state orelse return;
        * = st.slots[_.index].completed.fetchAdd(1, .monotonic);
    }

    /// Sets the estimated total item count (0 means unknown). Thread-safe.
    pub fn setEstimatedTotal(total: usize) {
        if (_.ended) return;
        const st = _.state orelse return;
        const clamped: u32 = if (total > std.math.maxInt(u32)) std.math.maxInt(u32) else @as(u32, @intCast(total));
        st.slots[_.index].estimated_total.store(clamped, .monotonic);
    }

    /// Atomically adds `delta` to the estimated total item count. Thread-safe.
    pub fn increaseEstimatedTotal(delta: usize) {
        if (_.ended) return;
        const st = _.state orelse return;
        const clamped: u32 = if (delta > std.math.maxInt(u32)) std.math.maxInt(u32) else @as(u32, @intCast(delta));
        * = st.slots[_.index].estimated_total.fetchAdd(clamped, .monotonic);
    }

    /// Updates the display name of this node.
    pub fn setName(name: []const u8) {
        if (_.ended) return;
        const st = _.state orelse return;
        st.slots[_.index].setName(name);
    }

    pub fn completed(): u32 {
        const st = _.state orelse return 0;
        return st.slots[_.index].completed.load(.monotonic);
    }

    pub fn estimatedTotal(): u32 {
        const st = _.state orelse return 0;
        return st.slots[_.index].estimated_total.load(.monotonic);
    }

    pub fn percent(): u8 {
        const tot = _.estimatedTotal();
        if (tot == 0) return 0;
        const comp = _.completed();
        if (comp >= tot) return 100;
        return @as(u8, @intCast((@as(u64, comp) * 100) / @as(u64, tot)));
    }

    /// Ends this node and increments its parent's completed count. Idempotent.
    pub fn end(*_) {
        if (_.ended) return;
        _.ended = true;
        const st = _.state orelse return;
        const parent_idx = st.slots[_.index].parent_index;
        if (parent_idx >= 0 && @as(usize, @intCast(parent_idx)) < MAX_NODES) {
            * = st.slots[@as(usize, @intCast(parent_idx))].completed.fetchAdd(1, .monotonic);
        }
        st.slots[_.index].active.store(false, .release);
        _.state = null;
    }

    ~init() {
        _.end();
    }
}

/// Root progress bar owning the shared atomic node table with idempotent `~init()` RAII.
pub struct ProgressBar {
    allocator: std.mem.Allocator,
    state: (*ProgressState)?,

    /// Starts a new hierarchical progress bar.
    pub static fn start(allocator: std.mem.Allocator, opts: ProgressOptions): ProgressBar! {
        const st = try allocator.create(ProgressState);
        st.initEmpty(opts.disable_printing);
        st.slots[0].reset(-1, opts.root_name, opts.estimated_total);
        return ProgressBar{
            allocator: allocator,
            state: st,
        };
    }

    /// Spawns a child `ProgressNode` attached to the root progress bar.
    pub fn startChild(name: []const u8, estimated_total: usize): ProgressNode {
        const st = _.state orelse return ProgressNode.none();
        const child_idx = st.allocChild(0, name, estimated_total) orelse return ProgressNode.none();
        return ProgressNode{
            state: st,
            index: child_idx,
            ended: false,
        };
    }

    pub fn setCompleted(completed_items: usize) {
        const st = _.state orelse return;
        const clamped: u32 = if (completed_items > std.math.maxInt(u32)) std.math.maxInt(u32) else @as(u32, @intCast(completed_items));
        st.slots[0].completed.store(clamped, .monotonic);
    }

    pub fn completeOne() {
        const st = _.state orelse return;
        * = st.slots[0].completed.fetchAdd(1, .monotonic);
    }

    pub fn setEstimatedTotal(total: usize) {
        const st = _.state orelse return;
        const clamped: u32 = if (total > std.math.maxInt(u32)) std.math.maxInt(u32) else @as(u32, @intCast(total));
        st.slots[0].estimated_total.store(clamped, .monotonic);
    }

    pub fn increaseEstimatedTotal(delta: usize) {
        const st = _.state orelse return;
        const clamped: u32 = if (delta > std.math.maxInt(u32)) std.math.maxInt(u32) else @as(u32, @intCast(delta));
        * = st.slots[0].estimated_total.fetchAdd(clamped, .monotonic);
    }

    pub fn setName(name: []const u8) {
        const st = _.state orelse return;
        st.slots[0].setName(name);
    }

    pub fn completed(): u32 {
        const st = _.state orelse return 0;
        return st.slots[0].completed.load(.monotonic);
    }

    pub fn estimatedTotal(): u32 {
        const st = _.state orelse return 0;
        return st.slots[0].estimated_total.load(.monotonic);
    }

    pub fn percent(): u8 {
        const tot = _.estimatedTotal();
        if (tot == 0) return 0;
        const comp = _.completed();
        if (comp >= tot) return 100;
        return @as(u8, @intCast((@as(u64, comp) * 100) / @as(u64, tot)));
    }

    pub fn activeCount(): usize {
        const st = _.state orelse return 0;
        let count: usize = 0;
        let i: usize = 0;
        while (i < MAX_NODES) : (i += 1) {
            if (st.slots[i].active.load(.acquire)) count += 1;
        }
        return count;
    }

    /// Renders a flicker-free ANSI synchronized snapshot of all active nodes into `buf`.
    pub fn render(buf: []u8): usize {
        const st = _.state orelse return 0;
        let pos: usize = 0;
        appendBytes(buf, &pos, SYNC_START);
        let i: usize = 0;
        while (i < MAX_NODES) : (i += 1) {
            if (st.slots[i].active.load(.acquire)) {
                if (i > 0) {
                    appendBytes(buf, &pos, "  |- ");
                }
                appendBytes(buf, &pos, st.slots[i].nameSlice());
                appendBytes(buf, &pos, " [");
                appendU32(buf, &pos, st.slots[i].completed.load(.monotonic));
                const tot = st.slots[i].estimated_total.load(.monotonic);
                if (tot > 0) {
                    appendBytes(buf, &pos, "/");
                    appendU32(buf, &pos, tot);
                }
                appendBytes(buf, &pos, "]\n");
            }
        }
        appendBytes(buf, &pos, SYNC_END);
        return pos;
    }

    /// Ends the root progress bar and releases its backing state. Idempotent.
    pub fn end(*_) {
        if (st := _.state) {
            st.slots[0].active.store(false, .release);
            _.allocator.destroy(st);
            _.state = null;
        }
    }

    ~init() {
        _.end();
    }
}

pub fn startProgress(allocator: std.mem.Allocator, opts: ProgressOptions): ProgressBar! {
    return ProgressBar.start(allocator, opts);
}
