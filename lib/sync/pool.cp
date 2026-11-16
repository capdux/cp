// ============================================================================
// Absorbed from Bun: bun/src/threading/pool.cp
// Upstream Sync: Refer to bun/src/threading/pool.zig for future upstream updates.
// ============================================================================
// lib/sync/pool.cp
// WorkPool and Guarded<T> synchronization wrappers
// Absorbed from Bun src/threading/work_pool.cp and guarded.cp with pure CP semantics

import { Mutex } from "./mutex";
import { Condition } from "./condition";

// Thread-safe container wrapping a value with an internal Mutex
pub struct Guarded<T> {
    value: T = undefined,
    mutex: Mutex = .{},

    pub static fn init(val: T): Guarded<T> {
        return Guarded<T> {
            value: val,
            mutex: .{},
        };
    }

    pub fn lock(*_): *T! {
        _.mutex.lock();
        return &_.value;
    }

    pub fn unlock(*_) {
        _.mutex.unlock();
    }

    pub fn get(*_): T {
        _.mutex.lock();
        defer _.mutex.unlock();
        return _.value;
    }

    pub fn set(*_, val: T) {
        _.mutex.lock();
        defer _.mutex.unlock();
        _.value = val;
    }
}

// Lightweight task descriptor for worker execution
pub struct TaskDesc {
    func: *const fn (*!) void = undefined,
    ctx: *! = undefined,
    next: (*TaskDesc)? = null,
}

// WorkPool: Global thread pool task dispatch and synchronization manager
pub struct WorkPool {
    mutex: Mutex = .{},
    cond: Condition = .{},
    head: (*TaskDesc)? = null,
    tail: (*TaskDesc)? = null,
    active_workers: usize = 0,
    queued_tasks: usize = 0,
    running: bool = true,

    pub static fn init(): WorkPool {
        return WorkPool {
            mutex: .{},
            cond: .{},
            head: null,
            tail: null,
            active_workers: 0,
            queued_tasks: 0,
            running: true,
        };
    }

    pub fn queueCount(*_): usize {
        _.mutex.lock();
        defer _.mutex.unlock();
        return _.queued_tasks;
    }

    pub fn isRunning(*_): bool {
        _.mutex.lock();
        defer _.mutex.unlock();
        return _.running;
    }

    pub fn schedule(*_, task: *TaskDesc) {
        _.mutex.lock();
        defer _.mutex.unlock();
        if (!_.running) {
            return;
        }
        task.next = null;
        if (t := _.tail) {
            t.next = task;
        } else {
            _.head = task;
        }
        _.tail = task;
        _.queued_tasks += 1;
        _.cond.signal();
    }

    pub fn pollTask(*_): (*TaskDesc)? {
        _.mutex.lock();
        defer _.mutex.unlock();
        if (h := _.head) {
            _.head = h.next;
            if (_.head == null) {
                _.tail = null;
            }
            _.queued_tasks -= 1;
            return h;
        }
        return null;
    }

    pub fn shutdown(*_) {
        _.mutex.lock();
        _.running = false;
        _.mutex.unlock();
        _.cond.broadcast();
    }

    pub fn release(*_) {
        _.shutdown();
        _.cond.release();
    }
}
