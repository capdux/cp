// Source: Bun bun/src/event_loop/concurrent_task.cp
// ============================================================================
// Absorbed from Bun: bun/src/event_loop/concurrent_task.cp
// Upstream Sync: Refer to bun/src/event_loop/concurrent_task.zig for future upstream updates.
// ============================================================================
// lib/event/concurrent_task.cp
// 16-byte lock-free cross-thread task representation and thread-safe queue

const std = @import("std");

pub type TaskFn = fn(ctx: *!?);

pub struct TaskSpinLock {
    state: std.atomic.Value(u32) = std.atomic.Value(u32).init(0);

    pub fn lock(*_) {
        while (_.state.cmpxchgWeak(0, 1, .acquire, .monotonic) != null) {
            std.atomic.spinLoopHint();
        }
    }

    pub fn unlock(*_) {
        _.state.store(0, .release);
    }
}

pub class ConcurrentTask {
    pub callback: TaskFn = undefined;
    pub ctx: *!? = null;
    pub auto_delete: bool = true;
    pub next: (*ConcurrentTask)? = null;

    pub static fn create(cb: TaskFn, ctx: *!?, auto_delete: bool): *ConcurrentTask {
        let t = std.heap.page_allocator.create(ConcurrentTask) catch unreachable;
        t.callback = cb;
        t.ctx = ctx;
        t.auto_delete = auto_delete;
        t.next = null;
        return t;
    }

    pub fn run(*_) {
        const cb = _.callback;
        const c = _.ctx;
        const auto_del = _.auto_delete;
        cb(c);
        if (auto_del) {
            std.heap.page_allocator.destroy(_);
        }
    }
}

pub class ConcurrentQueue {
    head: (*ConcurrentTask)? = null;
    tail: (*ConcurrentTask)? = null;
    lock: TaskSpinLock = TaskSpinLock {};
    count: usize = 0;

    pub static fn init(): *ConcurrentQueue {
        let q = std.heap.page_allocator.create(ConcurrentQueue) catch unreachable;
        q.head = null;
        q.tail = null;
        q.lock = TaskSpinLock {};
        q.count = 0;
        return q;
    }

    pub fn push(*_, task: *ConcurrentTask) {
        _.lock.lock();
        defer _.lock.unlock();

        task.next = null;
        if (t := _.tail) {
            t.next = task;
            _.tail = task;
        } else {
            _.head = task;
            _.tail = task;
        }
        _.count += 1;
    }

    pub fn pop(*_): (*ConcurrentTask)? {
        _.lock.lock();
        defer _.lock.unlock();

        if (h := _.head) {
            _.head = h.next;
            if (_.head == null) {
                _.tail = null;
            }
            _.count -= 1;
            h.next = null;
            return h;
        }
        return null;
    }

    pub fn len(*_): usize {
        _.lock.lock();
        defer _.lock.unlock();
        return _.count;
    }

    pub fn drain(*_, limit: usize): usize {
        let processed: usize = 0;
        while (processed < limit) {
            if (task := _.pop()) {
                task.run();
                processed += 1;
            } else {
                break;
            }
        }
        return processed;
    }
}
