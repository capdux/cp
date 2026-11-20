// Source: /data/dev/bun/src/event_loop/EventLoopTimer.cp
// lib/event/timer.cp
// Intrusive pairing min-heap timer scheduler for cap:event (unified event loop)

import std from "std";

pub type TimerCallback = fn (*Timer) void;

pub fn getNowMs(): i64 {
    let ts: std.os.linux.timespec = .{ .sec = 0, .nsec = 0 };
    __zig__ {
        _ = std.os.linux.clock_gettime(.MONOTONIC, &ts);
    }
    return (ts.sec * 1000) + @divTrunc(ts.nsec, 1_000_000);
}

pub struct TimeSpec {
    pub sec: i64 = 0,
    pub nsec: i64 = 0,

    pub static fn now(): TimeSpec {
        let ms = getNowMs();
        let sec = @divTrunc(ms, 1000);
        let rem_ms = @mod(ms, 1000);
        return TimeSpec{
            sec: sec,
            nsec: rem_ms * 1_000_000,
        };
    }

    pub static fn fromMillis(ms: i64): TimeSpec {
        let sec = @divTrunc(ms, 1000);
        let rem_ms = @mod(ms, 1000);
        return TimeSpec{
            sec: sec,
            nsec: rem_ms * 1_000_000,
        };
    }

    pub fn toMillis(): i64 {
        return _.sec * 1000 + @divTrunc(_.nsec, 1_000_000);
    }

    pub fn lessThan(other: TimeSpec): bool {
        if (_.sec != other.sec) {
            return _.sec < other.sec;
        }
        return _.nsec < other.nsec;
    }
}

pub enum TimerState {
    Pending,
    Active,
    Cancelled,
    Fired,
}

pub struct Timer {
    pub expires_at_ms: i64 = 0,
    pub interval_ms: u64 = 0,
    pub state: TimerState = TimerState.Pending,
    pub callback: fn (*Timer) void = defaultTimerCallback,
    pub ctx: (*anyopaque)? = null,

    // Intrusive pairing heap pointers
    pub child: (*Timer)? = null,
    pub prev: (*Timer)? = null,
    pub next: (*Timer)? = null,
    pub in_heap: bool = false,

    pub static fn defaultTimerCallback(t: *Timer) {}

    pub static fn init(delay_ms: u64, callback: fn (*Timer) void, ctx: (*anyopaque)?): Timer {
        let now_ms = getNowMs();
        return Timer{
            expires_at_ms: now_ms + i64(delay_ms),
            interval_ms: 0,
            state: TimerState.Pending,
            callback: callback,
            ctx: ctx,
            child: null,
            prev: null,
            next: null,
            in_heap: false,
        };
    }

    pub static fn initInterval(interval_ms: u64, callback: fn (*Timer) void, ctx: (*anyopaque)?): Timer {
        let now_ms = getNowMs();
        return Timer{
            expires_at_ms: now_ms + i64(interval_ms),
            interval_ms: interval_ms,
            state: TimerState.Pending,
            callback: callback,
            ctx: ctx,
            child: null,
            prev: null,
            next: null,
            in_heap: false,
        };
    }

    pub static fn initPaused(callback: fn (*Timer) void, ctx: (*anyopaque)?): Timer {
        return Timer{
            expires_at_ms: 0,
            interval_ms: 0,
            state: TimerState.Pending,
            callback: callback,
            ctx: ctx,
            child: null,
            prev: null,
            next: null,
            in_heap: false,
        };
    }

    pub fn isActive(): bool {
        return _.state == TimerState.Active;
    }

    pub fn isCancelled(): bool {
        return _.state == TimerState.Cancelled;
    }

    pub fn isFired(): bool {
        return _.state == TimerState.Fired;
    }

    pub fn isRepeating(): bool {
        return _.interval_ms > 0;
    }

    pub fn remainingMs(now_ms: i64): i64 {
        if (_.expires_at_ms <= now_ms) return 0;
        return _.expires_at_ms - now_ms;
    }

    pub fn getContext(): (*anyopaque)? {
        return _.ctx;
    }

    pub fn setContext(*_, new_ctx: (*anyopaque)?) {
        _.ctx = new_ctx;
    }

    pub fn fire(*_) {
        _.state = TimerState.Fired;
        _.callback(_);
    }
}

pub struct TimerHeap {
    pub root: (*Timer)? = null,
    pub item_count: usize = 0,

    pub static fn init(): TimerHeap {
        return TimerHeap{
            root: null,
            item_count: 0,
        };
    }

    pub static fn nowMs(): i64 {
        return getNowMs();
    }

    pub static fn sleepMs(ms: u64) {
        if (ms == 0) return;
        let req: std.os.linux.timespec = .{ .sec = @intCast(ms / 1000), .nsec = @intCast((ms % 1000) * 1_000_000) };
        let rem: std.os.linux.timespec = .{ .sec = 0, .nsec = 0 };
        __zig__ {
            _ = std.os.linux.nanosleep(&req, &rem);
        }
    }

    pub fn count(): usize {
        return _.item_count;
    }

    pub fn isEmpty(): bool {
        return _.root == null;
    }

