// Source: /data/dev/bun/src/event_loop/DeferredTaskQueue.cp
// lib/event/deferred_task.cp
// Deferred microtask queue for batching I/O and non-reentrant event execution

const std = @import("std");

pub type DeferredFn = fn(ctx: *!?);

pub struct DeferredEntry {
    pub ctx: *!? = null;
    pub task: DeferredFn = undefined;
}

pub class DeferredTaskQueue {
    entries: []DeferredEntry = [];
    capacity: usize = 0;
    len: usize = 0;

    pub static fn init(): *DeferredTaskQueue {
        let q = std.heap.page_allocator.create(DeferredTaskQueue) catch unreachable;
        q.capacity = 32;
        q.len = 0;
        q.entries = std.heap.page_allocator.alloc(DeferredEntry, 32) catch unreachable;
        return q;
    }

    pub fn count(): usize {
        return _.len;
    }

    pub fn postTask(*_, ctx: *!?, task: DeferredFn): bool {
        // Check for duplicate context to deduplicate flush requests
        if (ctx != null) {
            for (i of 0.._.len) {
                if (_.entries[i].ctx == ctx) {
                    _.entries[i].task = task;
                    return true;
                }
            }
        }

        if (_.len >= _.capacity) {
            const new_cap = _.capacity * 2;
            let new_entries = std.heap.page_allocator.alloc(DeferredEntry, new_cap) catch unreachable;
            for (i of 0.._.len) {
                new_entries[i] = _.entries[i];
            }
            std.heap.page_allocator.free(_.entries);
            _.entries = new_entries;
            _.capacity = new_cap;
        }

        const idx = _.len;
        _.len += 1;
        _.entries[idx].ctx = ctx;
        _.entries[idx].task = task;
        return false;
    }

    pub fn unregisterTask(*_, ctx: *!?): bool {
        if (ctx == null || _.len == 0) return false;
        for (i of 0.._.len) {
            if (_.entries[i].ctx == ctx) {
                _.len -= 1;
                if (i < _.len) {
                    _.entries[i] = _.entries[_.len];
                }
                return true;
            }
        }
        return false;
    }

    pub fn run(*_): usize {
        if (_.len == 0) return 0;
        const total = _.len;
        for (i of 0..total) {
            const entry = _.entries[i];
            entry.task(entry.ctx);
        }
        _.len = 0;
        return total;
    }
}
