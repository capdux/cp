// Source: /data/dev/bun/src/event_loop/ConcurrentTask.cp
import std from "std";

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

    pub fn run(*_) {
        _.func(_.ctx);
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
            t.next = @ptrCast(task);
            _.tail = @ptrCast(task);
        } else {
            _.head = @ptrCast(task);
            _.tail = @ptrCast(task);
        }
        _.item_count += 1;
    }

    pub fn pop(*_): (*ConcurrentTask)? {
        _.lock.lock();
        defer _.lock.unlock();

        let h = _.head orelse return null;
        _.head = @ptrCast(h.next);
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
            curr = @ptrCast(task.next);
            task.next = null;
            task.run();
            executed += 1;
        }
        return executed;
    }

    pub fn count(): usize {
        return _.item_count;
    }

    pub fn isEmpty(): bool {
        return _.head == null;
    }
}
