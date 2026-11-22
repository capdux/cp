// Source: Absorbed from /data/dev/bun/src/event_loop/AnyTask.cp | Synced: 2026-11-22T12:15:29Z
// lib/event/task.cp
// AnyTask, AnyTaskWithExtraContext and ManagedTask dynamic task abstractions for cap:event

import std from "std";

pub struct AnyTask {
    pub ctx: (*anyopaque)? = null,
    pub callback: fn ((*anyopaque)?) void = defaultCallback,

    pub static fn defaultCallback(c: (*anyopaque)?) {}

    pub static fn init(ctx: (*anyopaque)?, callback: fn ((*anyopaque)?) void): AnyTask {
        return AnyTask{
            ctx: ctx,
            callback: callback,
        };
    }

    pub fn run(*_) {
        _.callback(_.ctx);
    }
}

pub struct AnyTaskWithExtraContext {
    pub ctx: (*anyopaque)? = null,
    pub callback: fn ((*anyopaque)?, (*anyopaque)?) void = defaultCallbackExtra,
    pub next: (*AnyTaskWithExtraContext)? = null,

    pub static fn defaultCallbackExtra(c: (*anyopaque)?, extra: (*anyopaque)?) {}

    pub static fn init(ctx: (*anyopaque)?, callback: fn ((*anyopaque)?, (*anyopaque)?) void): AnyTaskWithExtraContext {
        return AnyTaskWithExtraContext{
            ctx: ctx,
            callback: callback,
            next: null,
        };
    }

    pub fn run(*_, extra: (*anyopaque)?) {
        _.callback(_.ctx, extra);
    }
}

pub struct ManagedTask {
    pub ctx: (*anyopaque)? = null,
    pub callback: fn ((*anyopaque)?) void = defaultCallbackManaged,
    pub cancelled: bool = false,
    pub is_heap_allocated: bool = false,

    pub static fn defaultCallbackManaged(c: (*anyopaque)?) {}

    pub static fn init(ctx: (*anyopaque)?, callback: fn ((*anyopaque)?) void): ManagedTask {
        return ManagedTask{
            ctx: ctx,
            callback: callback,
            cancelled: false,
            is_heap_allocated: false,
        };
    }

    pub static fn create(ctx: (*anyopaque)?, callback: fn ((*anyopaque)?) void): *ManagedTask! {
        let ptr: *ManagedTask! = unsafe { @ptrFromInt(8) };
        zig {
            const mem = std.heap.page_allocator.create(ManagedTask) catch unreachable;
            mem.* = ManagedTask{
                .ctx = ctx,
                .callback = callback,
                .cancelled = false,
                .is_heap_allocated = true,
            };
            ptr = mem;
        }
        return ptr;
    }

    pub fn cancel(*_) {
        _.cancelled = true;
    }

    pub fn isCancelled(): bool {
        return _.cancelled;
    }

    pub fn run(*_) {
        if (!_.cancelled) {
            let cb = _.callback;
            let c = _.ctx;
            cb(c);
        }
    }

    ~init() {
        if (_.is_heap_allocated) {
            _.is_heap_allocated = false;
            zig {
                std.heap.page_allocator.destroy(self);
            }
        }
    }
}
