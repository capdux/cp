// Source: /data/dev/bun/src/event_loop/ConcurrentTask.cp
// lib/event/concurrent_task.cp
// Cross-thread task representation and spin-locked intrusive queue for cap:event

import std from "std";

pub type TaskFn = fn ((*anyopaque)?) void;

pub struct SpinLock {
    state: std.atomic.Value(u32) = std.atomic.Value(u32).init(0),

    pub static fn init(): SpinLock {
        return SpinLock{
            state: std.atomic.Value(u32).init(0),
        };
    }

    pub fn lock(*_) {
        while (_.state.swap(1, .acquire) != 0) {
            std.atomic.spinLoopHint();
        }
    }

    pub fn unlock(*_) {
        _.state.store(0, .release);
    }
}

pub type TaskSpinLock = SpinLock;

pub struct ConcurrentTask {
    pub func: fn ((*anyopaque)?) void = defaultConcurrentFunc,
    pub ctx: (*anyopaque)? = null,
    pub next: (*ConcurrentTask)? = null,
    pub auto_delete: bool = false,

    pub static fn defaultConcurrentFunc(c: (*anyopaque)?) {}

    pub static fn init(func: fn ((*anyopaque)?) void, ctx: (*anyopaque)?, auto_delete: bool): ConcurrentTask {
        return ConcurrentTask{
            func: func,
            ctx: ctx,
            next: null,
            auto_delete: auto_delete,
        };
    }

    pub static fn create(func: fn ((*anyopaque)?) void, ctx: (*anyopaque)?, auto_delete: bool): *ConcurrentTask! {
        let ptr: *ConcurrentTask! = unsafe { @ptrFromInt(8) };
        zig {
            const mem = std.heap.page_allocator.create(ConcurrentTask) catch unreachable;
            mem.* = ConcurrentTask{
                .func = func,
                .ctx = ctx,
                .next = null,
                .auto_delete = auto_delete,
            };
            ptr = mem;
        }
        return ptr;
    }

    pub fn run(*_) {
        let cb = _.func;
        let c = _.ctx;
        let auto_del = _.auto_delete;
        cb(c);
        if (auto_del) {
            zig {
                std.heap.page_allocator.destroy(self);
            }
        }
    }
}

pub struct ConcurrentTaskQueue {
    pub head: (*ConcurrentTask)? = null,
    pub tail: (*ConcurrentTask)? = null,
    pub item_count: usize = 0,
    pub lock: SpinLock = SpinLock{},

    pub static fn init(): ConcurrentTaskQueue {
        return ConcurrentTaskQueue{
            head: null,
            tail: null,
            item_count: 0,
            lock: SpinLock.init(),
        };
    }

    pub fn push(*_, task: *ConcurrentTask) {
        _.lock.lock();
        defer _.lock.unlock();

        task.next = null;
        if (t := _.tail) {
            unsafe {
                t.next = @ptrCast(task);
                _.tail = @ptrCast(task);
            }
        } else {
            unsafe {
                _.head = @ptrCast(task);
                _.tail = @ptrCast(task);
            }
        }
        _.item_count += 1;
    }

    pub fn pop(*_): (*ConcurrentTask)? {
        _.lock.lock();
        defer _.lock.unlock();

        let h = _.head orelse return null;
        _.head = unsafe { @ptrCast(h.next) };
        if (_.head == null) {
            _.tail = null;
        }
        h.next = null;
        _.item_count -= 1;
        return h;
    }

    pub fn drain(*_): (*ConcurrentTask)? {
        _.lock.lock();
        defer _.lock.unlock();

        let h = _.head;
        _.head = null;
        _.tail = null;
        _.item_count = 0;
        return h;
    }

    pub fn drainAll(*_): usize {
        let list = _.drain();
        let executed: usize = 0;
        let curr = list;
        while (curr != null) {
            let task = curr.?;
            curr = unsafe { @ptrCast(task.next) };
            task.next = null;
            task.run();
            executed += 1;
        }
        return executed;
    }

    pub fn drainMax(*_, max_count: usize): usize {
        let executed: usize = 0;
        while (executed < max_count) {
            let task = _.pop() orelse break;
            task.run();
            executed += 1;
        }
        return executed;
    }

    pub fn count(): usize {
        return _.item_count;
    }

    pub fn len(): usize {
        return _.item_count;
    }

    pub fn isEmpty(): bool {
        return _.head == null;
    }
}

pub type ConcurrentQueue = ConcurrentTaskQueue;
