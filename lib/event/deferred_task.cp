// Source: /data/dev/bun/src/event_loop/DeferredTaskQueue.cp
// lib/event/deferred_task.cp
// Zero-allocation deduplicated deferred microtask queue for cap:event

import std from "std";

pub const MAX_DEFERRED_TASKS: usize = 128;

pub type DeferredFn = fn (*anyopaque) bool;

pub struct DeferredTaskEntry {
    pub ctx: *anyopaque,
    pub task: fn (*anyopaque) bool,
}

pub type DeferredEntry = DeferredTaskEntry;

pub struct DeferredTaskQueue {
    pub entries: [MAX_DEFERRED_TASKS]DeferredTaskEntry = undefined,
    pub item_count: usize = 0,

    pub static fn init(): DeferredTaskQueue {
        return DeferredTaskQueue{
            entries: undefined,
            item_count: 0,
        };
    }

    pub fn postTask(*_, ctx: *anyopaque, task: fn (*anyopaque) bool): bool {
        let i: usize = 0;
        while (i < _.item_count) {
            if (_.entries[i].ctx == ctx) {
                return false;
            }
            i += 1;
        }

        if (_.item_count >= MAX_DEFERRED_TASKS) {
            return false;
        }

        _.entries[_.item_count] = DeferredTaskEntry{
            ctx: ctx,
            task: task,
        };
        _.item_count += 1;
        return true;
    }

    pub fn unregisterTask(*_, ctx: *anyopaque): bool {
        let i: usize = 0;
        while (i < _.item_count) {
            if (_.entries[i].ctx == ctx) {
                _.entries[i] = _.entries[_.item_count - 1];
                _.item_count -= 1;
                return true;
            }
            i += 1;
        }
        return false;
    }

    pub fn run(*_): usize {
        let executed: usize = 0;
        let i: usize = 0;
        while (i < _.item_count) {
            let entry = _.entries[i];
            let should_repeat = entry.task(entry.ctx);
            executed += 1;

            if (!should_repeat) {
                _.entries[i] = _.entries[_.item_count - 1];
                _.item_count -= 1;
            } else {
                i += 1;
            }
        }
        return executed;
    }

    pub fn count(): usize {
        return _.item_count;
    }

    pub fn isEmpty(): bool {
        return _.item_count == 0;
    }

    pub fn clear(*_) {
        _.item_count = 0;
    }
}