    pub fn peek(): (*Timer)? {
        return _.root;
    }

    pub fn nextTimeoutMs(now_ms: i64): (i64)? {
        let r = _.root orelse return null;
        if (r.expires_at_ms <= now_ms) return 0;
        return r.expires_at_ms - now_ms;
    }

    pub fn getTimeoutMs(now_ms: i64): (i32)? {
        let diff = _.nextTimeoutMs(now_ms) orelse return null;
        if (diff > 2147483647) return 2147483647;
        return @intCast(diff);
    }

    pub fn insert(*_, timer: *Timer) {
        if (timer.in_heap) {
            * = _.remove(timer);
        }
        timer.child = null;
        timer.prev = null;
        timer.next = null;
        timer.state = TimerState.Active;
        timer.in_heap = true;

        if (r := _.root) {
            _.root = @ptrCast(_.meld(timer, r));
        } else {
            _.root = @ptrCast(timer);
        }
        _.item_count += 1;
    }

    pub fn push(*_, timer: *Timer) {
        _.insert(timer);
    }

    pub fn deleteMin(*_): (*Timer)? {
        let r = _.root orelse return null;
        if (child := r.child) {
            _.root = @ptrCast(_.combineSiblings(child));
            if (new_root := _.root) {
                new_root.prev = null;
            }
        } else {
            _.root = null;
        }
        r.child = null;
        r.prev = null;
        r.next = null;
        r.in_heap = false;
        _.item_count -= 1;
        return r;
    }

    pub fn remove(*_, timer: *Timer): bool {
        if (!timer.in_heap) return false;
        timer.in_heap = false;
        timer.state = TimerState.Cancelled;

        if (timer.prev == null) {
            if (_.root == timer) {
                * = _.deleteMin();
                return true;
            }
            return false;
        }

        let p_node = timer.prev.?;
        if (p_node.child == timer) {
            p_node.child = @ptrCast(timer.next);
        } else {
            p_node.next = @ptrCast(timer.next);
        }

        if (n := timer.next) {
            n.prev = @ptrCast(timer.prev);
        }

        let child = timer.child;
        timer.child = null;
        timer.prev = null;
        timer.next = null;

        if (c := child) {
            let combined = _.combineSiblings(c);
            if (cmb := combined) {
                cmb.prev = null;
                if (r := _.root) {
                    _.root = @ptrCast(_.meld(cmb, r));
                } else {
                    _.root = @ptrCast(cmb);
                }
            }
        }

        _.item_count -= 1;
        return true;
    }

    pub fn fireExpired(*_, now_ms: i64): usize {
        let fired_count: usize = 0;
        while (r := _.root) {
            if (r.expires_at_ms > now_ms) break;
            let timer = _.deleteMin().?;
            timer.fire();
            fired_count += 1;

            if (timer.isRepeating() and timer.state != TimerState.Cancelled) {
                timer.expires_at_ms = now_ms + i64(timer.interval_ms);
                _.insert(timer);
            }
        }
        return fired_count;
    }

    pub fn clear(*_) {
        while (_.root != null) {
            * = _.deleteMin();
        }
    }

    fn meld(*_, a: *Timer, b: *Timer): *Timer {
        if (a.expires_at_ms < b.expires_at_ms) {
            b.prev = @ptrCast(a);
            b.next = @ptrCast(a.child);
            if (c := a.child) {
                c.prev = @ptrCast(b);
            }
            a.child = @ptrCast(b);
            return a;
        } else {
            a.prev = @ptrCast(b);
            a.next = @ptrCast(b.child);
            if (c := b.child) {
                c.prev = @ptrCast(a);
            }
            b.child = @ptrCast(a);
            return b;
        }
    }

    fn combineSiblings(*_, left: *Timer): (*Timer)? {
        left.prev = null;
        let curr: (*Timer)? = left;
        let head: (*Timer)? = null;
        let tail: (*Timer)? = null;

        while (a := curr) {
            let next_node = a.next;
            a.next = null;
            a.prev = null;

            if (b := next_node) {
                let rest = b.next;
                b.next = null;
                b.prev = null;
                let melded = _.meld(a, b);
                melded.next = null;
                melded.prev = null;

                if (t1 := tail) {
                    t1.next = @ptrCast(melded);
                    melded.prev = @ptrCast(t1);
                    tail = @ptrCast(melded);
                } else {
                    head = @ptrCast(melded);
                    tail = @ptrCast(melded);
                }
                curr = @ptrCast(rest);
            } else {
                if (t2 := tail) {
                    t2.next = @ptrCast(a);
                    a.prev = @ptrCast(t2);
                    tail = @ptrCast(a);
                } else {
                    head = @ptrCast(a);
                    tail = @ptrCast(a);
                }
                break;
            }
        }

        let root_node = tail orelse return null;
        let curr_p = root_node.prev;
        root_node.prev = null;
        root_node.next = null;

        while (curr_p != null) {
            let prev_node = curr_p.?;
            curr_p = @ptrCast(prev_node.prev);
            prev_node.prev = null;
            prev_node.next = null;
            root_node = _.meld(prev_node, root_node);
        }

        return root_node;
    }
}
