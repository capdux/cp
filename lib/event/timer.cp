// ============================================================================
// Absorbed from Bun: bun/src/event_loop/timer.cp
// Upstream Sync: Refer to bun/src/event_loop/timer.zig for future upstream updates.
// ============================================================================
// lib/event_loop/timer.cp
// High-performance intrusive binary min-heap timer system


pub type TimerCallback = fn(ctx: *!?);

pub class Timer {
    pub id: u64 = 0;
    pub fire_at_ms: i64 = 0;
    pub interval_ms: i64 = 0;
    pub callback: TimerCallback = undefined;
    pub ctx: *!? = null;
    pub heap_idx: usize = 0;
    pub active: bool = false;
    pub repeat: bool = false;

    pub static fn create(id: u64, delay_ms: i64, interval_ms: i64, cb: TimerCallback, ctx: *!?): *Timer {
        let ptr = std.heap.page_allocator.create(Timer) catch unreachable;
        const now = TimerHeap.nowMs();
        ptr.id = id;
        ptr.fire_at_ms = now + delay_ms;
        ptr.interval_ms = interval_ms;
        ptr.callback = cb;
        ptr.ctx = ctx;
        ptr.heap_idx = 0;
        ptr.active = true;
        ptr.repeat = interval_ms > 0;
        return ptr;
    }

    pub fn destroy(*_) {
        _.active = false;
        std.heap.page_allocator.destroy(_);
    }
}

pub class TimerHeap {
    items: []*Timer = [];
    capacity: usize = 0;
    len: usize = 0;

    pub static fn nowMs(): i64 {
        __zig__ {
            var ts: std.os.linux.timespec = undefined;
            _ = std.os.linux.clock_gettime(std.os.linux.CLOCK.MONOTONIC, &ts);
            const sec: i64 = @as(i64, @intCast(ts.sec));
            const nsec: i64 = @as(i64, @intCast(ts.nsec));
            return sec * 1000 + @divTrunc(nsec, 1000000);
        }
    }

    pub static fn sleepMs(ms: u64) {
        __zig__ {
            const sec: i64 = @as(i64, @intCast(ms / 1000));
            const nsec: i64 = @as(i64, @intCast((ms % 1000) * 1000000));
            const ts = std.os.linux.timespec{ .sec = sec, .nsec = nsec };
            _ = std.os.linux.nanosleep(&ts, null);
        }
    }

    pub static fn init(): *TimerHeap {
        let heap = std.heap.page_allocator.create(TimerHeap) catch unreachable;
        heap.capacity = 64;
        heap.len = 0;
        heap.items = (std.heap.page_allocator.alloc(*Timer, 64) catch unreachable);
        return heap;
    }

    pub fn count(*_): usize {
        return _.len;
    }

    pub fn isEmpty(*_): bool {
        return _.len == 0;
    }

    pub fn peek(*_): (*Timer)? {
        if (_.len == 0) return null;
        return _.items[0];
    }

    pub fn push(*_, timer: *Timer) {
        if (_.len >= _.capacity) {
            const new_cap = _.capacity * 2;
            let new_items = std.heap.page_allocator.alloc(*Timer, new_cap) catch unreachable;
            for (i of 0.._.len) {
                new_items[i] = _.items[i];
            }
            std.heap.page_allocator.free(_.items);
            _.items = new_items;
            _.capacity = new_cap;
        }

        const idx = _.len;
        _.len += 1;
        timer.heap_idx = idx;
        timer.active = true;
        _.items[idx] = timer;
        _.siftUp(idx);
    }

    pub fn pop(*_): (*Timer)? {
        if (_.len == 0) return null;
        let root = _.items[0];
        root.active = false;
        _.len -= 1;
        if (_.len > 0) {
            _.items[0] = _.items[_.len];
            _.items[0].heap_idx = 0;
            _.siftDown(0);
        }
        return root;
    }

    pub fn remove(*_, timer: *Timer): bool {
        if (!timer.active or _.len == 0) return false;
        const idx = timer.heap_idx;
        if (idx >= _.len or _.items[idx] != timer) return false;

        timer.active = false;
        _.len -= 1;
        if (idx == _.len) {
            return true;
        }

        _.items[idx] = _.items[_.len];
        _.items[idx].heap_idx = idx;
        _.siftUp(idx);
        _.siftDown(idx);
        return true;
    }

    pub fn getTimeoutMs(*_, now_ms: i64): i32? {
        if (_.len == 0) return null;
        const top = _.items[0];
        const diff = top.fire_at_ms - now_ms;
        if (diff <= 0) return 0;
        if (diff > 2147483647) return 2147483647;
        return @as(i32, @intCast(diff));
    }

    pub fn drainExpired(*_, now_ms: i64, max_drain: usize): usize {
        let fired: usize = 0;
        while (fired < max_drain and _.len > 0) {
            const top = _.items[0];
            if (top.fire_at_ms > now_ms) break;

            let timer = _.pop().?;
            timer.callback(timer.ctx);
            fired += 1;

            if (timer.repeat and timer.active) {
                timer.fire_at_ms = now_ms + timer.interval_ms;
                _.push(timer);
            }
        }
        return fired;
    }

    fn siftUp(*_, start_idx: usize) {
        let idx = start_idx;
        while (idx > 0) {
            const parent = (idx - 1) / 2;
            if (_.items[idx].fire_at_ms < _.items[parent].fire_at_ms) {
                const tmp = _.items[idx];
                _.items[idx] = _.items[parent];
                _.items[parent] = tmp;
                _.items[idx].heap_idx = idx;
                _.items[parent].heap_idx = parent;
                idx = parent;
            } else {
                break;
            }
        }
    }

    fn siftDown(*_, start_idx: usize) {
        let idx = start_idx;
        while (true) {
            let smallest = idx;
            const left = idx * 2 + 1;
            const right = idx * 2 + 2;

            if (left < _.len and _.items[left].fire_at_ms < _.items[smallest].fire_at_ms) {
                smallest = left;
            }
            if (right < _.len and _.items[right].fire_at_ms < _.items[smallest].fire_at_ms) {
                smallest = right;
            }

            if (smallest != idx) {
                const tmp = _.items[idx];
                _.items[idx] = _.items[smallest];
                _.items[smallest] = tmp;
                _.items[idx].heap_idx = idx;
                _.items[smallest].heap_idx = smallest;
                idx = smallest;
            } else {
                break;
            }
        }
    }
}
