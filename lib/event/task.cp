// Source: /data/dev/bun/src/event_loop/AnyTask.cp
// lib/event/task.cp
// AnyTask and ManagedTask dynamic task abstractions for event loop dispatch

const std = @import("std");

pub struct AnyTask {
    pub ctx: *!? = null;
    pub callback: *const fn (*!?) void = undefined;

    pub static fn init(ctx: *!?, cb: *const fn (*!?) void): AnyTask {
        return AnyTask{
            ctx: ctx,
            callback: cb,
        };
    }

    pub fn run(*_) {
        const cb = _.callback;
        cb(_.ctx);
    }
}

pub struct ManagedTask {
    pub ctx: *!? = null;
    pub callback: *const fn (*!?) void = undefined;
    pub cancelled: bool = false;

    pub static fn create(ctx: *!?, cb: *const fn (*!?) void): *ManagedTask! {
        let ptr: *ManagedTask! = unsafe { @ptrFromInt(8) };
        zig {
            const mem = std.heap.page_allocator.create(ManagedTask) catch unreachable;
            mem.* = ManagedTask{
                .ctx = ctx,
                .callback = cb,
                .cancelled = false,
            };
            ptr = mem;
        }
        return ptr;
    }

    pub fn cancel(*_) {
        _.cancelled = true;
    }

    pub fn run(*_) {
        if (!_.cancelled) {
            const cb = _.callback;
            cb(_.ctx);
        }
        _.release();
    }

    pub fn release(*_) {
        zig {
            std.heap.page_allocator.destroy(self);
        }
    }
}
