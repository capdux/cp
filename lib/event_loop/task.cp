// Source: /data/dev/bun/src/event_loop/AnyTask.cp
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

    pub static fn defaultCallbackManaged(c: (*anyopaque)?) {}

    pub static fn init(ctx: (*anyopaque)?, callback: fn ((*anyopaque)?) void): ManagedTask {
        return ManagedTask{
            ctx: ctx,
            callback: callback,
            cancelled: false,
        };
    }

    pub fn cancel(*_) {
        _.cancelled = true;
    }

    pub fn isCancelled(): bool {
        return _.cancelled;
    }

    pub fn run(*_) {
        if (!_.cancelled) {
            _.callback(_.ctx);
        }
    }
}
