// lib/event_loop/concurrent_task.cp
// 16-byte lock-free cross-thread task representation and thread-safe queue

import { Mutex } from "../sync/mutex.cp";

pub type TaskFn = fn(ctx: *!?);

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
    lock: Mutex = .{};
    count: usize = 0;

    pub static fn init(): *ConcurrentQueue {
        let q = std.heap.page_allocator.create(ConcurrentQueue) catch unreachable;
        q.head = null;
        q.tail = null;
        q.lock = .{};
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

        let h = _.head orelse return null;
        _.head = h.next;
        if (_.head == null) {
            _.tail = null;
        }
        _.count -= 1;
        h.next = null;
        return h;
    }

    pub fn len(*_): usize {
        _.lock.lock();
        defer _.lock.unlock();
        return _.count;
    }

    pub fn drain(*_, limit: usize): usize {
        let processed: usize = 0;
        while (processed < limit) {
            const task = _.pop() orelse break;
            task.run();
            processed += 1;
        }
        return processed;
    }
}
