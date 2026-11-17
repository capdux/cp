// Source: Custom implementation by Antigravity AI
// ============================================================================
// Absorbed from Bun: bun/src/event_loop/task.cp
// Upstream Sync: Refer to bun/src/event_loop/task.zig for future upstream updates.
// ============================================================================
// lib/event/task.cp
// AnyTask and ManagedTask dynamic task abstractions for event loop dispatch
// Absorbed from Bun src/event_loop/AnyTask.cp and ManagedTask.cp with pure CP semantics

pub struct AnyTask {
    ctx: *!? = null,
    callback: *const fn (*!?) void = undefined,

    pub static fn init(ctx: *!?, cb: *const fn (*!?) void): AnyTask {
        return AnyTask {
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
    ctx: *!? = null,
    callback: *const fn (*!?) void = undefined,
    cancelled: bool = false,

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
