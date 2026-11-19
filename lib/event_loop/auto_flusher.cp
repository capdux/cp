// Source: /data/dev/bun/src/event_loop/AutoFlusher.cp
import std from "std";
import { DeferredTaskQueue } from "./deferred_task_queue.cp";

pub struct AutoFlusher {
    pub registered: bool = false,

    pub static fn init(): AutoFlusher {
        return AutoFlusher{ registered: false };
    }

    pub fn isRegistered(): bool {
        return _.registered;
    }

    pub fn markRegistered(*_) {
        _.registered = true;
    }

    pub fn markUnregistered(*_) {
        _.registered = false;
    }

    pub fn registerDeferred(*_, queue: *DeferredTaskQueue, ctx: *anyopaque, task: fn (*anyopaque) bool): bool {
        if (_.registered) return false;
        _.registered = true;
        return queue.postTask(ctx, task);
    }

    pub fn unregisterDeferred(*_, queue: *DeferredTaskQueue, ctx: *anyopaque): bool {
        if (!_.registered) return false;
        _.registered = false;
        return queue.unregisterTask(ctx);
    }
}
